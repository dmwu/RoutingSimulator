//
// Created by Dingming Wu on 11/28/17.
//

#include <set>
#include "MultipleSteadyLinkFailures.h"
MultipleSteadyLinkFailures::MultipleSteadyLinkFailures(EventList*ev, Topology *topo): _ev(ev),_topo(topo) {
    _allLinks = new vector<int>();
    _allSwitches = new vector<int>();
    _outstandingFailedLinks = new vector<int>();
    _outstandingFailedSwitches = new vector<int>();
    _totalLinks = NHOST + NK*K/2+ NK*K/2;
    _totalSwitches = NK+NK+NK/2;
    for(int i = 0; i < _totalLinks;i++){
        _allLinks->push_back(i);
    }
    for(int i =0; i< _totalSwitches;i++){
        _allSwitches->push_back(i);
    }
    _failedLinks = new set<int>();
    _failedSwitches = new set<int>();
}


void MultipleSteadyLinkFailures::setSingleSwitchFailure(int switchId) {
    _failedSwitches->insert(switchId);
}

void MultipleSteadyLinkFailures::setRandomSwitchFailure(int num) {
    std::srand(0);
    std::random_shuffle(_allSwitches->begin(),_allSwitches->end());
    for(int i = 0; i < num; i++){
        int sid = _allSwitches->at(i);
        _failedSwitches->insert(sid);
    }
}

void MultipleSteadyLinkFailures::setRandomSwitchFailure(double ratio) {
    int numSwitches = (int)(ratio*_totalSwitches);
    setRandomSwitchFailure(numSwitches);
}

void MultipleSteadyLinkFailures::updateBackupUsage() {
    multiset<int>* groupFailureCount = new multiset<int>();
    int gid = -1;
    for(int sid:*_failedSwitches){
        if(sid < 2*NK)
            gid = sid/(K/2);
        else
            gid = 2*K+(sid-2*NK)%(K/2);
        if(groupFailureCount->count(gid) < BACKUPS_PER_GROUP )
            groupFailureCount->insert(gid);
        else
            _outstandingFailedSwitches->push_back(sid);
    }
    for(int link:*_failedLinks){
        pair<Queue*,Queue*> ret = _topo->linkToQueues(link);
        int sid1 = ret.second->_switchId;
        int sid2 = ret.first->_switchId;
        assert(sid1<=sid2);
        if(sid1 < 0){ //host link failure
            int gid2 = sid2/(K/2);
            if(groupFailureCount->count(gid2)<BACKUPS_PER_GROUP)
                groupFailureCount->insert(gid2);
            else
                _outstandingFailedLinks->push_back(link);
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
                _outstandingFailedLinks->push_back(link);

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
                _outstandingFailedLinks->push_back(link);
        }

    }
}
void MultipleSteadyLinkFailures::setRandomLinkFailures(int num) {
    std::srand (0);
    std::random_shuffle ( _allLinks->begin(), _allLinks->end() );
    for(int i = 0; i < num; i++){
        int linkid = _allLinks->at(i);
        _failedLinks->insert(linkid);
    }
}
void MultipleSteadyLinkFailures::setRandomLinkFailures(double ratio) {
    int numLinks = (int) (ratio*_totalLinks);
    setRandomLinkFailures(numLinks);
}

void MultipleSteadyLinkFailures::setSingleLinkFailure(int linkid) {
    _failedLinks->insert(linkid);
}

void MultipleSteadyLinkFailures::installFailures() {
    if(_useShareBackup) {
        updateBackupUsage();
        for (int sid:*_outstandingFailedSwitches) {
            _topo->failSwitch(sid);
        }
        for (int link:*_outstandingFailedLinks) {
            _topo->failLink(link);
        }
    }else{
        for(int sid:*_failedSwitches){
            _topo->failSwitch(sid);
        }
        for(int link:*_failedLinks){
            _topo->failLink(link);
        }
    }
}

