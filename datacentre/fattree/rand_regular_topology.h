#ifndef RAND_REGULAR
#define RAND_REGULAR
#include "main.h"
#include "randomqueue.h"
#include "pipe.h"
#include "config.h"
#include "loggers.h"
#include "network.h"
#include "firstfit.h"
#include "topology.h"
#include "logfile.h"
#include "eventlist.h"
#include "GraphElements.h"
#include "Graph.h"
#include <ostream>

// Hardcoding number of switches, ports for now
#define SVRPORTS (NHOST%NSW==0?NHOST/NSW:NHOST/NSW+1)
#define HETERO (NHOST%NSW==0?0:1)

class RandRegularTopology: public Topology{
 public:
  // sw_sw: lower index to higher index = up link etc.
  Pipe * pipes_sw_sw[NSW][NSW];
  RandomQueue * queues_sw_sw[NSW][NSW];
  
  // These are for switch to server "down" links
  Pipe * pipes_sw_svr[NSW][SVRPORTS];
  RandomQueue * queues_sw_svr[NSW][SVRPORTS];

  // These are for svr to switch "up" links
  Pipe * pipes_svr_sw[NSW][SVRPORTS];
  RandomQueue * queues_svr_sw[NSW][SVRPORTS];

  FirstFit * ff;
  Logfile* logfile;
  EventList* eventlist;
  
  RandRegularTopology(Logfile* log,EventList* ev,FirstFit* f, string graphFile);

  void init_network();
  virtual vector<route_t*>* get_paths(int src, int dest);

  void count_queue(RandomQueue*);
//  void print_path(std::ofstream& paths,int src,route_t* route);
  vector<int>* get_neighbours(int src) { return NULL;};
 private:

  Graph* myGraph;
  map<RandomQueue*,int> _link_usage;
  vector<int>* adjMatrix[NSW];
  
  int find_switch(RandomQueue* queue);
  int find_destination(RandomQueue* queue);
  int ConvertHostToSwitch(int host);
  unsigned int ConvertHostToSwitchPort(int host);
};

#endif

