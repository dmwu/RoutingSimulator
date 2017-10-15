#include "config.h"
#include <sstream>
#include <strstream>
#include <iostream>
#include <string.h>
#include <unistd.h>
#include <list>
#include <math.h>
#include "network.h"
#include "randomqueue.h"
#include "subflow_control.h"
#include "shortflows.h"
#include "pipe.h"
#include "eventlist.h"
#include "logfile.h"
#include "loggers.h"
#include "clock.h"
#include "mtcp.h"
#include "tcp.h"
#include "tcp_transfer.h"
#include "cbr.h"
#include "firstfit.h"
#include "topology.h"
#include "connection_matrix.h"

#include "main.h"

// TOPOLOGY TO USE
#if CHOSEN_TOPO == FAT
#include "fat_tree_topology.h"
#elif CHOSEN_TOPO == RRG
#include "rand_regular_topology.h"
#endif

// Simulation params
#define PRINT_PATHS 1
#define PERIODIC 0

int RTT = 100; // Identical RTT microseconds = 0.1 ms
int N = NSW;

FirstFit* ff = NULL;
unsigned int subflow_count = 1;

string ntoa(double n);
string itoa(uint64_t n);
bool isRouteInvalid(route_t* rt);

//#define SWITCH_BUFFER (SERVICE * RTT / 1000)
#define USE_FIRST_FIT 0
#define FIRST_FIT_INTERVAL 100

EventList eventlist;

Logfile* lg;

void exit_error(char* progr) {
    cout << "Usage " << progr << " [UNCOUPLED(DEFAULT)|COUPLED_INC|FULLY_COUPLED|COUPLED_EPSILON] [epsilon][COUPLED_SCALABLE_TCP" << endl;
    exit(1);
}

void print_path(std::ofstream &paths,route_t* rt){
  for (unsigned int i=1;i<rt->size()-1;i+=2){
    RandomQueue* q = (RandomQueue*)rt->at(i);
    if (q!=NULL)
      paths << q->str() << " ";
    else 
      paths << "NULL ";
  }
  paths<<endl;
}

int main(int argc, char **argv) {
    eventlist.setEndtime(timeFromSec(10000.01));
    Clock c(timeFromSec(50 / 100.), eventlist);
    int algo = COUPLED_EPSILON;
    double epsilon = 1;
    int param = 0;
    int fail_id = -1;
    stringstream filename(ios_base::out);
    string traf_file_name;
    Logfile * logfile;
    if (argc > 1) {
      int i = 1;
      if (!strcmp(argv[1],"-o")){
	    filename << argv[2];
	    i+=2;
      }
      else
	    filename << "logout.dat";

      if (argc>i&&!strcmp(argv[i],"-sub")){
	    subflow_count = atoi(argv[i+1]);
	    i+=2;
      }

      if (argc>i&&!strcmp(argv[i],"-param")){
	    param = atoi(argv[i+1]);
	    i+=2;
      }

      if (argc>i&&!strcmp(argv[i],"-fail")){
	    fail_id = atoi(argv[i+1]);
	    i+=2;
      }
//
//      if (argc>i+1){
//        epsilon = -1;
//        if (!strcmp(argv[i], "UNCOUPLED"))
//          algo = UNCOUPLED;
//        else if (!strcmp(argv[i], "COUPLED_INC"))
//          algo = COUPLED_INC;
//        else if (!strcmp(argv[i], "FULLY_COUPLED"))
//          algo = FULLY_COUPLED;
//        else if (!strcmp(argv[i], "COUPLED_TCP"))
//          algo = COUPLED_TCP;
//        else if (!strcmp(argv[i], "COUPLED_SCALABLE_TCP"))
//          algo = COUPLED_SCALABLE_TCP;
//        else if (!strcmp(argv[i], "COUPLED_EPSILON")) {
//            algo = COUPLED_EPSILON;
//            if (argc > i+1)
//            epsilon = atof(argv[i+1]);
//            printf("Using epsilon %f\n", epsilon);
//        } else
//            exit_error(argv[0]);
//      }
      traf_file_name = argv[i];
      srand(time(NULL));
      logfile = new Logfile(filename.str(), eventlist);
    }else{
        cout<<"wrong arguments!"<<endl;
        exit(1);
    }

#if PRINT_PATHS
    filename << ".paths";
    std::ofstream paths(filename.str().c_str());
    if (!paths){
      cout << "Can't open for writing paths file!"<<endl;
      exit(1);
    }
#endif

    int tot_subs = 0;
    int cnt_con = 0;

    lg = logfile;

    logfile->setStartTime(timeFromSec(0));

    SinkLoggerSampling sinkLogger = SinkLoggerSampling(timeFromMs(10), eventlist);
    logfile->addLogger(sinkLogger);

    //TcpLoggerSimple logTcp;logfile.addLogger(logTcp);


    TcpSrc* tcpSrc;
    TcpSink* tcpSnk;


    route_t* routeout, *routein;
    double extrastarttime;

    TcpRtxTimerScanner tcpRtxScanner(timeFromMs(10), eventlist);
   
    MultipathTcpSrc* mtcp;
    
    int dest;

#if USE_FIRST_FIT
    if (subflow_count==1){
      ff = new FirstFit(timeFromMs(FIRST_FIT_INTERVAL),eventlist);
    }
#endif

#ifdef FAT_TREE
    FatTreeTopology* top = new FatTreeTopology(logfile, &eventlist,ff, fail_id);
#endif

#ifdef OV_FAT_TREE
    OversubscribedFatTreeTopology* top = new OversubscribedFatTreeTopology(&logfile, &eventlist,ff);
#endif

#ifdef MH_FAT_TREE
    MultihomedFatTreeTopology* top = new MultihomedFatTreeTopology(&logfile, &eventlist,ff);
#endif

#ifdef STAR
    StarTopology* top = new StarTopology(&logfile, &eventlist,ff);
#endif

#ifdef BCUBE
    BCubeTopology* top = new BCubeTopology(&logfile,&eventlist,ff);
    cout << "BCUBE " << K << endl;
#endif

#ifdef VL2
    VL2Topology* top = new VL2Topology(&logfile,&eventlist,ff);
#endif

//< Ankit added
#ifdef RAND_REGULAR
    //= "../../../rand_topologies/rand_" + ntoa(NSW) + "_" + ntoa(R) + ".txt";
    RandRegularTopology* top = new RandRegularTopology(&logfile,&eventlist,ff, rfile);
#endif
//>

    vector<route_t*>*** net_paths;
    net_paths = new vector<route_t*>**[NHOST];

    int* is_dest = new int[NHOST];
    
    for (int i=0;i<NHOST;i++){
      is_dest[i] = 0;
      net_paths[i] = new vector<route_t*>*[NHOST];
      for (int j = 0;j<NHOST;j++)
	net_paths[i][j] = NULL;
    }
    
    if (ff)
      ff->net_paths = net_paths;
    
    vector<int>* destinations;

    vector<int> subflows_chosen;
    int connID = -1;
    
    char file_name[1024];
    strcpy(file_name, traf_file_name.c_str());
    
    string line;
    ifstream traf_file(file_name);
    if(!traf_file.is_open()) {
        cout << "cannot find trace file:"<<file_name<<"!" << endl;
        char buffer[256];
        char *answer = getcwd(buffer, sizeof(buffer));
        string s_cwd;
        if (answer)
        {
            s_cwd = answer;
            cout<<"current directory:"<<s_cwd<<endl;
        }
    }
    getline(traf_file, line);
    int pos = line.find(" ");
    string str = line.substr(0, pos);
    int rack_n = atoi(str.c_str());
    int coflowNum = atoi(line.substr(pos+1).c_str());
    cout<<"Num of tors:"<<rack_n<<" Num of Coflows:"<<coflowNum<<endl;

    vector<uint64_t*> flows_sent;
    vector<uint64_t*> flows_recv;
    vector<bool*> flows_finish;
    
    while (getline(traf_file, line))
    {
        int i = 0;
        int pos = line.find(" ");
        string coflow_id = line.substr(0, pos);
        i = ++pos;
        pos = line.find(" ", pos);
        string str = line.substr(i, pos-i);
        simtime_picosec arrivalTime = (simtime_picosec)(atof(str.c_str())*1000000);
	    extrastarttime = atof(str.c_str());
        i = ++pos;
        pos = line.find(" ", pos);
        str = line.substr(i, pos-i);
        int mapper_n = atoi(str.c_str());
        vector<int> mappers;
	    vector<int> mappers_sw;
        for (int k = 0; k < mapper_n; k++) {
            i = ++pos;
            pos = line.find(" ", pos);
            str = line.substr(i, pos-i);
	        mappers.push_back(top->generate_server(atoi(str.c_str()), rack_n));
	        mappers_sw.push_back(atoi(str.c_str()));
        }
        i = ++pos;
        pos = line.find(" ", pos);
        str = line.substr(i, pos-i);
        int reducer_n = atoi(str.c_str());
        for (int k = 0; k < reducer_n; k++) {
            i = ++pos;
            pos = line.find(" ", pos);
            if (pos == string::npos)
                str = line.substr(i, line.length());
            else
                str = line.substr(i, pos-i);
            
            int posi = str.find(":");
            string sub = str.substr(0, posi);
            int reducer = atoi(sub.c_str());
            sub = str.substr(posi+1, str.length());
            uint64_t volume_bytes = (uint64_t) (atof(sub.c_str())/mappers.size()*1000000);

            int dest = top->generate_server(reducer, rack_n);
	        int dest_sw = dest/(K/2);
            for (int t = 0; t < mappers.size(); t++) {
                int src = mappers[t];
		        int src_sw = src/(K/2);
                connID++;
                uint64_t* curnt_sent = new uint64_t;
                *curnt_sent = 0;
                uint64_t* curnt_recv = new uint64_t;
                *curnt_recv = 0;
                bool* curnt_finish = new bool;
                *curnt_finish = false;
                flows_sent.push_back(curnt_sent);
                flows_recv.push_back(curnt_recv);
                flows_finish.push_back(curnt_finish);
                
                //cout << "flow id: " << connID << " src: " << src << " dest: " << dest << endl;
                if (!net_paths[src][dest]){
                    //cout << "Getting paths from topo" << endl;
                    net_paths[src][dest] = top->get_paths(src,dest);
                }
                
                {
                    for (int connection=0;connection<1;connection++){
                        if (algo == COUPLED_EPSILON)
                            mtcp = new MultipathTcpSrc(algo, eventlist, NULL, epsilon);
                        else
                            mtcp = new MultipathTcpSrc(algo, eventlist, NULL);

                        
                        subflows_chosen.clear();
                        
                        unsigned int it_sub;
                        unsigned int crt_subflow_count = subflow_count;

                        it_sub = crt_subflow_count > net_paths[src][dest]->size()?net_paths[src][dest]->size():crt_subflow_count;
                        
                        unsigned int use_all = it_sub==net_paths[src][dest]->size();
                        unsigned int choice = 0;
                        for (unsigned int inter = 0; inter < it_sub; inter++) {

#ifdef FAT_TREE
                            choice = rand()%net_paths[src][dest]->size();
                            //cout << "choice = " << choice << endl;
#endif
                            
#ifdef RAND_REGULAR
                            choice = (choice + 1)%net_paths[src][dest]->size();
#endif
                            
#ifdef OV_FAT_TREE
                            choice = rand()%net_paths[src][dest]->size();
#endif
                            
#ifdef MH_FAT_TREE
                            if (use_all)
                                choice = inter;
                            else
                                choice = rand()%net_paths[src][dest]->size();
#endif
                            
#ifdef VL2
                            choice = rand()%net_paths[src][dest]->size();
#endif
                            
#ifdef STAR
                            choice = 0;
#endif
                            
#ifdef BCUBE
                            //choice = inter;
                            
                            
                            int min = -1, max = -1,minDist = 1000,maxDist = 0;
                            if (subflow_count==1){
                                //find shortest and longest path
                                for (int dd=0;dd<net_paths[src][dest]->size();dd++){
                                    if (net_paths[src][dest]->at(dd)->size()<minDist){
                                        minDist = net_paths[src][dest]->at(dd)->size();
                                        min = dd;
                                    }
                                    if (net_paths[src][dest]->at(dd)->size()>maxDist){
                                        maxDist = net_paths[src][dest]->at(dd)->size();
                                        max = dd;
                                    }
                                }
                                choice = min;
                            } else
                                choice = rand()%net_paths[src][dest]->size();
#endif

			                if (isRouteInvalid(net_paths[src][dest]->at(choice))) {
                                cout << "coflow_id: " << coflow_id << " sw: " << src_sw << " -> " << dest_sw
                                     << " vol: " << volume_bytes/1000000 << "MB, time: " << arrivalTime/1000000<< "(ms)" << endl;
                                cout << "dropped flow " << connID << endl;
				                continue;
			                }
                            
                            subflows_chosen.push_back(choice);
                            
                            tcpSrc = new TcpSrc(NULL, NULL, eventlist, flows_sent[connID], flows_recv[connID], volume_bytes, flows_finish[connID], arrivalTime, connID);
                            tcpSnk = new TcpSink();
                            // yiting
                            tcpSnk->set_super_id(connID);

                            tcpSrc->setName("mtcp_" + ntoa(src) + "_" + ntoa(inter) + "_" + ntoa(dest)+"("+ntoa(connection)+")");
                            logfile->writeName(*tcpSrc);

                            tcpSnk->setName("mtcp_sink_" + ntoa(src) + "_" + ntoa(inter) + "_" + ntoa(dest)+ "("+ntoa(connection)+")");
                            logfile->writeName(*tcpSnk);

                            tcpRtxScanner.registerTcp(*tcpSrc);

                            if (choice>=net_paths[src][dest]->size()){
                                cout << "Weird path choice " << choice << " out of " << net_paths[src][dest]->size();
                                exit(1);
                            }
                            
#if PRINT_PATHS
                            paths << "Route from "<< ntoa(src) << " to " << ntoa(dest) << "  (" << choice << ") -> " ;
                            print_path(paths,net_paths[src][dest]->at(choice));
#endif
                            
                            routeout = new route_t(*(net_paths[src][dest]->at(choice)));
                            routeout->push_back(tcpSnk);
                            
                            routein = new route_t();
                            routein->push_back(tcpSrc);
                            
                            mtcp->addSubflow(tcpSrc);
                            
                            if (inter == 0) {
                                mtcp->setName("multipath" + ntoa(src) + "_" + ntoa(dest)+"("+ntoa(connection)+")");
                                logfile->writeName(*mtcp);
                            }
                            
                            tcpSrc->connect(*routeout, *routein, *tcpSnk, timeFromMs(extrastarttime));
                            
#ifdef PACKET_SCATTER
                            tcpSrc->set_paths(net_paths[src][dest]);
                            cout << "Using PACKET SCATTER!!!!"<<endl<<end;
#endif
                            
                            if ( ff&&!inter )
                                ff->add_flow(src,dest,tcpSrc);

                        }
                    }
                }
            }
        }
        
        //cout << endl;
    }

    int pktsize = TcpPacket::DEFAULTDATASIZE;
    logfile->write("# pktsize=" + ntoa(pktsize) + " bytes");
    logfile->write("# subflows=" + ntoa(subflow_count));
    logfile->write("# hostnicrate = " + ntoa(HOST_NIC) + " pkt/sec");
    logfile->write("# corelinkrate = " + ntoa(HOST_NIC*CORE_TO_HOST) + " pkt/sec");
    //logfile->write("# buffer = " + ntoa((double) (queues_na_ni[0][1]->_maxsize) / ((double) pktsize)) + " pkt");
    double rtt = timeAsSec(timeFromUs(RTT));
    logfile->write("# rtt =" + ntoa(rtt));

    // GO!
    while (eventlist.doNextEvent()) {
    }
}


string ntoa(double n) {
    stringstream s;
    s << n;
    return s.str();
}

string itoa(uint64_t n) {
    stringstream s;
    s << n;
    return s.str();
}

bool isRouteInvalid(route_t* rt){
  bool fail = true;
  for (unsigned int i=1;i<rt->size()-1;i+=2) {
      if (rt->at(i) == NULL) {
          return fail;
      }
  }
  return false;
}
