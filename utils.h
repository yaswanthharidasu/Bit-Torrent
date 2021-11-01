#ifndef UTILS_H
#define UTILS_H

#include<iostream>
#include<string>
#include<sstream>
using namespace std;

// Checks if the "quit" is entered in the tracker terminal
void* exitThreadRoutine(void* ptr) {
    string line;
    while (true) {
        getline(cin, line);
        if (line == "quit") {
            exit(0);
        }
    }
}

// Read line by line from the file
vector<string> readFile(char* arg) {
    // Reading the file
    vector<string> result;
    ifstream ifs(string(arg), ifstream::in);
    for (string line; getline(ifs, line);) {
        result.push_back(line);
    }
    // Closing the file
    ifs.close();
    return result;
}

vector<string> splitString(string str) {
    istringstream iss(str);
    vector<string> result;
    string word;
    while (iss >> word) {
        result.push_back(word);
    }
    return result;
}

#endif