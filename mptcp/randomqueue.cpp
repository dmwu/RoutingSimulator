#include "randomqueue.h"
#include <math.h>
#include <iostream>

RandomQueue::RandomQueue(linkspeed_bps bitrate, mem_b maxsize, EventList &eventlist, QueueLogger *logger, mem_b drop)
        : Queue(bitrate, maxsize, eventlist, logger),
          _drop(drop),
          _buffer_drops(0) {
    _drop_th = _maxsize - _drop;
    _plr = 0.0;
}

RandomQueue::RandomQueue(linkspeed_bps bitrate, mem_b maxsize, EventList &eventlist,
                         QueueLogger *logger, mem_b drop, string gid, int sid)
        : Queue(bitrate, maxsize, eventlist, logger),
          _drop(drop),
          _buffer_drops(0) {
    _drop_th = _maxsize - _drop;
    _plr = 0.0;
    _gid= gid;
    _switchId = sid;
}

void RandomQueue::set_packet_loss_rate(double l) {
    _plr = l;
}

void RandomQueue::receivePacket(Packet &pkt) {

    if (_disabled){
        this->eventlist().globalPacketDrops++;
        pkt.free();
        return;
    }

    int crt = _queuesize + pkt.size();
    if (_plr > 0.0 && drand() < _plr) {
        //if (_logger) _logger->logQueue(*this, QueueLogger::PKT_DROP, pkt);
        //pkt.flow().logTraffic(pkt,*this,TrafficLogger::PKT_DROP);
        pkt.free();
        this->eventlist().globalPacketDrops++;
        return;
    }

    //[WDM] let's make everything deterministic
    if (crt > _maxsize) {
        if (_logger) _logger->logQueue(*this, QueueLogger::PKT_DROP, pkt);
        pkt.flow().logTraffic(pkt, *this, TrafficLogger::PKT_DROP);
        if (crt > _maxsize) {
            _buffer_drops++;
        }
        pkt.free();
        this->eventlist().globalPacketDrops++;
        return;
    }
    pkt.flow().logTraffic(pkt, *this, TrafficLogger::PKT_ARRIVE);
    bool queueWasEmpty = _enqueued.empty();
    _enqueued.push_front(&pkt);
    _queuesize += pkt.size();
    if (_logger)
        _logger->logQueue(*this, QueueLogger::PKT_ENQUEUE, pkt);
    if (queueWasEmpty) {
        assert(_enqueued.size() == 1);
        beginService();
    }
}
