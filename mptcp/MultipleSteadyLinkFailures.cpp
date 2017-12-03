//
// Created by Dingming Wu on 11/28/17.
//

#include <set>
#include <iostream>
#include <random>
#include "MultipleSteadyLinkFailures.h"

struct Gen {
    mt19937 g;
    Gen() : g(static_cast<uint32_t>(time(0)))
    {
    }
    size_t operator()(size_t n)
    {
        std::uniform_int_distribution<size_t> d(0, n ? n-1 : 0);
        return d(g);
    }
};


MultipleSteadyLinkFailures::MultipleSteadyLinkFailures(EventList*ev, Topology *topo): _ev(ev),_topo(topo) {
    _allLinks = new vector<int>();
    _allSwitches = new vector<int>();
    _inNetworkLinks = new vector<int>();
    _inNetworkSwitches = new vector<int>();
    _outstandingFailedLinks = new set<int>();
    _outstandingFailedSwitches = new set<int>();

    _totalLinks = NHOST + NK*K/2+ NK*K/2;
    _totalSwitches = NK+NK+NK/2;
    for(int i = 0; i < _totalLinks;i++){
        _allLinks->push_back(i);
        if(i >= NHOST)
            _inNetworkLinks->push_back(i);
    }
    for(int i =0; i< _totalSwitches;i++){
        _allSwitches->push_back(i);
        if(i >= NK)
            _inNetworkSwitches->push_back(i);
    }
    _givenFailedLinks = new set<int>();
    _givenFailedSwitches = new set<int>();
}


void MultipleSteadyLinkFailures::setSingleSwitchFailure(int switchId) {
    _givenFailedSwitches->insert(switchId);
}

void MultipleSteadyLinkFailures::setRandomSwitchFailure(int num) {

    std::random_shuffle(_inNetworkSwitches->begin(),_inNetworkSwitches->end(),Gen());
    for(int i = 0; i < num; i++){
        int sid = _inNetworkSwitches->at(i);
        _givenFailedSwitches->insert(sid);
    }
}

void MultipleSteadyLinkFailures::setRandomSwitchFailure(double ratio) {
    int numSwitches = (int)(ratio*_totalSwitches);
    setRandomSwitchFailure(numSwitches);
}

void MultipleSteadyLinkFailures::updateBackupUsage() {
    multiset<int>* groupFailureCount = new multiset<int>();
    int gid = -1;
    for(int sid:*_givenFailedSwitches){
        if(sid < 2*NK)
            gid = sid/(K/2);
        else
            gid = 2*K+(sid-2*NK)%(K/2);
        if(groupFailureCount->count(gid) < BACKUPS_PER_GROUP )
            groupFailureCount->insert(gid);
        else
            _outstandingFailedSwitches->insert(sid);
    }
    for(int link:*_givenFailedLinks){
        pair<Queue*,Queue*> ret = _topo->linkToQueues(link);
        int sid1 = ret.second->_switchId;
        int sid2 = ret.first->_switchId;
        assert(sid1<=sid2);
        if(sid1 < 0){ //host link failure
            int gid2 = sid2/(K/2);
            if(groupFailureCount->count(gid2)<BACKUPS_PER_GROUP)
                groupFailureCount->insert(gid2);
            else
                _outstandingFailedLinks->insert(link);
        }
        else if(sid1<NK) {
            //lp to up link failure
            assert(sid2 >= NK && sid2 < 2 * NK);
            int gid1 = sid1/(K/2);
            int gid2 = sid2/(K/2);
            if(groupFailureCount->count(gid1) <BACKUPS_PER_GROUP
                    && groupFailureCount->count(gid2)<BACKUPS_PER_GROUP)
            {
                groupFailureCount->insert(gid1);
                groupFailureCount->insert(gid2);
            } else
                _outstandingFailedLinks->insert(link);

        }else{
            // up to core link failure
            assert(sid1<2*NK && sid2>=2*NK);
            int gid1 = sid1/(K/2);
            int gid2 = 2*K+ (sid2-2*NK)%(K/2);
            if(groupFailureCount->count(gid1) <BACKUPS_PER_GROUP
               && groupFailureCount->count(gid2)<BACKUPS_PER_GROUP)
            {
                groupFailureCount->insert(gid1);
                groupFailureCount->insert(gid2);
            } else
                _outstandingFailedLinks->insert(link);
        }

    }
}
void MultipleSteadyLinkFailures::setRandomLinkFailures(int num) {

    std::random_shuffle (_inNetworkLinks->begin(), _inNetworkLinks->end(),Gen() );
    for(int i = 0; i < num; i++){
        int linkid = _inNetworkLinks->at(i);
        _givenFailedLinks->insert(linkid);
    }
}
void MultipleSteadyLinkFailures::setRandomLinkFailures(double ratio) {
    int numLinks = (int) (ratio*_totalLinks);
    setRandomLinkFailures(numLinks);
}

void MultipleSteadyLinkFailures::setSingleLinkFailure(int linkid) {
    _givenFailedLinks->insert(linkid);
}

void MultipleSteadyLinkFailures::installFailures() {
    set<int>*actualFailedLinks, *actualFailedSwitches;
    if(_useShareBackup) {
        updateBackupUsage();
        actualFailedLinks = _outstandingFailedLinks;
        actualFailedSwitches = _outstandingFailedSwitches;
    }else{
        actualFailedLinks = _givenFailedLinks;
        actualFailedSwitches = _givenFailedSwitches;
    }

    cout<<"failedSwitched:";
    for (int sid: *actualFailedSwitches){
        _topo->failSwitch(sid);
        cout<<sid<<" ";
    }
    cout<<" failedLinks:";
    for (int link: *actualFailedLinks) {
        _topo->failLink(link);
        cout<<link<<" ";
    }
    cout<<endl;
}

