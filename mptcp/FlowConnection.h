//
// Created by Dingming Wu on 1/16/18.
//

#ifndef SIM_FLOWCONNECTION_H
#define SIM_FLOWCONNECTION_H
#include "tcp.h"

class FlowConnection{
public:
    FlowConnection(TcpSrc* tcpSrc, int superID, int src, int dest, uint32_t fs, uint32_t arrivalTime_ms);
    FlowConnection(TcpSrc*, int, int);
    TcpSrc* _tcpSrc;
    int _src;
    int _dest;
    int _superId;
    int _coflowId;
    double _throughput=-1;
    uint32_t _flowSize_Bytes;
    uint32_t _arrivalTimeMs;
    uint32_t _completionTimeMs=-1;
    uint32_t _duration_ms=-1;

};


#endif //SIM_FLOWCONNECTION_H
