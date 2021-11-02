#include<iostream>
#include<vector>
#include<sys/socket.h>
#include<arpa/inet.h>  // struct sockaddr_in
#include<pthread.h>
#include<thread>
#include"log.h"
#include"common.h"
#include"utils.h"
#include<unistd.h>
#include<string.h>
#include"color.h"
class Tracker {
    Log log;
    pthread_t exitThreadId;
    // thread threadfunc;

    int trackerPort;
    string trackerIP;
    vector<string> trackerDetails;

    int tracker_desc, peer_desc;
    struct sockaddr_in trackerAddr, peerAddr;

public:
    void parseArgs(int argc, char* argv[]);
    // Spawn new thread for exiting the server using "quit"
    void exitThread();
    void connectToPeer();
    void trackerAsServer(int desc);
    void displayInfo();
    void processCommand(string command, int desc);
    void create_user(string username, string password, int desc);
    void login(string username, string password, int desc);
    void logout(string username, int desc);
    void list_users(string myself, int desc);
};

void Tracker::parseArgs(int argc, char* argv[]) {
    // Log File
    log.setFileName("tracker_log_" + string(argv[2]) + ".txt");
    log.clearLog();
    log.printLog("Assigning tracker details\n");

    // Reading the IP address and port from the given text file
    trackerDetails = readFile(argv[1]);
    if (string(argv[2]) == "1") {
        trackerIP = trackerDetails[0];
        trackerPort = stoi(trackerDetails[1]);
    }
    else if (string(argv[2]) == "2") {
        trackerIP = trackerDetails[2];
        trackerPort = stoi(trackerDetails[3]);
    }
    log.printLog("Completed assigning tracker details\n");
}

void Tracker::displayInfo() {
    printf("%s------------------------------------------------------------%s\n", KYEL, RESET);
    printf("\t%sTRACKER%s ---------> %sIP:%s %s %sPORT:%s %d\n", KRED, RESET, KYEL, RESET, &trackerIP[0], KYEL, RESET, trackerPort);
    printf("%s------------------------------------------------------------%s\n", KYEL, RESET);
}

void Tracker::exitThread() {
    // thread exitFunc(exitThreadRoutine);
    if (pthread_create(&exitThreadId, NULL, exitThreadRoutine, NULL) < 0) {
        perror("ExitThread");
    }
    // if (exitFunc.joinable())
    //     exitFunc.join();
}

void Tracker::connectToPeer() {
    if ((tracker_desc = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
        perror("Create socket");
        exit(1);
    }
    log.printLog("Socket Created\n");

    // Set the socket options
    // IP Address and port can be reused .i.e. value of those two options are being set to 1
    int option_value = 1;
    if (setsockopt(tracker_desc, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &option_value, sizeof(option_value)) == -1) {
        perror("Socket options");
        exit(1);
    }
    log.printLog("Socket options set\n");

    // Assign the IP address and port number 
    trackerAddr.sin_family = AF_INET;
    trackerAddr.sin_port = htons(trackerPort);
    if (inet_pton(AF_INET, &trackerIP[0], &trackerAddr.sin_addr.s_addr) != 1) {
        perror("pton");
        exit(1);
    }

    // Bind socket to the tracker port
    if (bind(tracker_desc, (struct sockaddr*)&trackerAddr, sizeof(trackerAddr)) < 0) {
        perror("Bind socket");
        exit(1);
    }
    log.printLog("Binded the socket\n");

    // Listen for the connectToPeers
    listen(tracker_desc, 10);
    puts("Listening...");

    int p = sizeof(struct sockaddr_in);
    while (true) {
        // Accepting the connection
        if ((peer_desc = accept(tracker_desc, (struct sockaddr*)&peerAddr, (socklen_t*)&p)) < 0) {
            perror("Accept peer Connection");
            exit(1);
        }
        printf("....Incoming Connection from %sIP:%s %s %sPORT:%s %d | %sCONNECTED%s\n",
            KYEL, RESET, inet_ntoa(peerAddr.sin_addr), KYEL, RESET, ntohs(peerAddr.sin_port), KGRN, RESET);

        // Creating threads for each peer
        peerThreads.push_back(thread(&Tracker::trackerAsServer, this, peer_desc));
    }

    // Waiting for the main to stop until all threads are executed
    for (int i = 0; i < peerThreads.size(); i++) {
        if (peerThreads[i].joinable())
            peerThreads[i].join();
    }
}

void Tracker::trackerAsServer(int desc) {
    while (true) {
        char command[200];
        memset(command, 0, sizeof(command));
        // Read command from peer
        recv(desc, command, sizeof(command), 0);
        if (strlen(command) <= 0)
            continue;
        puts(command);
        processCommand(command, desc);
    }
}

void Tracker::processCommand(string command, int desc) {
    vector<string> words = splitString(command);
    string reply;
    if ((words[0] == "create_user" && words.size() < 3) ||
        words[0] == "login" && words.size() < 3
        ) {
        reply = "Invalid Arguments";
        send(desc, &reply[0], reply.length(), 0);
    }
    else if (words[0] == "create_user") {
        create_user(words[1], words[2], desc);
    }
    else if (words[0] == "login") {
        login(words[1], words[2], desc);
    }
    else if (words[0] == "logout") {
        logout(words[1], desc);
    }
    else if (words[0] == "list_users") {
        cout << "listing...." << endl;
        list_users(words[1], desc);
    }
    else {
        reply = "Invalid Command";
        send(desc, &reply[0], reply.length(), 0);
    }
}

void Tracker::create_user(string username, string password, int desc) {
    string reply;
    if (allPeers.find(username) != allPeers.end()) {
        reply = "Username/Password already exists";
    }
    else {
        peerInfo newPeer;
        newPeer.username = username;
        newPeer.password = password;
        newPeer.loggedIn = false;
        allPeers[username] = newPeer;
        reply = "Registration Completed Successfully";
        log.printLog("User '" + username + "' registered succesfully\n");
    }
    send(desc, &reply[0], reply.length(), 0);
}

void Tracker::login(string username, string password, int desc) {
    string reply;
    if (allPeers.find(username) == allPeers.end())
        reply = "User not found";
    else if (allPeers[username].loggedIn)
        reply = "You already have an active session";
    else if (allPeers[username].password != password)
        reply = "Incorrect password";
    else {
        allPeers[username].loggedIn = true;
        allPeers[username].ip = inet_ntoa(peerAddr.sin_addr);
        allPeers[username].port = ntohs(peerAddr.sin_port);
        reply = "Logged in successfully";
        log.printLog("User '" + username + "'logged in with IP ADDR: " +
            allPeers[username].ip + " PORT: " + to_string(allPeers[username].port) + "\n");
    }
    send(desc, &reply[0], reply.length(), 0);
}

void Tracker::logout(string username, int desc) {
    // char buff[200];
    // Reading username
    // recv(desc, buff, sizeof(buff), 0);
    // string username = buff;
    string reply;
    if (username == "none" || allPeers[username].loggedIn == false) {
        cout << username << endl;
        reply = "Did not found any active session";
    }
    else if (allPeers[username].loggedIn) {
        allPeers[username].loggedIn = false;
        reply = "Logged out successfully";
        log.printLog("User '" + username + "'logged out\n");
    }
    send(desc, &reply[0], reply.length(), 0);
}

void Tracker::list_users(string myself, int desc) {
    // char buff[200] = { 0 };
    // Reading username
    // cout << "read" << endl;
    // bzero(buff, sizeof(buff));
    // recv(desc, buff, sizeof(buff), 0);
    // string myself = buff;
    // cout << "myself:" << myself << endl;
    string users;
    int count = 0;
    for (auto it : allPeers) {
        if (it.second.loggedIn && it.second.username != myself) {
            users += it.first + "\n";
            count++;
        }
    }
    if (count == 0) {
        users = "No active users";
    }
    // send(desc, )
    send(desc, &users[0], users.length(), 0);
}


int main(int argc, char* argv[]) {
    if (argc != 3) {
        puts("Provide arguments <tracker_info_file> and <tracker_no>");
        exit(1);
    }
    Tracker t;
    t.parseArgs(argc, argv);
    t.displayInfo();
    t.connectToPeer();
    t.exitThread();

    return 0;
}
