#include"common.h"
#include"tracker_data.h"

class Tracker {
    Log log;
    pthread_t exitThreadId;
    // thread threadfunc;

    int trackerPort;
    string trackerIP;
    vector<string> trackerDetails;

    int tracker_desc, peer_desc;
    struct sockaddr_in trackerAddr, peerAddr;
    struct threadArgs {
        Tracker* t;
        int desc;
    };

public:
    void parseArgs(int argc, char* argv[]);
    void displayInfo();
    
    // Spawn new thread for exiting the server using "quit"
    void exitThread();

    // =========================== Tracker <--> Peer communication methods ========================
    void connectToPeer();
    void trackerAsServer(int desc);
    void processCommand(string command, int desc);

    // =================================== Command methods ========================================
    void create_user(string username, string password, int desc);
    void login(string username, string password, string ip, string port, int desc);
    void logout(string username, int desc);
    void create_group(string group_name, string username, int desc);
    void list_users(string myself, int desc);
    void list_groups(int desc);
    void list_requests(string group_name, string username, int desc);
    void join_group(string group_name, string username, int desc);
    void leave_group(string group_name, string username, int desc);
    void accept_request(string group_name, string user, string myusername, int desc);
    void upload_file(string file_path, string group_name, string username, string file_hash, string no_of_chunks, string last_chunk_size, int desc);
    void list_files(string group_name, int desc);
    void download_file(string group_name, string file_name, string destination, string username, int desc);

    // ================================== Utitlity methods ========================================
    void file_info(string file_name, string username, int desc);

    // ================================== Thread helpers ==========================================
    static void* serverHandler(void* ptr) {
        int desc = ((struct threadArgs*)ptr)->desc;
        ((struct threadArgs*)ptr)->t->trackerAsServer(desc);
        return 0;
    }

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
    pthread_t id;
    while (true) {
        // Accepting the connection
        if ((peer_desc = accept(tracker_desc, (struct sockaddr*)&peerAddr, (socklen_t*)&p)) < 0) {
            perror("Accept peer Connection");
            exit(1);
        }
        printf("....Incoming Connection from %sIP:%s %s %sPORT:%s %d | %sCONNECTED%s\n",
            KYEL, RESET, inet_ntoa(peerAddr.sin_addr), KYEL, RESET, ntohs(peerAddr.sin_port), KGRN, RESET);

        log.printLog("Incoming connection from IP: " + string(inet_ntoa(peerAddr.sin_addr))
            + " PORT: " + to_string(ntohs(peerAddr.sin_port)));

        // Creating threads for each peer
        threadArgs* args = new threadArgs();
        args->t = this;
        args->desc = peer_desc;
        pthread_create(&id, NULL, &Tracker::serverHandler, (void*)args);
        // peerThreads.push_back(thread(&Tracker::trackerAsServer, this, peer_desc));
    }
    // Waiting for the main to stop until all threads are executed
    // for (int i = 0; i < peerThreads.size(); i++) {
    //     if (peerThreads[i].joinable())
    //         peerThreads[i].join();
    // }
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
    if ((words[0] == CREATE_USER && words.size() != 3) ||
        (words[0] == LOGIN && words.size() != 5) ||
        (words[0] == LOGOUT && words.size() != 2) ||
        (words[0] == CREATE_GROUP && words.size() != 3) ||
        (words[0] == LIST_USERS && words.size() != 2) ||
        (words[0] == LIST_GROUPS && words.size() != 1) ||
        (words[0] == JOIN_GROUP && words.size() != 3) ||
        (words[0] == LEAVE_GROUP && words.size() != 3) ||
        (words[0] == LIST_REQUESTS && words.size() != 3) ||
        (words[0] == ACCEPT_REQUEST && words.size() != 4) ||
        (words[0] == UPLOAD_FILE && words.size() != 7) ||
        (words[0] == LIST_FILES && words.size() != 2) ||
        (words[0] == DOWNLOAD_FILE && words.size() != 5)
        ) {
        reply = KYEL "Invalid Arguments" RESET;
        send(desc, &reply[0], reply.length(), 0);
    }
    else if (words[0] == CREATE_USER) {
        create_user(words[1], words[2], desc);
    }
    else if (words[0] == LOGIN) {
        login(words[1], words[2], words[3], words[4], desc);
    }
    else if (words[0] == LOGOUT) {
        logout(words[1], desc);
    }
    else if (words[0] == CREATE_GROUP) {
        create_group(words[1], words[2], desc);
    }
    else if (words[0] == LIST_USERS) {
        list_users(words[1], desc);
    }
    else if (words[0] == LIST_GROUPS) {
        list_groups(desc);
    }
    else if (words[0] == JOIN_GROUP) {
        join_group(words[1], words[2], desc);
    }
    else if (words[0] == LEAVE_GROUP) {
        leave_group(words[1], words[2], desc);
    }
    else if (words[0] == LIST_REQUESTS) {
        list_requests(words[1], words[2], desc);
    }
    else if (words[0] == ACCEPT_REQUEST) {
        accept_request(words[1], words[2], words[3], desc);
    }
    else if (words[0] == UPLOAD_FILE) {
        upload_file(words[1], words[2], words[3], words[4], words[5], words[6], desc);
    }
    else if (words[0] == LIST_FILES) {
        list_files(words[1], desc);
    }
    else if (words[0] == DOWNLOAD_FILE) {
        download_file(words[1], words[2], words[3], words[4], desc);
    }
    else if (words[0] == FILE_INFO) {
        file_info(words[1], words[2], desc);
    }
    else if (words[0] == "send_message") {
        string user = words[1];
        reply = allPeers[user].ip + " " + to_string(allPeers[user].port);
        send(desc, &reply[0], reply.length(), 0);
    }
    else {
        reply = KRED "Invalid Command" RESET;
        send(desc, &reply[0], reply.length(), 0);
    }
}

void Tracker::create_user(string username, string password, int desc) {
    string reply;
    if (allPeers.find(username) != allPeers.end()) {
        reply = KRED "Username/Password already exists" RESET;
    }
    else {
        peerInfo newPeer;
        newPeer.username = username;
        newPeer.password = password;
        newPeer.loggedIn = false;
        allPeers[username] = newPeer;
        reply = USER_REGISTER_SUCCESS;
        log.printLog("User '" + username + "' registered succesfully\n");
    }
    send(desc, &reply[0], reply.length(), 0);
}

void Tracker::login(string username, string password, string ip, string port, int desc) {
    string reply;
    if (allPeers.find(username) == allPeers.end())
        reply = KRED "User not found" RESET;
    else if (allPeers[username].loggedIn)
        reply = KYEL "You already have an active session" RESET;
    else if (allPeers[username].password != password)
        reply = KRED "Incorrect password" RESET;
    else {
        allPeers[username].loggedIn = true;
        allPeers[username].ip = ip;
        allPeers[username].port = stoi(port);
        reply = LOGIN_SUCCESS;
        log.printLog("User '" + username + "'logged in with IP ADDR: " +
            allPeers[username].ip + " PORT: " + to_string(allPeers[username].port) + "\n");
    }
    send(desc, &reply[0], reply.length(), 0);
}

void Tracker::logout(string username, int desc) {
    string reply;
    if (allPeers.find(username) == allPeers.end() || allPeers[username].loggedIn == false) {
        reply = KYEL "Did not found any active session" RESET;
    }
    else if (allPeers[username].loggedIn) {
        allPeers[username].loggedIn = false;
        reply = LOGOUT_SUCCESS;
        log.printLog("User '" + username + "'logged out\n");
    }
    send(desc, &reply[0], reply.length(), 0);
}

void Tracker::create_group(string group_name, string username, int desc) {
    string reply;
    if (allPeers.find(username) == allPeers.end() || allPeers[username].loggedIn == false) {
        reply = KYEL "Login to create a group" RESET;
    }
    else if (allGroups.find(group_name) != allGroups.end()) {
        reply = KRED "Group name already exists" RESET;
    }
    else {
        groupInfo newGroup;
        newGroup.admin = username;
        allGroups[group_name] = newGroup;
        reply = GROUP_REGISTER_SUCCESS;
    }
    send(desc, &reply[0], reply.length(), 0);
}

void Tracker::list_users(string myself, int desc) {
    string reply;
    int count = 0;
    for (auto it : allPeers) {
        if (it.second.loggedIn && it.second.username != myself) {
            reply += it.first + "\n";
            count++;
        }
    }
    if (count == 0) {
        reply = KYEL "No active users" RESET;
    }
    send(desc, &reply[0], reply.length(), 0);
}

void Tracker::list_groups(int desc) {
    string reply;
    if (allGroups.size() == 0) {
        reply = KYEL "No Groups" RESET;
    }
    else {
        bool flag = true;
        for (auto it : allGroups) {
            if (flag) {
                reply += KYEL + it.first + RESET;
                flag = false;
            }
            else {
                reply += "\n" KYEL + it.first + RESET;
            }
        }
    }
    send(desc, &reply[0], reply.length(), 0);
}

void Tracker::list_requests(string group_name, string username, int desc) {
    string reply;
    if (allGroups.find(group_name) == allGroups.end()) {
        reply = KRED "Group doesn't exists" RESET;
    }
    else if (allGroups[group_name].admin != username) {
        reply = KYEL "You're not admin of the group and cannot see the requests" RESET;
    }
    else if (allPeers.find(username) == allPeers.end() || allPeers[username].loggedIn == false) {
        reply = KYEL "Login to see the requests" RESET;
    }
    else if (allGroups[group_name].members.size() == 0) {
        reply = KYEL "No pending requests" RESET;
    }
    else {
        bool flag = true;
        for (auto it : allGroups[group_name].members) {
            if (it.second == 0) {
                if (flag) {
                    reply += KYEL + it.first + RESET;
                    flag = false;
                }
                else {
                    reply += "\n" KYEL + it.first + RESET;
                }
            }
        }
    }
    send(desc, &reply[0], reply.length(), 0);
}

void Tracker::join_group(string group_name, string username, int desc) {
    string reply;
    if (allPeers.find(username) == allPeers.end() || allPeers[username].loggedIn == false) {
        reply = KYEL "Login to join a group" RESET;
    }
    else if (allGroups.find(group_name) == allGroups.end()) {
        reply = KRED "Group doesn't exists" RESET;
    }
    else if (allGroups[group_name].admin == username) {
        reply = KYEL "You're admin of the group. No need to join again" RESET;
    }
    else if (allGroups[group_name].members.find(username) != allGroups[group_name].members.end()) {
        reply = KYEL "You're already part of the group" RESET;
    }
    else {
        // In pending state
        allGroups[group_name].members[username] = 0;
        reply = GROUP_JOIN_SUCCESS;
    }
    send(desc, &reply[0], reply.length(), 0);
}

void Tracker::leave_group(string group_name, string username, int desc) {
    string reply;
    if (allPeers.find(username) == allPeers.end() || allPeers[username].loggedIn == false) {
        reply = KYEL "Login to leave a group" RESET;
    }
    else if (allGroups.find(group_name) == allGroups.end()) {
        reply = KRED "Group doesn't exists" RESET;
    }
    else if (allGroups[group_name].admin == username) {
        reply = KYEL "You're admin of the group. No need to join again" RESET;
    }
    send(desc, &reply[0], reply.length(), 0);
}

void Tracker::accept_request(string group_name, string user, string myusername, int desc) {
    string reply;
    if (allGroups.find(group_name) == allGroups.end()) {
        reply = KRED "Group doesn't exists" RESET;
    }
    else if (allPeers.find(myusername) == allPeers.end() || allPeers[myusername].loggedIn == false) {
        reply = KYEL "Login to accept the request" RESET;
    }
    else if (allGroups[group_name].admin != myusername) {
        reply = KRED "You're not admin of the group and cannot accept the requests" RESET;
    }
    else if (allGroups[group_name].members.find(user) == allGroups[group_name].members.end()) {
        reply = KRED "No user request exists with the given id" RESET;
    }
    else if (allGroups[group_name].members[user] == 1) {
        reply = KYEL "User's request has been already accepted" RESET;
    }
    else {
        allGroups[group_name].members[user] = 1;
        allGroups[group_name].acceptedMembers.push_back(user);
        reply = KGRN "User's request accepted successfully" RESET;
    }
    send(desc, &reply[0], reply.length(), 0);
}

void Tracker::upload_file(string file_path, string group_name, string username, string file_hash, string no_of_chunks, string last_chunk_size, int desc) {
    string reply;
    if (allPeers.find(username) == allPeers.end() || allPeers[username].loggedIn == false) {
        reply = KYEL "Login to upload the file" RESET;
    }
    else if (allGroups.find(group_name) == allGroups.end()) {
        reply = KRED "Group doesn't exists" RESET;
    }
    else if (allGroups[group_name].admin != username && allGroups[group_name].members.find(username) == allGroups[group_name].members.end()) {
        reply = KYEL "Join the group to upload the files" RESET;
    }
    else if (allGroups[group_name].admin != username && allGroups[group_name].members[username] == 0) {
        reply = KYEL "Your group join request was not accepted by admin. Wait for admin's approval to upload the files" RESET;
    }
    else if (!validFilePath(file_path)) {
        reply = KRED "Enter valid file path" RESET;
    }
    else if (allGroups[group_name].files.find(file_hash) != allGroups[group_name].files.end()) {
        reply = KYEL "File already exists in the group. No need to upload again" RESET;
    }
    else {
        // Storing <file_name> in the group
        string file_name = getFileName(file_path);
        allGroups[group_name].files.insert(file_name);
        // Storing <file_name, fileInfo> in allFiles
        fileInfo newFile;
        newFile.location = file_path;
        newFile.noOfChunks = stoi(no_of_chunks);
        newFile.lastChunkSize = stoll(last_chunk_size);
        newFile.users.push_back(username);
        allFiles[file_name] = newFile;
        reply = KGRN "Uploaded the file in the group successfully" RESET;
    }
    send(desc, &reply[0], reply.length(), 0);
}

void Tracker::list_files(string group_name, int desc) {
    string reply;
    if (allGroups.find(group_name) == allGroups.end()) {
        reply = KRED "Group doesn't exists" RESET;
    }
    else if (allGroups[group_name].files.size() == 0) {
        reply = KYEL "No files in the group" RESET;
    }
    else {
        unordered_set<string> files = allGroups[group_name].files;
        bool flag = true;
        for (auto it : files) {
            if (flag) {
                reply += KGRN + it + RESET;
                flag = false;
            }
            else {
                reply += "\n" KGRN + it + RESET;
            }
        }
    }
    send(desc, &reply[0], reply.length(), 0);
}

void Tracker::download_file(string group_name, string file_name, string destination, string username, int desc) {
    string reply;
    if (allPeers.find(username) == allPeers.end() || allPeers[username].loggedIn == false) {
        reply = KYEL "Login to download the file" RESET;
    }
    else if (allGroups.find(group_name) == allGroups.end()) {
        reply = KRED "Group doesn't exists" RESET;
    }
    else if (allGroups[group_name].admin != username && allGroups[group_name].members.find(username) == allGroups[group_name].members.end()) {
        reply = KYEL "Join the group to download the files" RESET;
    }
    else if (allGroups[group_name].admin != username && allGroups[group_name].members[username] == 0) {
        reply = KYEL "You're not part of the group" RESET;
    }
    // else if (!validFilePath(file_name)) {
    //     reply = KRED "Enter valid file path" RESET;
    // }
    else if (!validDirectory(destination)) {
        reply = KRED "Destination path is not a valid directory" RESET;
    }
    else {
        if (allGroups[group_name].files.find(file_name) == allGroups[group_name].files.end()) {
            reply = KYEL "File doesn't exists in the group" RESET;
        }
        else {
            fileInfo reqFile = allFiles[file_name];
            // Send all the peers which has that file.
            // reply +=  file_name;
            reply += "$";
            bool flag = true;
            for (int i = 0; i < reqFile.users.size(); i++) {
                string user = reqFile.users[i];
                if (user == username) {
                    reply = KYEL "You already have the file" RESET;
                    break;
                }
                if (flag) {
                    reply += allPeers[user].ip + ":" + to_string(allPeers[user].port);
                    flag = false;
                }
                else {
                    reply += " " + allPeers[user].ip + ":" + to_string(allPeers[user].port);
                }
            }
            reply += " " + to_string(allFiles[file_name].noOfChunks) + " " + to_string(allFiles[file_name].lastChunkSize);
            log.printLog("Download_file: " + reply);
            // Storing the user who is downloading the file even though the user hasn't started downloading yet.
            allFiles[file_name].users.push_back(username);
        }
    }
    send(desc, &reply[0], reply.length(), 0);
}

void Tracker::file_info(string file_name, string username, int desc) {
    string reply;
    fileInfo reqFile = allFiles[file_name];
    reply += "$";
    bool flag = true;
    for (int i = 0; i < reqFile.users.size(); i++) {
        string user = reqFile.users[i];
        if (user == username)
            continue;
        if (flag) {
            reply += allPeers[user].ip + ":" + to_string(allPeers[user].port);
            flag = false;
        }
        else {
            reply += " " + allPeers[user].ip + ":" + to_string(allPeers[user].port);
        }
    }
    reply += " " + to_string(allFiles[file_name].noOfChunks) + " " + to_string(allFiles[file_name].lastChunkSize);
    log.printLog("file_info: " + reply);
}

int main(int argc, char* argv[]) {
    if (argc != 3) {
        puts("Provide arguments <tracker_info_file> and <tracker_no>");
        exit(1);
    }
    Tracker t;
    t.parseArgs(argc, argv);
    t.exitThread();
    t.displayInfo();
    t.connectToPeer();
    return 0;
}
