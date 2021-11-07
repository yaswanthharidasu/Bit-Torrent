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

struct fileInfo {
    string hash;
    int noOfChunks;
    long long lastChunkSize;
    string location;
    vector<string> users;
};

struct groupInfo {
    string admin;
    vector<string> acceptedMembers;
    unordered_map<string, int> members;
    ///////////// FILE_PATH, FILE_NAME /////////
    // unordered_map<string, string> files;
    ///////////// FILE_PATH ////
    unordered_set<string> files;
    // unordered_map<string, int> fileChunks;
    ///////////// FILE_NAME /////////// USER ////// CHUNKS ///////
    // unordered_map<string, unordered_map<string, vector<int>>>files;
};

vector<thread> peerThreads;
unordered_map<string, peerInfo> allPeers;
unordered_map<string, groupInfo> allGroups;
///////////// FILE_NAME, INFO ///////////
unordered_map<string, fileInfo> allFiles;

#endif