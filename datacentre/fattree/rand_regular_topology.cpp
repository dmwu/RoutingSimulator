#include "rand_regular_topology.h"
#include <vector>
#include <fstream>
#include <string>
//#include "string.h"
#include <sstream>
#include <strstream>
#include <iostream>
#include "main.h"
#include "YenTopKShortestPathsAlg.h"

extern int RTT;

string ntoa(double n);
string itoa(uint64_t n);

extern int N;

RandRegularTopology::RandRegularTopology(Logfile* lg, EventList* ev,FirstFit * fit, string graphFile){
  logfile = lg;
  eventlist = ev;
  ff = fit;
  
  for (int i=0; i < NSW; i++)
    adjMatrix[i] = new vector<int>();

  //< read graph from the graphFile
  ifstream myfile(graphFile.c_str());
  string line;
  if (myfile.is_open()){
    while(myfile.good()){
	getline(myfile, line);
	if (line.find("->") == string::npos) break;
	int from = atoi(line.substr(0, line.find("->")).c_str());
	int to = atoi(line.substr(line.find("->") + 2).c_str());

	adjMatrix[from]->push_back(to);
	adjMatrix[to]->push_back(from);
    }
    myfile.close();
  }
  //>

  // Ankit: Check adjacency matrix
  /*for (unsigned int i = 0; i < NSW; i++){
    cout << "SW " << i << " NBRS = ";
    for (unsigned int j = 0; j < adjMatrix[i]->size(); j++)
      cout << (*adjMatrix[i])[j] << " ";
    cout << endl;
  }*/

  //< initialize a graph data structure for shortest paths algo
  string command = "sed 's/->/ /g' " + graphFile + " > temp_graph";
  int sysReturn = system(command.c_str());
  if (sysReturn != 0) {
  	  cout << "ERROR: System command to process graph file failed" << endl;
  }
  myGraph = new Graph("temp_graph");
  //>

  init_network();
}

void RandRegularTopology::init_network(){
  QueueLoggerSampling* queueLogger;

  // sw-svr
  for (int j=0;j<NSW;j++)
    for (int k=0;k<SVRPORTS;k++){
      queues_sw_svr[j][k] = NULL;
      pipes_sw_svr[j][k] = NULL;
      queues_svr_sw[j][k] = NULL;
      pipes_svr_sw[j][k] = NULL;
    }
  
  // sw-sw
  for (int j=0;j<NSW;j++)
    for (int k=0;k<NSW;k++){
      queues_sw_sw[j][k] = NULL;
      pipes_sw_sw[j][k] = NULL;
    }


    // sw-svr
    for (int j = 0; j < NSW; j++) {
	int nsvrports = K - adjMatrix[j]->size();

        for (int k = 0; k < nsvrports; k++) {
          // Downlink: sw to server = sw-svr
	  queueLogger = new QueueLoggerSampling(timeFromMs(1000), *eventlist);
	  logfile->addLogger(*queueLogger);

	  queues_sw_svr[j][k] = new RandomQueue(speedFromPktps(HOST_NIC), memFromPkt(SWITCH_BUFFER + RANDOM_BUFFER), *eventlist, queueLogger, memFromPkt(RANDOM_BUFFER));
	  queues_sw_svr[j][k]->setName("SW_" + ntoa(j) + "-" + "DST_" +ntoa(k));
	  logfile->writeName(*(queues_sw_svr[j][k]));
	  
	  pipes_sw_svr[j][k] = new Pipe(timeFromUs(RTT), *eventlist);
	  pipes_sw_svr[j][k]->setName("Pipe-sw-svr-" + ntoa(j) + "-" + ntoa(k));
	  logfile->writeName(*(pipes_sw_svr[j][k]));
	  
	  // Uplink: server to sw = svr-sw
	  queueLogger = new QueueLoggerSampling(timeFromMs(1000), *eventlist);
	  logfile->addLogger(*queueLogger);

	  queues_svr_sw[j][k] = new RandomQueue(speedFromPktps(HOST_NIC), memFromPkt(SWITCH_BUFFER + RANDOM_BUFFER), *eventlist, queueLogger, memFromPkt(RANDOM_BUFFER));
	  queues_svr_sw[j][k]->setName("SRC_" + ntoa(k) + "-" + "SW_" +ntoa(j));
	  logfile->writeName(*(queues_svr_sw[j][k]));
	  
	  pipes_svr_sw[j][k] = new Pipe(timeFromUs(RTT), *eventlist);
	  pipes_svr_sw[j][k]->setName("Pipe-svr-sw-" + ntoa(k) + "-" + ntoa(j));
	  logfile->writeName(*(pipes_svr_sw[j][k]));
	  
	  if (ff){
	    ff->add_queue(queues_sw_svr[j][k]);
	    ff->add_queue(queues_svr_sw[j][k]);
	  }
        }
    }

  // Now use adjMatrix to add pipes etc. between switches
  for (int i = 0; i < NSW; i++) { // over the adjMatrix i.e. different switches
    for (unsigned int j = 0; j < adjMatrix[i]->size(); j++) { // over connections from each switch
	    
      int k = (*adjMatrix[i])[j];
      if ( i > k) continue;
      
      queueLogger = new QueueLoggerSampling(timeFromMs(1000), *eventlist);
      logfile->addLogger(*queueLogger);

      queues_sw_sw[i][k] = new RandomQueue(speedFromPktps(SW_BW), memFromPkt(SWITCH_BUFFER + RANDOM_BUFFER), *eventlist, queueLogger, memFromPkt(RANDOM_BUFFER));
      queues_sw_sw[i][k]->setName("SW_" + ntoa(i) + "-" + "SW_" +ntoa(k));
      logfile->writeName(*(queues_sw_sw[i][k]));

      pipes_sw_sw[i][k] = new Pipe(timeFromUs(RTT), *eventlist);
      pipes_sw_sw[i][k]->setName("Pipe-sw-sw-" + ntoa(i) + "-" + ntoa(k));
      logfile->writeName(*(pipes_sw_sw[i][k]));
	  
      queueLogger = new QueueLoggerSampling(timeFromMs(1000), *eventlist);
      logfile->addLogger(*queueLogger);

      queues_sw_sw[k][i] = new RandomQueue(speedFromPktps(SW_BW), memFromPkt(SWITCH_BUFFER + RANDOM_BUFFER), *eventlist, queueLogger, memFromPkt(RANDOM_BUFFER));
      queues_sw_sw[k][i]->setName("SW_" + ntoa(k) + "-" + "SW_" +ntoa(i));
      logfile->writeName(*(queues_sw_sw[k][i]));
	  
      pipes_sw_sw[k][i] = new Pipe(timeFromUs(RTT), *eventlist);
      pipes_sw_sw[k][i]->setName("Pipe-sw-sw-" + ntoa(k) + "-" + ntoa(i));
      logfile->writeName(*(pipes_sw_sw[k][i]));
	  
      if (ff){
        ff->add_queue(queues_sw_sw[i][k]);
        ff->add_queue(queues_sw_sw[k][i]);
      }
    }
  }
}

void check_non_null(route_t* rt){
  int fail = 0;
  for (unsigned int i=1;i<rt->size()-1;i+=2)
    if (rt->at(i)==NULL){
      fail = 1;
      break;
    }
  
  if (fail){
    //    cout <<"Null queue in route"<<endl;
    for (unsigned int i=1;i<rt->size()-1;i+=2)
      printf("%p ",rt->at(i));

    cout<<endl;
    assert(0);
  }
}

unsigned int RandRegularTopology::ConvertHostToSwitchPort(int host)
{	
  int nsw_less_svrport = NSW - NHOST%NSW;  // The number of switches with less servers than others = (# of hosts % # of switches)
  unsigned int myportindex;
  if(HETERO==1){
	  if(SVRPORTS != 1){
		 int reducedSvrport = SVRPORTS - 1;
		  if(host < nsw_less_svrport * reducedSvrport)
			  myportindex = host % reducedSvrport;
		  else
			  myportindex = (host - nsw_less_svrport * reducedSvrport) % (SVRPORTS);
	  }
	  else {
                  myportindex = 0;
	  }
  }
  else{
	  myportindex = host % SVRPORTS;
  }
  return myportindex;
}



int RandRegularTopology::ConvertHostToSwitch(int host)
{	
  int nsw_less_svrport = NSW - NHOST%NSW;  // The number of switches with less servers than others = (# of hosts % # of switches)
  int myswitchindex;
  if(HETERO==1){
	  if(SVRPORTS != 1){
		 int reducedSvrport = SVRPORTS - 1;
		  if(host < nsw_less_svrport * reducedSvrport)
			  myswitchindex = host / reducedSvrport;
		  else
			  myswitchindex = (host - nsw_less_svrport * reducedSvrport) / (SVRPORTS) + nsw_less_svrport;
	  }
	  else {
		  myswitchindex =  nsw_less_svrport + host;
	  }
  }
  else{
	  myswitchindex = host / SVRPORTS;
  }
  return myswitchindex;
}

vector<route_t*>* RandRegularTopology::get_paths(int src, int dest){
  vector<route_t*>* paths = new vector<route_t*>();

  route_t* routeout;

  //for(int z=0; z<NHOST; z++)
        //cout << "Host " << z << " Connect to switch " << ConvertHostToSwitch(z) << endl;

 


  // Put Yen algorithm's src-dest shortest paths in routeout

  //cout << "HERETO " << HETERO << " NHOST " << NHOST << " NSW " << NSW << " SVRPORTS " << SVRPORTS << endl;

  int src_sw = ConvertHostToSwitch(src);
  int dest_sw = ConvertHostToSwitch(dest);

  // Ankit: Testing if our numbering of switches/servers and topology construction is causing issues
  //cout << " From Switch " << src_sw << " to switch " << dest_sw << endl;

  //< If same switch then only path through switch
  if (src_sw == dest_sw){
    Queue* pqueue = new Queue(speedFromPktps(HOST_NIC), memFromPkt(FEEDER_BUFFER), *eventlist, NULL);
    pqueue->setName("PQueue_" + ntoa(src) + "_" + ntoa(dest));
  
    routeout = new route_t();
    routeout->push_back(pqueue);

    routeout->push_back(queues_svr_sw[src_sw][ConvertHostToSwitchPort(src)]);
    routeout->push_back(pipes_svr_sw[src_sw][ConvertHostToSwitchPort(src)]);

    routeout->push_back(queues_sw_svr[dest_sw][ConvertHostToSwitchPort(dest)]);
    routeout->push_back(pipes_sw_svr[dest_sw][ConvertHostToSwitchPort(dest)]);

    paths->push_back(routeout);

    //cout << "CHECK-NOT-NULL AT SAME SWITCH SERVERS" << endl;
    check_non_null(routeout);
    return paths;
  }
  //>

  else { // Use the shortest path algo to set this stuff up
    YenTopKShortestPathsAlg yenAlg(myGraph, myGraph->get_vertex(src_sw), myGraph->get_vertex(dest_sw));

    int i=0;
    int shortestLen = -1;
    while(yenAlg.has_next() && i < NUMPATHS){
	// Ankit: Checking if YenAlgo gives anything
	//cout << "YEN-ALGO gave some paths" << endl;

	++i;                                                                                                           
	
	vector<BaseVertex*> pathIHave = yenAlg.next()->m_vtVertexList;

//	if (shortestLen == -1) shortestLen = pathIHave.size();
//	if (pathIHave.size() > shortestLen + 1) break;

	Queue* pqueue = new Queue(speedFromPktps(HOST_NIC), memFromPkt(FEEDER_BUFFER), *eventlist, NULL);
	pqueue->setName("PQueue_" + ntoa(src) + "_" + ntoa(dest));

	routeout = new route_t();
	routeout->push_back(pqueue);

	// First hop = svr to sw
	routeout->push_back(queues_svr_sw[src_sw][ConvertHostToSwitchPort(src)]);
	routeout->push_back(pipes_svr_sw[src_sw][ConvertHostToSwitchPort(src)]);

	// Ankit: Print path given by Yen algo
	//cout << "YEN PATH NEW = ";
	for (unsigned int hop = 1; hop < pathIHave.size(); hop++) {
		int fr = pathIHave[hop-1]->getID();
		int to = pathIHave[hop]->getID();
		
	//	cout << fr << " -> " << to << " -> ";
		
		routeout->push_back(queues_sw_sw[fr][to]);
	//	cout << "(Converted = " << queues_sw_sw[fr][to]->str() << ")";
		routeout->push_back(pipes_sw_sw[fr][to]);
	}
	//cout << endl;

	routeout->push_back(queues_sw_svr[dest_sw][ConvertHostToSwitchPort(dest)]);
	routeout->push_back(pipes_sw_svr[dest_sw][ConvertHostToSwitchPort(dest)]);

	paths->push_back(routeout);
	//cout << "CHECK-NOT-NULL AT DIFFERENT SWITCH" << endl;
	check_non_null(routeout);
    }
    
    yenAlg.clear();
    return paths;
  }
}

void RandRegularTopology::count_queue(RandomQueue* queue){
  if (_link_usage.find(queue)==_link_usage.end()){
    _link_usage[queue] = 0;
  }

  _link_usage[queue] = _link_usage[queue] + 1;
}

/*
int RandRegularTopology::find_destination(RandomQueue* queue){
  //first check nlp_ns
  for (int i=0;i<NK;i++)
    for (int j = 0;j<NSRV;j++)
      if (queues_nlp_ns[i][j]==queue)
	return j;

  return -1;
}

void RandRegularTopology::print_path(std::ofstream &paths,int src,route_t* route){
  paths << "SRC_" << src << " ";
  
  if (route->size()/2==2){
    paths << "LS_" << find_lp_switch((RandomQueue*)route->at(1)) << " ";
    paths << "DST_" << find_destination((RandomQueue*)route->at(3)) << " ";
  } else if (route->size()/2==4){
    paths << "LS_" << find_lp_switch((RandomQueue*)route->at(1)) << " ";
    paths << "US_" << find_up_switch((RandomQueue*)route->at(3)) << " ";
    paths << "LS_" << find_lp_switch((RandomQueue*)route->at(5)) << " ";
    paths << "DST_" << find_destination((RandomQueue*)route->at(7)) << " ";
  } else if (route->size()/2==6){
    paths << "LS_" << find_lp_switch((RandomQueue*)route->at(1)) << " ";
    paths << "US_" << find_up_switch((RandomQueue*)route->at(3)) << " ";
    paths << "CS_" << find_core_switch((RandomQueue*)route->at(5)) << " ";
    paths << "US_" << find_up_switch((RandomQueue*)route->at(7)) << " ";
    paths << "LS_" << find_lp_switch((RandomQueue*)route->at(9)) << " ";
    paths << "DST_" << find_destination((RandomQueue*)route->at(11)) << " ";
  } else {
    paths << "Wrong hop count " << ntoa(route->size()/2);
  }
  
  paths << endl;
}
*/
