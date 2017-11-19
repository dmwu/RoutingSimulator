#include <sstream>
#include <strstream>
#include <iostream>
#include <string.h>
#include <unistd.h>
#include <list>
#include <math.h>
#include "LinkFailureEvent.h"
#include "network.h"
#include "randomqueue.h"
#include "pipe.h"
#include "eventlist.h"
#include "logfile.h"
#include "loggers.h"
#include "clock.h"
#include "mtcp.h"
#include "tcp.h"
#include "tcp_transfer.h"
#include "cbr.h"
#include "topology.h"
#include "config.h"
#include "topology.h"
#include "fat_tree_topology.h"
#include "F10.h"
#include "main.h"

// TOPOLOGY TO USE
#if CHOSEN_TOPO == FAT


#elif CHOSEN_TOPO == RRG
#include "rand_regular_topology.h"
#endif


#define USE_FIRST_FIT 0
#define FIRST_FIT_INTERVAL 100


// Simulation params
#define PRINT_PATHS 0
#define PERIODIC 0

int N = NSW;

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

int main(int argc, char **argv) {
    clock_t begin = clock();
    eventlist.setEndtime(timeFromSec(2000.01));

    double simStartingTime_ms = -1;
    double epsilon = 1;
    int routing = 1; //[WDM] 0 stands for ecmp; 1 stands for two-level routing
    int topology = 0; // 0 stands for fattree; 1 stands for sharebackup; 2 stands for f10
    int failedLinkId = -1;
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
        if (argc > i && !strcmp(argv[i], "-topo")) {
            topology = atoi(argv[i + 1]);
            i += 2;
        }
        if (argc > i && !strcmp(argv[i], "-routing")) {
            routing = atoi(argv[i + 1]);
            i += 2;
        }
        if (argc > i && !strcmp(argv[i], "-failLink")) {
            failedLinkId = atoi(argv[i + 1]);
            i += 2;
        }

        traf_file_name = argv[i];
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

    TcpRtxTimerScanner tcpRtxScanner(timeFromMs(TCP_TIMEOUT_SCANNER_PERIOD), eventlist); //irregular tcp retransmit timeout
    Topology *top = nullptr;
    LinkFailureEvent* linkFailureEvent;
    if(topology<=1) {
        top = new FatTreeTopology(&eventlist);
        linkFailureEvent = new LinkFailureEvent(eventlist, top, timeFromSec(100), timeFromSec(300), failedLinkId);
        linkFailureEvent->setFailureRecoveryDelay(timeFromMs(GLOBAL_REROUTE_DELAY), timeFromMs(GLOBAL_REROUTE_DELAY));
        if(topology==1){
            linkFailureEvent->UsingShareBackup = true;
            vector<int>*lpBackupUsageTracker = new vector<int>(K);
            vector<int>* upBackupUsageTracker = new vector<int>(K);
            vector<int>* coreBackupUsageTracker = new vector<int>(K);
            for(int i = 0; i < K; i++){
                (*lpBackupUsageTracker)[i] = BACKUPS_PER_GROUP;
                (*upBackupUsageTracker)[i] = BACKUPS_PER_GROUP;
                (*coreBackupUsageTracker)[i/2] = BACKUPS_PER_GROUP;
            }
            linkFailureEvent->setFailureRecoveryDelay(timeFromMs(CIRCUIT_SWITCHING_DELAY), 0);
            linkFailureEvent->setBackupUsageTracker(lpBackupUsageTracker,upBackupUsageTracker,coreBackupUsageTracker);
        }
    }
    else {
        top = new F10Topology(&eventlist);
        linkFailureEvent = new LinkFailureEvent(eventlist, top, timeFromSec(50), timeFromSec(500), failedLinkId);
        linkFailureEvent->setFailureRecoveryDelay(timeFromMs(LOCAL_REROUTE_DELAY), timeFromMs(LOCAL_REROUTE_DELAY));
    }



    int connID = 0, subFlowID;
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
            exit(1);
        }
    }
    getline(traf_file, line);
    int pos = line.find(" ");
    string str = line.substr(0, pos);
    int rack_n = atoi(str.c_str());
    int coflowNum = atoi(line.substr(pos + 1).c_str());
    cout << "Num of racks:" << rack_n << " Num of Coflows:" << coflowNum << endl;

    vector<uint64_t *> flows_sent;
    vector<uint64_t *> flows_recv;
    vector<bool *> flows_finish;
    map<int,FlowConnection*>* flowStats = new map<int, FlowConnection*>();

    int src, dest;
    route_t *path = NULL;
    srand(time(NULL));
    while (getline(traf_file, line)) {
        int i = 0;
        int pos = line.find(" ");
        string coflow_id = line.substr(0, pos);
        i = ++pos;
        pos = line.find(" ", pos);
        string str = line.substr(i, pos - i);
        double arrivalTime_ms = atof(str.c_str());

        if (arrivalTime_ms < simStartingTime_ms || simStartingTime_ms < 0) {
            simStartingTime_ms = arrivalTime_ms;
        }

        i = ++pos;
        pos = line.find(" ", pos);
        str = line.substr(i, pos - i);
        int mapper_n = atoi(str.c_str());
        vector<int> mappers;
        subFlowID=0;
        for (int k = 0; k < mapper_n; k++) {
            i = ++pos;
            pos = line.find(" ", pos);
            str = line.substr(i, pos - i);
            if (SERVER_LEVEL_TRAFFIC) {
                mappers.push_back(atoi(str.c_str()));
            } else {
                mappers.push_back(top->getServerUnderTor(atoi(str.c_str()), rack_n));
            }
        }
        i = ++pos;
        pos = line.find(" ", pos);
        str = line.substr(i, pos - i);
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
            if (SERVER_LEVEL_TRAFFIC) {
                dest = reducer;
            } else {
                dest = top->getServerUnderTor(reducer, rack_n);
            }
            for (int t = 0; t < mappers.size(); t++) {
                src = mappers[t];
                pair<route_t*, route_t*> path;
                if (routing == 0) {
                    //ecmp
                    path = top->getReroutingPath(src, dest, nullptr);

                } else {
                    path = top->getStandardPath(src,dest);
                }

                if (path.first == NULL || path.second == NULL) {
                    cout << "[ERROR] coflow:" << coflow_id << " cannot find a valid path for flow:" << src << "->"
                         << dest << endl;
                    coflowHasPath = false;
                    break;

                }
                tcpSrc = new TcpSrc(src, dest, eventlist, volume_bytes, arrivalTime_ms, connID,
                                    atoi(coflow_id.c_str()), flowStats);
                connID++;
                subFlowID++;
                tcpSnk = new TcpSink(&eventlist);
                // yiting
                tcpSnk->set_super_id(connID);

                tcpSrc->setName("mtcp_" + ntoa(src) + "_" + ntoa(0) + "_" + ntoa(dest) + "(" +
                                ntoa(0) + ")");

                tcpSnk->setName("mtcp_sink_" + ntoa(src) + "_" + ntoa(0) + "_" + ntoa(dest) + "(" +
                                ntoa(0) + ")");

                tcpRtxScanner.registerTcp(*tcpSrc);


#if PRINT_PATHS
                pathFile << "Coflow:" << coflow_id << ":" << subFlowID;
                pathFile << " Route from " << ntoa(src) << " to " << ntoa(dest) << " -> ";
                top->printPath(pathFile, path.first);
#endif

                routeout = new route_t(*(path.first));
                routeout->push_back(tcpSnk);

                routein = new route_t(*(path.second));

                routein->push_back(tcpSrc);

                if(failedLinkId >=0 && failedLinkId < 3*K*K*K/4 && linkFailureEvent->isPathOverlapping(routeout)){
                    linkFailureEvent->registerConnection(new FlowConnection(tcpSrc, tcpSnk, src,dest));
                }

                tcpSrc->connect(*routeout, *routein, *tcpSnk, timeFromMs(arrivalTime_ms));
                path.first->clear();
                path.second->clear();

#ifdef PACKET_SCATTER
                tcpSrc->set_paths(net_paths[src][dest]);
                cout << "Using PACKET SCATTER!!!!"<<endl<<end;
#endif

            }
        }
    }

    cout << "Total Number of TCP flows:" << connID << endl;
    tcpRtxScanner.StartFrom(timeFromMs(simStartingTime_ms));
    if(failedLinkId >=0 && failedLinkId < 3*K*K*K/4) {
        linkFailureEvent->installEvent();
    }
    // GO!
    while (eventlist.doNextEvent()) {

    }
    double elapsed_secs = double(clock() - begin) / CLOCKS_PER_SEC;
    cout << "Elapsed:" << elapsed_secs << "s" << endl;
    double sum = 0;

    for (pair<int,FlowConnection*> it: *flowStats) {
        sum += it.second->_duration;
    }

    if(topology == 0){
        cout<< "Using FatTree Topology ";
    }else if(topology == 1){
        cout<<"Using ShareBackup Toplogy ";
    }else{
        cout<<"Using F10 Topology ";
    }
    if (routing == 0) {
        cout << "Using ECMP Routing" << endl;
    } else {
        cout << "Using Standard Routing" << endl;
    }
    cout<<"LinkFailure:"<<failedLinkId<<endl;
    cout << "finished flows:" << flowStats->size() << " all flows:" << connID << endl;
    cout << "Average coFCT:" << sum / flowStats->size() << endl;
    cout<< "Total TCP timeouts: "<<eventlist.globalTimeOuts<<endl;
    //cout << "PacketLoss Random:"<<eventlist.randomPacketLoss<<" BufferOverflow:"<<eventlist.bufferOverflowPacketDrops
    //     <<" linkFailure:"<<eventlist.linkFailurePacketDrops<<" ack:"<<eventlist.ackLinkFailureLoss
     //    <<" total:"<< eventlist.getTotalPacketLoss() << endl;
    double temp = linkFailureEvent->getThroughputOfImpactedFlows(flowStats);
    cout<<"average cct of failure flows:"<<temp<<endl;
}


