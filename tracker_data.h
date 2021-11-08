#ifndef __TRACKER_DATA_H_
#define __TRACKER_DATA_H_

#include<vector>
#include<deque>
#include<unordered_map>
#include<thread>
using namespace std;

struct peerInfo {
    string username, password;
    string ip;
    int port;
    bool loggedIn;
};

struct fileInfo {
    string hash;
    int noOfChunks;
    long long lastChunkSize;
    string location;
    vector<string> users;
};

struct groupInfo {
    string admin;
    deque<string> acceptedMembers;
    unordered_map<string, int> members;

    ///////////// FILE_NAME ////
    unordered_set<string> files;

    ///////////// FILE_PATH, FILE_NAME /////////
    // unordered_map<string, string> files;
};

// vector<thread> peerThreads;

///////////// PEER_NAME,INFO ///////////
unordered_map<string, peerInfo> allPeers;

///////////// GROUP_NAME,INFO //////////
unordered_map<string, groupInfo> allGroups;

///////////// FILE_NAME,INFO ///////////
unordered_map<string, fileInfo> allFiles;

#endif