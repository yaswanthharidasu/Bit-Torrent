#ifndef COMMON_H
#define COMMON_H

#include<vector>
#include<string>
#include<thread>
#include<unordered_map>
using namespace std;

struct peerInfo {
    string username, password;
    string ip;
    int port;
    bool loggedIn;
};


vector<thread> peerThreads;
unordered_map<string, peerInfo> allPeers;

#endif