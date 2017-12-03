#include <sstream>
#include <strstream>
#include <iostream>
#include <string.h>
#include <unistd.h>
#include <list>
#include <math.h>
#include <set>
#include <MultipleSteadyLinkFailures.h>
#include "SingleDynamicLinkFailureEvent.h"
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

map<int, double>* getCoflowStats(map<int,FlowConnection*>* flowStats){
    map<int, double>* cct = new map<int, double>();
    for(pair<int,FlowConnection*> it: *flowStats){
        int cid = it.second->_coflowId;
        int duration = it.second->_duration_ms;
        if(cct->count(cid)==0 || cct->at(cid) < duration)
            (*cct)[cid] = duration;
    }
    return cct;
}

string getCCTFileName(int topology, int routing, int link, string trace){
    stringstream file;
    file<<"top"<<itoa(topology) <<"rtng"<<itoa(routing)<<"K"<< K <<"links"<<itoa(link)<<trace<<".txt";
    return file.str();
}
EventList eventlist;

Logfile *lg;
map<int, double>* getCoflowStats(map<int,FlowConnection*>* flowStats);
void exit_error(char *progr) {
    cout << "Usage " << progr
         << " [UNCOUPLED(DEFAULT)|COUPLED_INC|FULLY_COUPLED|COUPLED_EPSILON] [epsilon][COUPLED_SCALABLE_TCP" << endl;
    exit(1);
}

int main(int argc, char **argv) {
    clock_t begin = clock();
    int totalFlows = 0;
    eventlist.setEndtime(timeFromSec(2000.01));
    set<int>* impactedCoflow = new set<int>();
    set<int>* impactedFlow = new set<int>();
    set<int>* deadCoflow = new set<int>();
    set<int>* deadFlow = new set<int>();

    double simStartingTime_ms = -1;
    double epsilon = 1;
    int routing = 0; //[WDM] 0 stands for ecmp; 1 stands for two-level routing
    int topology = 0; // 0 stands for fattree; 1 stands for sharebackup; 2 stands for f10
    int failedLinkId = -1;
    string traf_file_name;
    int steadyOnly = 0;
    if (argc > 1) {
        int i = 1;
        if (argc > i && !strcmp(argv[i], "-topo")) {
            topology = atoi(argv[i + 1]);
            i += 2;
        }
        if (argc > i && !strcmp(argv[i], "-routing")) {
            routing = atoi(argv[i + 1]);
            i += 2;
        }
        if (argc > i && !strcmp(argv[i], "-failLinkId")) {
            failedLinkId = atoi(argv[i + 1]);
            i += 2;
        }

        if (argc > i && !strcmp(argv[i], "-steadyFailure")) {
            steadyOnly= atoi(argv[i + 1]);
            i += 2;
        }
        traf_file_name = argv[i];
    } else {
        cout << "wrong arguments!" << endl;
        exit(1);
    }

#if PRINT_PATHS
    filename << "logs.paths";
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
    Topology *top = NULL;

    SingleDynamicLinkFailureEvent *linkFailureEvent;
    if (topology <= 1) {
        top = new FatTreeTopology(&eventlist);
        linkFailureEvent = new SingleDynamicLinkFailureEvent(eventlist, top, timeFromSec(0), timeFromSec(300),
                                                             failedLinkId);
        linkFailureEvent->setFailureRecoveryDelay(timeFromMs(GLOBAL_REROUTE_DELAY),
                                                  timeFromMs(GLOBAL_REROUTE_DELAY));
        if (topology == 1) {
            linkFailureEvent->UsingShareBackup = true;
            vector<int> *lpBackupUsageTracker = new vector<int>(K,BACKUPS_PER_GROUP);
            vector<int> *upBackupUsageTracker = new vector<int>(K,BACKUPS_PER_GROUP);
            vector<int> *coreBackupUsageTracker = new vector<int>(K/2, BACKUPS_PER_GROUP);
            linkFailureEvent->setFailureRecoveryDelay(timeFromMs(CIRCUIT_SWITCHING_DELAY), 0);
            linkFailureEvent->setBackupUsageTracker(lpBackupUsageTracker, upBackupUsageTracker,
                                                    coreBackupUsageTracker);
        }
    } else {
        top = new F10Topology(&eventlist);
        linkFailureEvent = new SingleDynamicLinkFailureEvent(eventlist, top, timeFromSec(0), timeFromSec(300),
                                                             failedLinkId);
        linkFailureEvent->setFailureRecoveryDelay(timeFromMs(LOCAL_REROUTE_DELAY), timeFromMs(LOCAL_REROUTE_DELAY));
    }


    int flowId = 0;
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
    vector<bool*> flows_finish;
    map<int,FlowConnection*>* flowStats = new map<int, FlowConnection*>();

    int src, dest;
    route_t *path = NULL;
    srand(time(NULL));
    string cctFilename = getCCTFileName(topology,routing,failedLinkId,traf_file_name);
    while (getline(traf_file, line)) {
        int i = 0;
        int pos = line.find(" ");
        int coflow_id = atoi(line.substr(0, pos).c_str());
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
        totalFlows += mapper_n*reducer_n;
        for (int k = 0; k < reducer_n; k++) {
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
                    path = top->getEcmpPath(src, dest);

                } else {
                    path = top->getStandardPath(src,dest);
                }

                if (!top->isPathValid(path.first) || !top->isPathValid(path.second)) {
                    impactedFlow->insert(flowId);
                    impactedCoflow->insert(coflow_id);
                    route_t* curPath = path.first;
                    path = top->getReroutingPath(src, dest, curPath);

                    if(!top->isPathValid(path.first) || !top->isPathValid(path.second)){
                        cout<<"no path available for:"<<src<<"->"<<dest<<endl;
                        deadCoflow->insert(coflow_id);
                        deadFlow->insert(flowId);
                        flowId++;
                        continue;
                    }
                }
                tcpSrc = new TcpSrc(src, dest, eventlist, volume_bytes, arrivalTime_ms, flowId,
                                    coflow_id, flowStats);
                flowId++;
                tcpSnk = new TcpSink(&eventlist);
                // yiting
                tcpSnk->set_super_id(flowId);

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

            }
        }
    }

    cout << "Total Number of TCP flows:" << flowId << endl;
    tcpRtxScanner.StartFrom(timeFromMs(simStartingTime_ms));
    if(failedLinkId >=0 && failedLinkId < 3*K*K*K/4) {
        linkFailureEvent->installEvent();
    }
    // GO!
    while (eventlist.doNextEvent()) {

    }
    double fsum = 0,csum = 0;

    for (pair<int,FlowConnection*> it: *flowStats) {
        fsum += it.second->_duration_ms;
    }
    map<int, double>* cct = getCoflowStats(flowStats);

    for(pair<int,double> it:*cct){
        if(deadCoflow->count(it.first)==0)
            csum += it.second;
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
    cout <<"Finished flows:" << flowStats->size() << " all flows:" << totalFlows << endl;
    cout<<"Finished coflows:"<<cct->size()<<" all coflows:"<<coflowNum<<endl;
    cout <<"Average FCT:" << fsum / flowStats->size() << endl;
    cout <<"Average CCT:"<<csum/cct->size()<<endl;
    cout<<"Num of Impacted Flows:"<<impactedFlow->size()<<endl;
    cout<< "Num of Impacted Coflows:"<<impactedCoflow->size()<<endl;
    cout<<"Num of Dead Flows:"<<deadFlow->size()<<endl;
    cout<<"Num of Dead Coflows:"<<deadCoflow->size()<<endl;
    double elapsed_secs = double(clock() - begin) / CLOCKS_PER_SEC;
    cout << "Elapsed:" << elapsed_secs << "s" << endl;
}
