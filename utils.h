#ifndef __UTILS_H_
#define __UTILS_H_

#include<iostream>
#include<string>
#include<sstream>
#include<fstream>
#include<sys/stat.h>
#include<openssl/sha.h>

#define CHUNK_SIZE 524288 // (512*1024) 

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

// Tokenize the given string into words
vector<string> splitString(string str) {
    istringstream iss(str);
    vector<string> result;
    string word;
    while (iss >> word) {
        result.push_back(word);
    }
    return result;
}

// Check if the given path is a file or not
bool validFilePath(string path) {
    struct stat fileStat;
    if (stat(&path[0], &fileStat)) {
        return false;
    }
    if (!S_ISREG(fileStat.st_mode)) {
        return false;
    }
    return true;
}

string getFileName(string path) {
    string name;
    int i;
    for (i = path.length() - 1; i >= 0; i--) {
        if (path[i] == '/') {
            i += 1;
            break;
        }
    }
    name = path.substr(i);
    return name;
}

// Returns file size
long long getFileSize(string file_path) {
    struct stat results;
    stat(&file_path[0], &results);
    return results.st_size;
}

// Returns SHA-1 hash of the file
// string getSHAhash(string file_path, int no_of_chunks, long long last_chunk_size) {
//     unsigned char input[CHUNK_SIZE];
//     unsigned char piece_hash[SHA_DIGEST_LENGTH];

//     ifstream ifs;
//     ifs.open(file_path, ifstream::in);
//     string hash;

//     while (no_of_chunks--) {
//         ifs.read((char*)input, CHUNK_SIZE);
//         if (no_of_chunks > 1) {
//             SHA1(input, CHUNK_SIZE, piece_hash);
//         }
//         else {
//             SHA1(input, last_chunk_size, piece_hash);
//         }
//         string piece((char*)piece_hash);
//         hash += piece;
//         // piece_wise += piece + " ";
//     }
//     ifs.close();
//     return hash;
// }

void send_file(string path, int desc) {
    char buff[BUFSIZ];
    FILE* src = fopen(&path[0], "r");
    size_t size;
    // copying the contents
    while ((size = fread(buff, 1, BUFSIZ, src)) > 0) {
        send(desc, buff, size, 0);
        memset(buff, 0, sizeof(buff));
    }
    fclose(src);
}

void receive_file(string destination, int desc) {
    char buff[BUFSIZ];
    FILE* dest = fopen(&destination[0], "w");
    size_t size;
    while ((size = recv(desc, buff, sizeof(buff), 0)) > 0) {
        fwrite(buff, 1, size, dest);
        memset(buff, 0, sizeof(buff));
    }
    fclose(dest);
    return;
}

#endif