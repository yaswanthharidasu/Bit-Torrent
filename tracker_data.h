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
    unordered_map<string, string> files;
    // unordered_map<string, int> fileChunks;
    ///////////// FILE_NAME /////////// USER ////// CHUNKS ///////
    // unordered_map<string, unordered_map<string, vector<int>>>files;
};

vector<thread> peerThreads;
unordered_map<string, peerInfo> allPeers;
unordered_map<string, groupInfo> allGroups;
// file_path is used as key because file_name can be same in two different folders.
// Thus, file_name is not unique and cannot be used as key
unordered_map<string, fileInfo> allFiles;

#endif