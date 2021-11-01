#include<iostream>
#include<sstream>
#include<vector>
#include<string>
#include<fstream>
#include<sys/socket.h>
#include<arpa/inet.h>  // struct sockaddr_in
#include"utils.h"
#include<thread>
#include<unistd.h>
#include<string.h>
#include"color.h"
#include"common.h"
using namespace std;

class Peer {
    string username, password;
    string peerIP, trackerIP;
    int peerPort, trackerPort;
    int peer_desc, tracker_desc;
    struct sockaddr_in trackerAddr, peerAddr;
    vector<string>trackerDetails;

public:
    void parseArgs(int argc, char* argv[]);
    void connectToTracker();
    void displayInfo();
};

void Peer::parseArgs(int argc, char* argv[]) {
    // Extract peer IP and Port from the command line arguments
    string peerDetails = string(argv[1]);
    int i = peerDetails.find_last_of(':');
    peerIP = peerDetails.substr(0, i);
    peerPort = stoi(peerDetails.substr(i + 1));

    // Retriving the tracker details from the text file
    trackerDetails = readFile(argv[2]);
    trackerIP = trackerDetails[0];
    trackerPort = stoi(trackerDetails[1]);
}

void Peer::displayInfo() {
    printf("%s------------------------------------------------------------%s\n", KYEL, RESET);
    printf("\t%sPEER%s ---------> %sIP:%s %s, %sPORT:%s %d\n", KRED, RESET, KYEL, RESET, &peerIP[0], KYEL, RESET, peerPort);
    printf("%s------------------------------------------------------------%s\n", KYEL, RESET);

}

void Peer::connectToTracker() {
    // Create Socket
    if ((tracker_desc = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
        perror("Create socket");
        exit(1);
    }   

    // Assigning custom IP and port to the peer
    peerAddr.sin_family = AF_INET;
    peerAddr.sin_addr.s_addr = inet_addr(&peerIP[0]);
    peerAddr.sin_port = htons(peerPort);  
    // Binding the IP and port to the peer
    bind(tracker_desc, (struct sockaddr*)&peerAddr, sizeof(peerAddr));

    int option_value = 1;
    if (setsockopt(tracker_desc, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &option_value, sizeof(option_value)) == -1) {
        perror("Socket options");
        exit(1);
    }
    // Assign the IP address and port number 
    trackerAddr.sin_family = AF_INET;
    // peer.sin_addr.s_addr = inet_addr(&peerIP[0]);
    inet_pton(AF_INET, &trackerIP[0], &trackerAddr.sin_addr.s_addr);
    trackerAddr.sin_port = htons(trackerPort);

    // Connecting to server
    if (connect(tracker_desc, (struct sockaddr*)&trackerAddr, sizeof(trackerAddr)) == -1) {
        puts("Failed to connect server");
        exit(1);
    }
    puts("Connected to tracker...");
    char reply[200];
    string input;
    while (true) {
        cout << ">> ";
        getline(cin, input);
        if (input.length() <= 0)
            continue;

        write(tracker_desc, &input[0], sizeof(input));

        vector<string> words = splitString(input);
        if (words[0] == "create_user") {
            username = words[1];
            password = words[2];
        }
        else if (words[0] == "login") {
            username = words[1];
            password = words[2];
            string loginDetails = peerIP + " " + to_string(peerPort);
            write(tracker_desc, &loginDetails[0], loginDetails.length());
        }
        else if (words[0] == "logout") {
            write(tracker_desc, &username[0], username.length());
        }
        memset(reply, 0, sizeof(reply));
        read(tracker_desc, reply, sizeof(reply));
        puts(reply);
    }
}


int main(int argc, char* argv[]) {
    if (argc != 3) {
        puts("Provide arguments <tracker_info_file> and <tracker_no>");
        exit(1);
    }

    Peer p;

    p.parseArgs(argc, argv);
    p.displayInfo();
    p.connectToTracker();
    // readInput();


    return 0;
}
