#ifndef PIPE_H
#define PIPE_H

/*
 * A pipe is a dumb device which simply delays all incoming packets
 */

#include <list>
#include <utility>
#include "config.h"
#include "eventlist.h"
#include "network.h"
#include "loggertypes.h"


class Pipe : public EventSource, public PacketSink {
public:
    Pipe(simtime_picosec delay, EventList &eventlist);

    Pipe(simtime_picosec delay, EventList &eventlist, uint32_t gid);

    void receivePacket(Packet &pkt); // inherited from PacketSink
    void doNextEvent(); // inherited from EventSource
    simtime_picosec delay() { return _delay; }

    uint32_t _gid;
private:
    simtime_picosec _delay;
    typedef pair<simtime_picosec, Packet *> pktrecord_t;
    list<pktrecord_t> _inflight; // the packets in flight (or being serialized)
};


#endif
