#ifndef __TRACKER_DATA_H_
#define __TRACKER_DATA_H_

#include<vector>
#include<unordered_map>
#include<thread>
using namespace std;

struct peerInfo {
    string username, password;
    string ip;
    int port;
    bool loggedIn;
};

struct groupInfo {
    string admin;
    vector<string> acceptedMembers;
    unordered_map<string, int> members;
};

vector<thread> peerThreads;
unordered_map<string, peerInfo> allPeers;
unordered_map<string, groupInfo> allGroups;

#endif