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


//#define SWITCH_BUFFER (SERVICE * RTT / 1000)
#define USE_FIRST_FIT 0
#define FIRST_FIT_INTERVAL 100


// Simulation params
#define PRINT_PATHS 1
#define PERIODIC 0

int RTT = 1; // Identical RTT microseconds = 0.001 ms [WDM] I change it to 1us to match the link speed.
int N = NSW;

FirstFit *ff = NULL;
unsigned int subflow_count = 1;

string ntoa(double n);

string itoa(uint64_t n);

bool isRouteValid(route_t *rt);

EventList eventlist;

Logfile *lg;

void exit_error(char *progr) {
    cout << "Usage " << progr
         << " [UNCOUPLED(DEFAULT)|COUPLED_INC|FULLY_COUPLED|COUPLED_EPSILON] [epsilon][COUPLED_SCALABLE_TCP" << endl;
    exit(1);
}

void print_path(std::ofstream &paths, route_t *rt) {
    for (unsigned int i = 0; i < rt->size(); i += 2) {
        RandomQueue *q = (RandomQueue *) rt->at(i);
        if (q != NULL)
            paths << q->str() << " ";
        else
            paths << "NULL ";
    }
    paths << endl;
}

int main(int argc, char **argv) {
    clock_t begin = clock();
    eventlist.setEndtime(timeFromSec(10000.01));
    double epsilon = 1;
    int routing = 1; //[WDM] 0 stands for ecmp; 1 stands for two-level routing
    int fail_id = -1;
    stringstream filename(ios_base::out);
    string traf_file_name;
    Logfile *logfile;
    if (argc > 1) {
        int i = 1;
        if (!strcmp(argv[1], "-o")) {
            filename << argv[2];
            i += 2;
        } else
            filename << "logout.dat";

        if (argc > i && !strcmp(argv[i], "-routing")) {
            routing = atoi(argv[i + 1]);
            i += 2;
        }

        if(routing == 0){
            cout<<"Using ECMP Routing"<<endl;
        }else{
            cout<<"Using Standard Routing"<<endl;
        }

        if (argc > i && !strcmp(argv[i], "-fail")) {
            fail_id = atoi(argv[i + 1]);
            i += 2;
        }
        traf_file_name = argv[i];
        srand(time(NULL));
        logfile = new Logfile(filename.str(), eventlist);
    } else {
        cout << "wrong arguments!" << endl;
        exit(1);
    }

#if PRINT_PATHS
    filename << ".paths";
    std::ofstream pathFile(filename.str().c_str());
    if (!pathFile) {
        cout << "Can't open for writing paths file!" << endl;
        exit(1);
    }
#endif


    TcpSrc *tcpSrc;
    TcpSink *tcpSnk;


    route_t *routeout, *routein;
    double extrastarttime;

    TcpRtxTimerScanner tcpRtxScanner(timeFromMs(10), eventlist);

    MultipathTcpSrc *mtcp;

#if USE_FIRST_FIT
    if (subflow_count==1){
      ff = new FirstFit(timeFromMs(FIRST_FIT_INTERVAL),eventlist);
    }
#endif

    FatTreeTopology *top = new FatTreeTopology(logfile, &eventlist, ff, fail_id);


    vector<route_t *> ***net_paths;
    net_paths = new vector<route_t *> **[NHOST];

    int *is_dest = new int[NHOST];
    double simStartingTime_ms = -1;
    for (int i = 0; i < NHOST; i++) {
        is_dest[i] = 0;
        net_paths[i] = new vector<route_t *> *[NHOST];
        for (int j = 0; j < NHOST; j++)
            net_paths[i][j] = NULL;
    }

    if (ff)
        ff->net_paths = net_paths;

    vector<int> *destinations;

    vector<int> subflows_chosen;
    int connID = 0;
    int subFlowID;
    char file_name[1024];
    strcpy(file_name, traf_file_name.c_str());

    string line;
    ifstream traf_file(file_name);
    if (!traf_file.is_open()) {
        cout << "cannot find trace file:" << file_name << "!" << endl;
        char buffer[256];
        char *answer = getcwd(buffer, sizeof(buffer));
        string s_cwd;
        if (answer) {
            s_cwd = answer;
            cout << "current directory:" << s_cwd << endl;
        }
    }
    getline(traf_file, line);
    int pos = line.find(" ");
    string str = line.substr(0, pos);
    int rack_n = atoi(str.c_str());
    int coflowNum = atoi(line.substr(pos + 1).c_str());
    cout << "Num of tors:" << rack_n << " Num of Coflows:" << coflowNum << endl;

    vector<uint64_t *> flows_sent;
    vector<uint64_t *> flows_recv;
    vector<bool *> flows_finish;

    vector<double>* flowStats = new vector<double>();

    int src, dest;
    route_t* path = NULL;

    while (getline(traf_file, line)) {
        int i = 0;
        int pos = line.find(" ");
        string coflow_id = line.substr(0, pos);
        i = ++pos;
        pos = line.find(" ", pos);
        string str = line.substr(i, pos - i);
        double arrivalTime_ms = atof(str.c_str());

        if(arrivalTime_ms < simStartingTime_ms || simStartingTime_ms < 0){
            simStartingTime_ms = arrivalTime_ms;
        }

        i = ++pos;
        pos = line.find(" ", pos);
        str = line.substr(i, pos - i);
        int mapper_n = atoi(str.c_str());
        vector<int> mappers;

        for (int k = 0; k < mapper_n; k++) {
            i = ++pos;
            pos = line.find(" ", pos);
            str = line.substr(i, pos - i);
            if(SERVER_LEVEL_TRAFFIC){
                mappers.push_back(atoi(str.c_str()));
            }else {
                mappers.push_back(top->generate_server(atoi(str.c_str()), rack_n));
            }
        }
        i = ++pos;
        pos = line.find(" ", pos);
        str = line.substr(i, pos - i);
        subFlowID = 0;
        int reducer_n = atoi(str.c_str());
        bool coflowHasPath = true;
        for (int k = 0; k < reducer_n && coflowHasPath; k++) {
            i = ++pos;
            pos = line.find(" ", pos);
            if (pos == string::npos)
                str = line.substr(i, line.length());
            else
                str = line.substr(i, pos - i);

            int posi = str.find(":");
            string sub = str.substr(0, posi);
            int reducer = atoi(sub.c_str());
            sub = str.substr(posi + 1, str.length());
            uint64_t volume_bytes = (uint64_t) (atof(sub.c_str()) / mappers.size() * 1000000);
            if(SERVER_LEVEL_TRAFFIC){
                dest = reducer;
            }
            else {
                dest = top->generate_server(reducer, rack_n);
            }
            for (int t = 0; t < mappers.size(); t++) {
                src = mappers[t];
                uint64_t *curnt_sent = new uint64_t;
                *curnt_sent = 0;
                uint64_t *curnt_recv = new uint64_t;
                *curnt_recv = 0;
                bool *curnt_finish = new bool;
                *curnt_finish = false;
                flows_sent.push_back(curnt_sent);
                flows_recv.push_back(curnt_recv);
                flows_finish.push_back(curnt_finish);
                if(routing == 0){
                    //ecmp
                    if (!net_paths[src][dest]) {
                        net_paths[src][dest] = top->get_paths_ecmp(src, dest);
                    }
                    int choice = rand()%net_paths[src][dest]->size();
                    path = net_paths[src][dest]->at(choice);

                }else{
                    path = top->get_path_2levelrt(src, dest);
                }

                if (!isRouteValid(path)) {

                    if (!net_paths[src][dest]) {
                        net_paths[src][dest] = top->get_paths_ecmp(src, dest);
                    }

                    for (int i = 0; i < net_paths[src][dest]->size(); i++) {
                        path = net_paths[src][dest]->at(i);
                        if (isRouteValid(path)) {
                            break;
                        }
                    }

                    if (!isRouteValid(path)) {
                        cout << "[ERROR] coflow:" << coflow_id << " cannot find a valid path for flow:" << src << "->"
                             << dest << endl;
                        coflowHasPath = false;
                        break;
                    }
                }


                tcpSrc = new TcpSrc(NULL, NULL, eventlist, flows_sent[connID], flows_recv[connID],
                                    volume_bytes, flows_finish[connID], arrivalTime_ms, subFlowID,
                                    atoi(coflow_id.c_str()), flowStats);
                connID++;
                subFlowID++;

                tcpSnk = new TcpSink();
                // yiting
                tcpSnk->set_super_id(connID);

                tcpSrc->setName("mtcp_" + ntoa(src) + "_" + ntoa(0) + "_" + ntoa(dest) + "(" +
                                ntoa(0) + ")");
                logfile->writeName(*tcpSrc);

                tcpSnk->setName("mtcp_sink_" + ntoa(src) + "_" + ntoa(0) + "_" + ntoa(dest) + "(" +
                                ntoa(0) + ")");
                logfile->writeName(*tcpSnk);

                tcpRtxScanner.registerTcp(*tcpSrc);


#if PRINT_PATHS
                pathFile << "Coflow:subflow "<<coflow_id<<":"<<subFlowID;
                pathFile << " Route from " << ntoa(src) << " to " << ntoa(dest) << " -> ";
                print_path(pathFile, path);
#endif

                routeout = new route_t(*path);
                routeout->push_back(tcpSnk);

                routein = new route_t();
                RandomQueue* destTxQueue = new RandomQueue(speedFromPktps(HOST_NIC), memFromPkt(FEEDER_BUFFER + RANDOM_BUFFER),
                                                           eventlist, NULL, memFromPkt(RANDOM_BUFFER));
                destTxQueue->setName("DST_TX_"+ntoa(src)+"-"+ntoa(dest));
                routein->push_back(destTxQueue);
                for(int i = path->size()-2; i >=1; i--) {
                    routein->push_back(path->at(i));
                }
                RandomQueue* srcRxQueue = new RandomQueue(speedFromPktps(HOST_NIC), memFromPkt(FEEDER_BUFFER + RANDOM_BUFFER),
                                                          eventlist, NULL, memFromPkt(RANDOM_BUFFER));
                srcRxQueue->setName("SRC_RX"+ntoa(src)+"-"+ntoa(dest));

                routein->push_back(srcRxQueue);

                routein->push_back(tcpSrc);

                tcpSrc->connect(*routeout, *routein, *tcpSnk, timeFromMs(arrivalTime_ms));

#ifdef PACKET_SCATTER
                tcpSrc->set_paths(net_paths[src][dest]);
                cout << "Using PACKET SCATTER!!!!"<<endl<<end;
#endif

                if (ff)
                    ff->add_flow(src, dest, tcpSrc);


            }
        }
    }

    cout << "Total Number of TCP flows:" << connID << endl;
    double rtt = timeAsSec(timeFromUs(RTT));
    tcpRtxScanner.StartFrom(timeFromMs(simStartingTime_ms));
    // GO!
    while (eventlist.doNextEvent()) {

    }
    double elapsed_secs = double(clock() - begin) / CLOCKS_PER_SEC;
    cout << "Elapsed:" << elapsed_secs << "s" << endl;
    double sum = 0;
    for (int i =0; i < flowStats->size(); i++){
        sum += flowStats->at(i);
    }
    cout<<"Routing:"<<routing<<endl;
    cout<<"finished flows:"<<flowStats->size()<<" all flows:"<<connID<<endl;
    cout<<"Average coFCT:"<<sum/flowStats->size()<<endl;
    std::sort(flowStats->begin(), flowStats->end());
    std::ofstream ofs;
    ofs.open ("fcts"+filename.str()+".txt", std::ofstream::out);
    for(int i = 0; i < flowStats->size();i++){
        ofs<<flowStats->at(i)<<endl;
    }
    ofs.close();
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

bool isRouteValid(route_t *rt) {
    for (unsigned int i = 0; i < rt->size(); i += 2) {
        if (rt->at(i) == NULL) {
            return false;
        }
    }
    return rt != NULL;
}
