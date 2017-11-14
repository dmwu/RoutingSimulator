#include "tcp.h"
#include "mtcp.h"
#include "topology.h"
#include <iostream>

////////////////////////////////////////////////////////////////
//  TCP SOURCE
////////////////////////////////////////////////////////////////

TcpSrc::TcpSrc(TcpLogger *logger, TrafficLogger *pktlogger,
               EventList &eventlist, map<int, double>*flowStats)
        : EventSource(eventlist, "tcp"), _logger(logger), _flow(pktlogger) {
    _mss = TcpPacket::DEFAULTDATASIZE;
    _maxcwnd = 0xffffffff;//MAX_SENT*_mss;
    _sawtooth = 0;
    _rtt_avg = timeFromMs(0);
    _rtt_cum = timeFromMs(0);
    _highest_sent = 0; // [WDM]the last byte index of the highest packet (not sure)
    _packets_sent = 0;
    _app_limited = -1;
    _effcwnd = 0;
    _ssthresh = 0xffffffff;
    _last_acked = 0;
    _last_ping = timeInf;
    _dupacks = 0;
    _rtt = 0;
    _rto = timeFromMs(3000);
    _mdev = 0;
    _recoverq = 0;
    _in_fast_recovery = false;
    _mSrc = NULL;
    _drops = 0;

    // yiting
    uint64_t tmp_sent = 0;
    uint64_t tmp_received = 0;
    bool tmp_finish = false;

    _flow_total_sent = &tmp_sent;
    _flow_total_received = &tmp_received;
    _flow_volume_bytes = 0;
    _flow_finish = &tmp_finish;
    _flow_start_time_ms = 0;
    _super_id = -1;
    _flowStats = flowStats;

#ifdef PACKET_SCATTER
    _crt_path = 0;
    DUPACK_TH = 3;
    _paths = NULL;
#endif

    _old_route = NULL;
    _last_packet_with_old_route = 0;

    _rtx_timeout_pending = false;
    _RFC2988_RTO_timeout = timeInf;
}

TcpSrc::TcpSrc(TcpLogger *logger, TrafficLogger *pktlogger, EventList &eventlist)
        : EventSource(eventlist, "tcp"), _logger(logger), _flow(pktlogger) {
    TcpSrc(logger, pktlogger, eventlist, NULL);
}


TcpSrc::TcpSrc(TcpLogger *logger, TrafficLogger *pktlogger,
               EventList &eventlist, uint64_t *total_sent, uint64_t *total_received,
               uint64_t volume, bool *finish, double startTime_ms, int super_id, int coflowId,
               map<int, double>*flowStats)
        : EventSource(eventlist, "tcp"), _logger(logger), _flow(pktlogger) {
    _mss = TcpPacket::DEFAULTDATASIZE;
    _maxcwnd = 0xffffffff;//MAX_SENT*_mss;
    _sawtooth = 0;
    _rtt_avg = timeFromMs(0);
    _rtt_cum = timeFromMs(0);
    _highest_sent = 0;
    _packets_sent = 0;
    _app_limited = -1;
    _effcwnd = 0;
    _ssthresh = 0xffffffff;
    _last_acked = 0;
    _last_ping = timeInf;
    _dupacks = 0;
    _rtt = 0;
    _rto = timeFromMs(3000);
    _mdev = 0;
    _recoverq = 0;
    _in_fast_recovery = false;
    _mSrc = NULL;
    _drops = 0;
    _flowStats = flowStats;
    // yiting
    _flow_total_sent = total_sent;
    _flow_total_received = total_received;
    _flow_volume_bytes = volume;
    _flow_finish = finish;
    _flow_start_time_ms = startTime_ms;
    _super_id = super_id;
    _coflowID = coflowId;

#ifdef PACKET_SCATTER
    _crt_path = 0;
    DUPACK_TH = 3;
    _paths = NULL;
#endif

    _old_route = NULL;
    _last_packet_with_old_route = 0;

    _rtx_timeout_pending = false;
    _RFC2988_RTO_timeout = timeInf;
}

#ifdef PACKET_SCATTER
void TcpSrc::set_paths(vector<route_t*>* rt){
  //this should only be used with route
  _paths = new vector<route_t*>();

  for (unsigned int i=0;i<rt->size();i++){
    route_t* t = new route_t(*(rt->at(i)));
    t->push_back(_sink);
    _paths->push_back(t);
  }
  DUPACK_TH = 3 + rt->size();
}
#endif

void TcpSrc::set_app_limit(int pktps) {
    if (_app_limited == 0 && pktps) {
        _cwnd = _mss;
    }
    _ssthresh = 0xffffffff;
    _app_limited = pktps;
    send_packets();
}

void TcpSrc::startflow() {
    _cwnd = _mss;
    _unacked = _cwnd;
    send_packets();
}

uint32_t TcpSrc::effective_window() {
    return _in_fast_recovery ? _ssthresh : _cwnd;
}

void TcpSrc::replace_route(route_t *newroute) {

    //[WDM] newRoute doesn't contain the sink
    _old_route = _route;
    _route = newroute;
    _route->push_back(_sink);
    _last_packet_with_old_route = _highest_sent;
    _last_ping = timeInf;

    //  printf("Wiating for ack %d to delete\n",_last_packet_with_old_route);
}

void TcpSrc::connect(route_t &routeout, route_t &routeback, TcpSink &sink, simtime_picosec startTime_ps) {
    _route = &routeout;

    assert(_route);
    _sink = &sink;
    _flow.id = id; // identify the packet flow with the TCP source that generated it
    _sink->connect(*this, routeback);
    eventlist().sourceIsPending(*this, startTime_ps);
}

#define ABS(X) ((X)>0?(X):-(X))

void
TcpSrc::receivePacket(Packet &pkt) {
    simtime_picosec ts;
    TcpAck *p = (TcpAck *) (&pkt);
    TcpAck::seq_t seqno = p->ackno();
    pkt.flow().logTraffic(pkt, *this, TrafficLogger::PKT_RCVDESTROY);

    ts = p->ts();
    p->free();

    if (seqno < _last_acked) {
        cout << "O seqno" << seqno << " last acked " << _last_acked;
        return;
    }

    assert(seqno >= _last_acked);  // no dups or reordering allowed in this simple simulator

    // yiting
    *_flow_total_received += (seqno - _last_acked);
    if (_flow_volume_bytes != 0 && *_flow_total_received >= _flow_volume_bytes && *_flow_finish == false) {
        *_flow_finish = true;
        double comp_time_ms = (eventlist().now() / 1e9) - _flow_start_time_ms;
        double rate = (_flow_volume_bytes / 1e6) / (comp_time_ms / 1e3);
        cout << "coflow:"<<_coflowID <<" "<<(*_route)[0]->_gid<<"->"<<(*_route)[_route->size()-2]->_gid <<" "<<
             _super_id << " compTime(ms):" << comp_time_ms<< " now(ms):" << (double)eventlist().now()/1e9 << " rate(MB/s):" << rate << endl;
        _flowStats->insert(pair<int,double>(_super_id, comp_time_ms));
        Topology::printPath(cout, _route);
        return;
    }

    //compute rtt
    uint64_t m = eventlist().now() - ts;

    if (m != 0) {
        if (_rtt > 0) {
            uint64_t abs;
            if (m > _rtt)
                abs = m - _rtt;
            else
                abs = _rtt - m;

            _mdev = 3 * _mdev / 4 + abs / 4;
            _rtt = 7 * _rtt / 8 + m / 8;

            _rto = _rtt + 4 * _mdev;
        } else {
            _rtt = m;
            _mdev = m / 2;

            _rto = _rtt + 4 * _mdev;
        }
    }

    if (_rto < timeFromMs(1))
        _rto = timeFromMs(1);

    if (seqno > _last_acked) { // a brand new ack
        if (_old_route) {
            if (seqno >= _last_packet_with_old_route) {
                //delete _old_route;
                _old_route = NULL;
            }
        }

        _RFC2988_RTO_timeout = eventlist().now() + _rto;// RFC 2988 5.3
        _last_ping = eventlist().now();

        if (seqno == _highest_sent) {
            _RFC2988_RTO_timeout = timeInf;// RFC 2988 5.2
            _last_ping = timeInf;
        }

        if (!_in_fast_recovery) { // best behaviour: proper ack of a new packet, when we were expecting it
            //clear timers
            _last_acked = seqno;
            _dupacks = 0;
            inflate_window();
            _unacked = _cwnd;
            _effcwnd = _cwnd;
            if (_logger) _logger->logTcp(*this, TcpLogger::TCP_RCV);
            send_packets();
            return;
        }
        // We're in fast recovery, i.e. one packet has been
        // dropped but we're pretending it's not serious
        if (seqno >= _recoverq) {
            // got ACKs for all the "recovery window": resume
            // normal service
            uint32_t flightsize = _highest_sent - seqno;
            _cwnd = min(_ssthresh, flightsize + _mss);
            _unacked = _cwnd;
            _effcwnd = _cwnd;
            _last_acked = seqno;
            _dupacks = 0;
            _in_fast_recovery = false;
            if (_logger) _logger->logTcp(*this, TcpLogger::TCP_RCV_FR_END);
            send_packets();
            return;
        }
        // In fast recovery, and still getting ACKs for the
        // "recovery window"
        // This is dangerous. It means that several packets
        // got lost, not just the one that triggered FR.
        uint32_t new_data = seqno - _last_acked;
        _last_acked = seqno;
        if (new_data < _cwnd)
            _cwnd -= new_data;
        else
            _cwnd = 0;

        _cwnd += _mss;
        if (_logger) _logger->logTcp(*this, TcpLogger::TCP_RCV_FR);
        retransmit_packet();
        send_packets();
        return;
    }
    // It's a dup ack
    if (_in_fast_recovery) { // still in fast recovery; hopefully the prodigal ACK is on it's way
        _cwnd += _mss;
        if (_cwnd > _maxcwnd) {
            _cwnd = _maxcwnd;
        }
        // When we restart, the window will be set to
        // min(_ssthresh, flightsize+_mss), so keep track of
        // this
        _unacked = min(_ssthresh, (uint32_t) (_highest_sent - _recoverq + _mss));
        if (_last_acked + _cwnd >= _highest_sent + _mss) _effcwnd = _unacked; // starting to send packets again
        if (_logger) _logger->logTcp(*this, TcpLogger::TCP_RCV_DUP_FR);
        send_packets();
        return;
    }
    // Not yet in fast recovery. What should we do instead?
    _dupacks++;

#ifdef PACKET_SCATTER
    if (_dupacks!=DUPACK_TH) { //not yet serious worry
#else
    if (_dupacks != 3) { // not yet serious worry
#endif
        if (_logger) _logger->logTcp(*this, TcpLogger::TCP_RCV_DUP);
        send_packets();
        return;
    }
    // _dupacks==3
    if (_last_acked < _recoverq) {  //See RFC 3782: if we haven't
        //recovered from timeouts
        //etc. don't do fast recovery
        if (_logger) _logger->logTcp(*this, TcpLogger::TCP_RCV_3DUPNOFR);
        return;
    }

    // begin fast recovery

    //only count drops in CA state
    _drops++;

    if (_mSrc == NULL)
        _ssthresh = max(_cwnd / 2, (uint32_t) (2 * _mss));
    else {
        _ssthresh = _mSrc->deflate_window(_cwnd, _mss);
    }

    if (_sawtooth > 0)
        _rtt_avg = _rtt_cum / _sawtooth;
    else
        _rtt_avg = timeFromMs(0);

    _sawtooth = 0;
    _rtt_cum = timeFromMs(0);

    retransmit_packet();
    _cwnd = _ssthresh + 3 * _mss;
    _unacked = _ssthresh;
    _effcwnd = 0;
    _in_fast_recovery = true;
    _recoverq = _highest_sent; // _recoverq is the value of the
    // first ACK that tells us things
    // are back on track
    if (_logger)
        _logger->logTcp(*this, TcpLogger::TCP_RCV_DUP_FASTXMIT);
}

void
TcpSrc::inflate_window() {
    int newly_acked = (_last_acked + _cwnd) - _highest_sent;
    // be very conservative - possibly not the best we can do, but
    // the alternative has bad side effects.
    if (newly_acked > _mss) newly_acked = _mss;
    if (newly_acked < 0)
        return;
    if (_cwnd < _ssthresh) { //slow start
        int increase = min(_ssthresh - _cwnd, (uint32_t) newly_acked);
        _cwnd += increase;
        newly_acked -= increase;
    }
        // additive increase
    else {
        uint32_t pkts = _cwnd / _mss;

        if (_mSrc == NULL) {
            _cwnd += (newly_acked * _mss) / _cwnd;  //XXX beware large windows, when this increase gets to be very small

        } else
            _cwnd = _mSrc->inflate_window(_cwnd, newly_acked, _mss);

        if (pkts != _cwnd / _mss) {
            _rtt_cum += _rtt;
            _sawtooth++;
        }
    }
}

// Note: the data sequence number is the number of Byte1 of the packet, not the last byte.
void
TcpSrc::send_packets() {
    //cout << "id: " << _super_id << " volume: " << _flow_volume << " sent: " << *_flow_total_sent << endl;

    int c = _cwnd;
    if (_app_limited >= 0 && _rtt > 0) {
        uint64_t d = (uint64_t) _app_limited * _rtt / 1000000000;
        if (c > d) {
            c = d;
        }
    }

    while (_last_acked + c >= _highest_sent + _mss) {
        if (_flow_volume_bytes != 0 && *_flow_total_sent >= _flow_volume_bytes) {
            return;
        }

#ifdef PACKET_SCATTER
        TcpPacket* p = TcpPacket::newpkt(_flow, *(_paths->at(_crt_path)), _highest_sent+1, _mss);
        _crt_path = (_crt_path + 1) % _paths->size();
#else
        TcpPacket *p = TcpPacket::newpkt(_flow, *_route, _highest_sent + 1, _mss);
#endif
        p->flow().logTraffic(*p, *this, TrafficLogger::PKT_CREATESEND);
        p->set_ts(eventlist().now());

        _highest_sent += _mss;  //XX beware wrapping
        _packets_sent += _mss;
        // yiting
        *_flow_total_sent += _mss;

        p->sendOn();

        if (_RFC2988_RTO_timeout == timeInf) {// RFC2988 5.1
            _RFC2988_RTO_timeout = eventlist().now() + _rto;
        }
    }
}

void
TcpSrc::retransmit_packet() {
    //cout << " RTO " << _rto/1000000000 << " MDEV " << _mdev/1000000000 << " RTT "<< _rtt/1000000000 << " SEQ " << _last_acked / _mss << " CWND "<< _cwnd/_mss << " FAST RECOVERY? " << 	_in_fast_recovery << " Flow ID " << str()  << endl;
#ifdef PACKET_SCATTER
    TcpPacket* p = TcpPacket::newpkt(_flow, *(_paths->at(_crt_path)), _last_acked+1, _mss);
    _crt_path = (_crt_path + 1) % _paths->size();
#else
    TcpPacket *p = TcpPacket::newpkt(_flow, *_route, _last_acked + 1, _mss);
#endif

    p->flow().logTraffic(*p, *this, TrafficLogger::PKT_CREATESEND);
    p->set_ts(eventlist().now());
    p->sendOn();

    _packets_sent += _mss;

    if (_RFC2988_RTO_timeout == timeInf) {// RFC2988 5.1
        _RFC2988_RTO_timeout = eventlist().now() + _rto;
    }
}

void TcpSrc::rtx_timer_hook(simtime_picosec now, simtime_picosec period) {
    if (now <= _RFC2988_RTO_timeout || _RFC2988_RTO_timeout == timeInf) return;

    if (_highest_sent == 0) return;

    //if ((int)(now/(double)1000000000) % 100 < 1) {
    //	  cout <<"At " << now/(double)1000000000<< " RTO " << _rto/1000000000 << " MDEV " << _mdev/1000000000 << " RTT "<< _rtt/1000000000 << " SEQ " << _last_acked / _mss << " CWND "<< _cwnd/_mss << " FAST RECOVERY? " << 	_in_fast_recovery << " Flow ID " << str()  << endl;
    //}
    // here we can run into phase effects because the timer is checked only periodically for ALL flows
    // but if we keep the difference between scanning time and real timeout time when restarting the flows
    // we should minimize them !
    if (!_rtx_timeout_pending) {
        _rtx_timeout_pending = true;
        this->eventlist().globalTimeOuts++;
        // check the timer difference between the event and the real value
        simtime_picosec too_late = now - (_RFC2988_RTO_timeout);
        // careful: we might calculate a negative value if _rto suddenly drops very much
        // to prevent overflow but keep randomness we just divide until we are within the limit
        while (too_late > period) too_late >>= 1;

        // carry over the difference for restarting
        simtime_picosec rtx_off = (period - too_late) / 200;

        //[WDM] since its already late, why not retransmit now
        //eventlist().sourceIsPendingRel(*this, rtx_off);
        eventlist().sourceIsPending(*this, now);

        //reset our rtx timerRFC 2988 5.5 & 5.6

        _rto *= 2;
        if (_rto > timeFromMs(1000))
            _rto = timeFromMs(1000);
        _RFC2988_RTO_timeout = now + _rto;
    }
}

void TcpSrc::doNextEvent() {
    //cout << "id = " << _super_id << endl;
    if (_rtx_timeout_pending) {
        _rtx_timeout_pending = false;

        if (_logger) _logger->logTcp(*this, TcpLogger::TCP_TIMEOUT);

        if (_in_fast_recovery) {

            uint32_t flightsize = _highest_sent - _last_acked;

            _cwnd = min(_ssthresh, flightsize + _mss);

        }

        if (_mSrc == NULL)
            _ssthresh = max(_cwnd / 2, (uint32_t) (2 * _mss));
        else
            _ssthresh = _mSrc->deflate_window(_cwnd, _mss);

        if (_sawtooth > 0)
            _rtt_avg = _rtt_cum / _sawtooth;
        else
            _rtt_avg = timeFromMs(0);

        _sawtooth = 0;
        _rtt_cum = timeFromMs(0);
        _cwnd = _mss;

        if (_mSrc)
            _mSrc->window_changed();

        _unacked = _cwnd;
        _effcwnd = _cwnd;
        _in_fast_recovery = false;
        _recoverq = _highest_sent;
        // yiting
        *_flow_total_sent += _last_acked + _mss - _highest_sent;
        _highest_sent = _last_acked + _mss;
        _dupacks = 0;

        retransmit_packet();
    } else {
        //cout << "start flow " << _super_id << endl;
        startflow();
    }
}

////////////////////////////////////////////////////////////////
//  TCP SINK
////////////////////////////////////////////////////////////////

TcpSink::TcpSink()
        : Logged("sink"), _cumulative_ack(0), _packets(0) {}

void
TcpSink::connect(TcpSrc &src, route_t &route) {
    _src = &src;
    _route = &route;
    _cumulative_ack = 0;
    _drops = 0;

    //cout << "sink id: " << this->id << endl;
}

// Note: _cumulative_ack is the last byte we've ACKed.
// seqno is the first byte of the new packet.
void
TcpSink::receivePacket(Packet &pkt) {
    TcpPacket *p = (TcpPacket *) (&pkt);
    TcpPacket::seq_t seqno = p->seqno();
    simtime_picosec ts = p->ts();
    int size = p->size(); // TODO: the following code assumes all packets are the same size
    pkt.flow().logTraffic(pkt, *this, TrafficLogger::PKT_RCVDESTROY);
    p->free();

    _packets += 1000;

    if (seqno == _cumulative_ack + 1) { // it's the next expected seq no
        //cout << "here seqno: " << seqno << " size: " << size << endl;
        _cumulative_ack = seqno + size - 1;
        // are there any additional received packets we can now ack?
        while (!_received.empty() && (_received.front() == _cumulative_ack + 1)) {
            _received.pop_front();
            _cumulative_ack += size;
            //cout << "next cumu_act: " << _cumulative_ack << endl;
        }
    } else if (seqno < _cumulative_ack + 1) {} //must have been a bad retransmit
    else { // it's not the next expected sequence number
        if (_received.empty()) {
            _received.push_front(seqno);
            //it's a drop in this simulator there are no reorderings.
            _drops += (1000 + seqno - _cumulative_ack - 1) / 1000;
        } else if (seqno > _received.back()) { // likely case
            _received.push_back(seqno);
        } else { // uncommon case - it fills a hole
            list<uint64_t>::iterator i;
            for (i = _received.begin(); i != _received.end(); i++) {
                if (seqno == *i) break; // it's a bad retransmit
                if (seqno < (*i)) {
                    _received.insert(i, seqno);
                    break;
                }
            }
        }
    }
    //cout << "cumu_ack: " << _cumulative_ack << endl;
    send_ack(ts);
}

void
TcpSink::send_ack(simtime_picosec ts) {
    TcpAck *ack = TcpAck::newpkt(_src->_flow, *_route, 0, _cumulative_ack);
    ack->flow().logTraffic(*ack, *this, TrafficLogger::PKT_CREATESEND);
    ack->set_ts(ts);
    ack->sendOn();
}


void TcpSink::replace_route(route_t *newroute) {
    _oldroute = _route;
    _route = newroute;
    _route->push_back(_src);
}

////////////////////////////////////////////////////////////////
//  TCP RETRANSMISSION TIMER
////////////////////////////////////////////////////////////////

TcpRtxTimerScanner::TcpRtxTimerScanner(simtime_picosec scanPeriod, EventList &eventlist)
        : EventSource(eventlist, "RtxScanner"),
          _scanPeriod(scanPeriod) {
}

void TcpRtxTimerScanner::StartFrom(simtime_picosec startTime) {
    eventlist().sourceIsPending(*this, startTime);
}

void
TcpRtxTimerScanner::registerTcp(TcpSrc &tcpsrc) {
    _tcps.push_back(&tcpsrc);
}

void
TcpRtxTimerScanner::doNextEvent() {
    simtime_picosec now = eventlist().now();
    tcps_t::iterator i;
    for (i = _tcps.begin(); i != _tcps.end(); i++) {
        (*i)->rtx_timer_hook(now, _scanPeriod);
    }
    eventlist().sourceIsPendingRel(*this, _scanPeriod);
}
