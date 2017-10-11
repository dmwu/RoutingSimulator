CC = g++
CFLAGS = -O6 -Wall -g -Wno-deprecated -Wno-unused-variable
CRT=`pwd`
INCLUDE= -I/$(CRT)/.. -I$(CRT) 
#-I$(CRT)/ksp -I$(CRT)/ksp/boost
LIB=-L.. 
#-Lksp

#all:	fattree rrg
all: clos

#
clos: main.o firstfit.o ../libhtsim.a fat_tree_topology.o subflow_control.o connection_matrix.o shortflows.o star_topology.o Graph.o YenTopKShortestPathsAlg.o DijkstraShortestPathAlg.o
	$(CC) $(CFLAGS) firstfit.o main.o fat_tree_topology.o subflow_control.o connection_matrix.o shortflows.o star_topology.o Graph.o YenTopKShortestPathsAlg.o DijkstraShortestPathAlg.o $(LIB) -lhtsim -o clos

main.o: main.cpp main.h
	$(CC) $(INCLUDE) $(CFLAGS) -c main.cpp 

star_topology.o: star_topology.cpp star_topology.h  main.h
	$(CC) $(INCLUDE) $(CFLAGS) -c star_topology.cpp 

shortflows.o: shortflows.cpp  main.h
	$(CC) $(INCLUDE) $(CFLAGS) -c shortflows.cpp 

connection_matrix.o: connection_matrix.cpp main.h 
	$(CC) $(INCLUDE) $(CFLAGS) -c connection_matrix.cpp 

firstfit.o: firstfit.cpp  main.h
	$(CC) $(INCLUDE) $(CFLAGS) -c firstfit.cpp

subflow_control.o: subflow_control.cpp subflow_control.h  main.h
	$(CC) $(INCLUDE) $(CFLAGS) -c subflow_control.cpp

fat_tree_topology.o: fat_tree_topology.cpp fat_tree_topology.h topology.h  main.h
	$(CC) $(INCLUDE) $(CFLAGS) -c fat_tree_topology.cpp

rand_regular_topology.o: rand_regular_topology.cpp rand_regular_topology.h topology.h Graph.h GraphElements.h main.h
	$(CC) $(INCLUDE) $(CFLAGS) -c rand_regular_topology.cpp

DijkstraShortestPathAlg.o: DijkstraShortestPathAlg.cpp DijkstraShortestPathAlg.h
	$(CC) $(INCLUDE) $(CFLAGS) -c DijkstraShortestPathAlg.cpp

Graph.o: Graph.cpp Graph.h GraphElements.h main.h
	$(CC) $(INCLUDE) $(CFLAGS) -c Graph.cpp

YenTopKShortestPathsAlg.o: YenTopKShortestPathsAlg.cpp YenTopKShortestPathsAlg.h
	$(CC) $(INCLUDE) $(CFLAGS) -c YenTopKShortestPathsAlg.cpp

clean:	
	rm *.o
