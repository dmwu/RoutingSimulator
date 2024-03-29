#include "exoqueue.h"
#include <math.h>

ExoQueue::ExoQueue(double loss_rate)
  : _loss_rate(loss_rate){
}

void 
ExoQueue::setLossRate(double l){
  _loss_rate = l;
}

void
ExoQueue::receivePacket(Packet& pkt) 
{
  if (drand()<_loss_rate){
    pkt.free();
    return;
  }
  else 
  pkt.sendOn();
}
