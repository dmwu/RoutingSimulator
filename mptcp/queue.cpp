#include "queue.h"
#include <iostream>
#include <math.h>

Queue::Queue(linkspeed_bps bitrate, mem_b maxsize, EventList &eventlist, QueueLogger *logger)
        : EventSource(eventlist, "queue"),
          _maxsize(maxsize), _logger(logger), _bitrate(bitrate) {
    _queuesize = 0;
    _ps_per_byte = (simtime_picosec) ((pow(10.0, 12.0) * 8) / _bitrate);
}

Queue::Queue(linkspeed_bps bitrate, mem_b maxsize, EventList &eventlist, QueueLogger *logger, string gid, int sid)
        : EventSource(eventlist, "queue"),
          _maxsize(maxsize), _logger(logger), _bitrate(bitrate) {
    _queuesize = 0;
    _ps_per_byte = (simtime_picosec) ((pow(10.0, 12.0) * 8) / _bitrate);
    _gid = gid;
    _switchId = sid;
}

void Queue::beginService() {
    assert(!_enqueued.empty());
    int delay = 0;
    if(!_isHostQueue){
        delay = drainTime(_enqueued.back());
    }
    eventlist().sourceIsPendingRel(*this, delay);
}

void Queue::completeService() {
    if (_enqueued.empty()) {
        std::cout << "empty queue:" << this->str() << std::endl;
        exit(1);
    }
    Packet *pkt = _enqueued.back();
    _enqueued.pop_back();

    _queuesize -= pkt->size();

    if(this->_gid=="Queue-up-lp-3-3"){
       // cout<<"[Packet Depart]:"<<pkt->_src<<"->"<<pkt->_dest<<" at "<<eventlist().now()/1e6<<"us"<<endl;
    }
    pkt->flow().logTraffic(*pkt, *this, TrafficLogger::PKT_DEPART);
    if (_logger)
        _logger->logQueue(*this, QueueLogger::PKT_SERVICE, *pkt);
    pkt->sendOn();
    if (!_enqueued.empty())
        beginService();
}

void Queue::doNextEvent() {
    completeService();
}


void Queue::receivePacket(Packet &pkt) {
    if (_disabled || _queuesize + pkt.size() > _maxsize) {
        if (_logger)
            _logger->logQueue(*this, QueueLogger::PKT_DROP, pkt);
        pkt.flow().logTraffic(pkt, *this, TrafficLogger::PKT_DROP);
        pkt.free();
        cout<<"[Packet Drop]"<<pkt.flow().str()<<" at "<<this->str()<<endl;
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
