#include"common.h"
#include<openssl/sha.h>

class Peer {
    Log log;

    // IP & Port details of tracker and peer
    vector<string>trackerDetails;
    string peerIP, trackerIP;
    int peerPort, trackerPort;

    // Username and password of peer
    string username, password;
    bool loggedIn;

    // Peer <--> Tracker details
    int peer_desc, tracker_desc;
    struct sockaddr_in trackerAddr, peerAddr;

    // Peer <--> Peer details
    int peerServer_desc, peerPeer_desc;
    struct sockaddr_in peerServerAddr, peerPeerAddr;

    struct threadArgs {
        Peer* p;
        int desc;
    };

    struct fileInfo {
        string location;
        int noOfChunks;
        long long lastChunkSize;
        vector<bool> availableChunks;
    };

    unordered_map<string, fileInfo> files;


public:
    Peer();
    void parseArgs(int argc, char* argv[]);
    void displayInfo();
    void download(int noOfChunks, string hash, vector<string> users);
    void download_file(int noOfChunks, string hash, vector<string> users);
    void getChunkInfo(vector<vector<string>>& chunkData, string hash, string ip, int port);
    void download_chunk(vector<string> chunkDetails);

    // Peer <--> Tracker communication methods
    void connectToTracker();
    void communicateWithTracker();
    void processInput(vector<string>& words, string& input);
    void processReply(vector<string>& words, string& reply);

    // Peer <--> Peer communication Methods
    void connectToPeer();
    void peerAsServer(int desc);
    void processCommand(string command, int desc);
    string communicateWithPeer(string message, string ip, int port);


    static void* createServer(void* ptr) {
        ((Peer*)ptr)->connectToPeer();
        return 0;
    }
    static void* peerServerHandler(void* ptr) {
        int desc = ((struct threadArgs*)ptr)->desc;
        ((struct threadArgs*)ptr)->p->peerAsServer(desc);
        return 0;
    }
};

Peer::Peer() {
    username = "###";
    password = "###";
    loggedIn = false;
}

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
    printf("\t%sPEER%s ---------> %sIP:%s %s %sPORT:%s %d\n", KRED, RESET, KYEL, RESET, &peerIP[0], KYEL, RESET, peerPort);
    printf("%s------------------------------------------------------------%s\n", KYEL, RESET);
}

void Peer::connectToTracker() {
    // Peer as server
    pthread_t id;
    pthread_create(&id, NULL, &Peer::createServer, (void*)this);

    // Create Socket
    if ((tracker_desc = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
        perror("Create socket");
        exit(1);
    }

    int option_value = 1;
    if (setsockopt(tracker_desc, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &option_value, sizeof(option_value)) == -1) {
        perror("Socket options");
        exit(1);
    }

    // Assign the IP address and port number 
    trackerAddr.sin_family = AF_INET;
    trackerAddr.sin_port = htons(trackerPort);
    if (inet_pton(AF_INET, &trackerIP[0], &trackerAddr.sin_addr.s_addr) != 1) {
        perror("pton");
        exit(1);
    }

    // Assigning custom IP and port to the peer
    peerAddr.sin_family = AF_INET;
    peerAddr.sin_port = htons(peerPort);
    if (inet_pton(AF_INET, &peerIP[0], &peerAddr.sin_addr.s_addr) != 1) {
        perror("pton");
        exit(1);
    }
    // Binding the IP and port to the peer
    bind(tracker_desc, (struct sockaddr*)&peerAddr, sizeof(peerAddr));

    // Connecting to server
    if (connect(tracker_desc, (struct sockaddr*)&trackerAddr, sizeof(trackerAddr)) == -1) {
        puts("Failed to connect server");
        exit(1);
    }
    puts("Connected to tracker...");

    communicateWithTracker();
}

void Peer::communicateWithTracker() {
    char reply[200];
    while (true) {
        string input;
        cout << ">> ";
        getline(cin, input);
        if (input.length() <= 0)
            continue;
        vector<string> words;
        processInput(words, input);
        send(tracker_desc, &input[0], input.length(), 0);
        memset(reply, 0, sizeof(reply));
        recv(tracker_desc, reply, sizeof(reply), 0);
        if (words[0] != DOWNLOAD_FILE && reply[0] != '$')
            puts(reply);
        string res = reply;
        processReply(words, res);
    }
}

void Peer::processInput(vector<string>& words, string& input) {
    words = splitString(input);
    if (words[0] == LOGIN) {
        input += " " + peerIP + " " + to_string(peerPort);
    }
    else if (words[0] == LOGOUT ||
        words[0] == LIST_USERS ||
        words[0] == CREATE_GROUP ||
        words[0] == JOIN_GROUP ||
        words[0] == LEAVE_GROUP ||
        words[0] == LIST_REQUESTS ||
        words[0] == ACCEPT_REQUEST ||
        words[0] == DOWNLOAD_FILE
        ) {
        input += " " + username;
    }
    else if (words[0] == UPLOAD_FILE) {
        // Storing the file details on the peer side
        string hash = SHA1::from_file(words[1]);
        fileInfo newFile;
        newFile.location = words[1];
        chunkDetails(words[1], newFile.noOfChunks, newFile.lastChunkSize);
        // As this peer is uploading the file, it'll have all the chunks
        for (int i = 0; i < newFile.noOfChunks; i++) {
            newFile.availableChunks.push_back(true);
        }
        files[hash] = newFile;
        input += " " + username + " " + hash + " " + to_string(newFile.noOfChunks) + " " + to_string(newFile.lastChunkSize);
    }
}

void Peer::processReply(vector<string>& words, string& reply) {
    // cout << "reply:" << reply << endl;
    if (words[0] == CREATE_USER && reply == USER_REGISTER_SUCCESS) {
        username = words[1];
        password = words[2];
    }
    else if (words[0] == LOGIN && reply == LOGIN_SUCCESS) {
        username = words[1];
        password = words[2];
        loggedIn = true;
    }
    else if (words[0] == LOGOUT && reply == LOGOUT_SUCCESS) {
        username = "###";
        password = "###";
        loggedIn = false;
    }
    else if (words[0] == DOWNLOAD_FILE && reply[0] == '$') {
        // Removing '$' at the start
        reply = reply.substr(1);
        // Storing the hash
        string hash = reply.substr(0, 40);
        // Removing the hash from reply
        reply = reply.substr(41);
        vector<string> users = splitString(reply);
        int noOfChunks = stoi(users[0]);
        // Removing no.of chunks
        users.erase(users.begin());
        download(noOfChunks, hash, users);
    }
    else if ((words[0] == "send_message")) {
        // Create Socket
        vector<string> replyWords = splitString(reply);
        string myIP = replyWords[0];
        int myPort = stoi(replyWords[1]);

        int mydesc;
        struct sockaddr_in myTrackerAddr, myPeerAddr;
        if ((mydesc = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
            perror("Create socket");
            exit(1);
        }

        int option_value = 1;
        if (setsockopt(mydesc, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &option_value, sizeof(option_value)) == -1) {
            perror("Socket options");
            exit(1);
        }

        // Assign the IP address and port number of the server
        myTrackerAddr.sin_family = AF_INET;
        myTrackerAddr.sin_port = htons(myPort);
        if (inet_pton(AF_INET, &myIP[0], &myTrackerAddr.sin_addr.s_addr) != 1) {
            perror("pton");
            exit(1);
        }

        // Assigning custom IP and port to the peer
        myPeerAddr.sin_family = AF_INET;
        myPeerAddr.sin_port = htons(peerPort);
        if (inet_pton(AF_INET, &peerIP[0], &myPeerAddr.sin_addr.s_addr) != 1) {
            perror("pton");
            exit(1);
        }
        // Binding the IP and port to the peer
        bind(mydesc, (struct sockaddr*)&myPeerAddr, sizeof(myPeerAddr));

        // Connecting to server
        if (connect(mydesc, (struct sockaddr*)&myTrackerAddr, sizeof(myTrackerAddr)) == -1) {
            puts("Failed to connect server");
            exit(1);
        }

        string msg = "send_message hello";
        send(mydesc, &msg[0], msg.length(), 0);
        char replyFromPeer[200];
        recv(mydesc, replyFromPeer, sizeof(replyFromPeer), 0);
        puts(replyFromPeer);
    }
    // else if (words[0] == CREATE_GROUP && reply == GROUP_REGISTER_SUCCESS) {
    //     myAdminGroups.push_back(words[1]);
    // }
    // else if (words[0] == JOIN_GROUP && reply == GROUP_JOIN_SUCCESS) {
    //     myGroups.push_back(words[1]);
    // }
}

void Peer::download(int noOfChunks, string hash, vector<string> users) {
    // Multiple files can be downloaded at the same time.
    // Hence, creating thread for each download
    thread t(&Peer::download_file, this, noOfChunks, hash, users);
    t.join();
}

void Peer::download_file(int noOfChunks, string hash, vector<string> users) {
    // Creating string of vectors for each chunk
    // Chunk1 -> IP:PORT, IP:PORT....
    vector<vector<string>> chunkData(noOfChunks);
    // Find which chunks are available at which peer
    vector<thread> userThreads;
    for (int i = 0; i < users.size(); i++) {
        int j = users[i].find_last_of(':');
        string ip = users[i].substr(0, j);
        int port = stoi(users[i].substr(j + 1));
        userThreads.push_back(thread(&Peer::getChunkInfo, this, std::ref(chunkData), hash, ip, port));
    }
    for (int i = 0; i < userThreads.size(); i++) {
        userThreads[i].join();
    }

    // for (int i = 0; i < chunkData.size(); i++) {
    //     cout << "Chunk " << i << ":";
    //     for (auto it : chunkData[i]) {
    //         cout << it << " ";
    //     }
    //     cout << endl;
    // }
    
    // Sort the chunks based on their availability
    // Rarest chunks will be at starting indices after sorting
    sort(chunkData.begin(), chunkData.end(), compareSizes);

    // Now download each chunk 
    for(int i=0; i<chunkData.size(); i++){
        userThreads.push_back(thread(&Peer::download_chunk, this, std::ref(chunkData[i])));
    }
}

void Peer::getChunkInfo(vector<vector<string>>& chunkData, string hash, string ip, int port) {
    string message = "get_chunk_info";
    message += " " + hash;
    string reply = communicateWithPeer(message, ip, port);
    // cout << "Reply: " << reply << endl;
    int noOfChunks = reply[0] - '0';
    reply = reply.substr(2);
    vector<string> values = splitString(reply);
    for (int i = 0; i < values.size(); i++) {
        chunkData[i].push_back(ip + ":" + to_string(port));
    }
}

// Client Peer --> Connecting to Server Peer 
string Peer::communicateWithPeer(string message, string ip, int port) {
    struct sockaddr_in serverAddr, clientAddr;
    int desc;
    // Create Socket
    if ((desc = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
        perror("Create socket");
        exit(1);
    }

    int option_value = 1;
    if (setsockopt(desc, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &option_value, sizeof(option_value)) == -1) {
        perror("Socket options");
        exit(1);
    }

    // Assign the IP address and port number 
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(port);
    if (inet_pton(AF_INET, &ip[0], &serverAddr.sin_addr.s_addr) != 1) {
        perror("pton");
        exit(1);
    }

    // Assigning custom IP and port to the Client Peer
    clientAddr.sin_family = AF_INET;
    clientAddr.sin_port = htons(peerPort);
    if (inet_pton(AF_INET, &peerIP[0], &clientAddr.sin_addr.s_addr) != 1) {
        perror("pton");
        exit(1);
    }
    // Binding the IP and port to the Client Peer
    bind(desc, (struct sockaddr*)&clientAddr, sizeof(clientAddr));

    // Connecting to Peer server
    if (connect(desc, (struct sockaddr*)&serverAddr, sizeof(serverAddr)) == -1) {
        puts("Failed to connect server");
        exit(1);
    }

    if (send(desc, &message[0], message.length(), 0) < 0) {
        cout << "Failed to send data to server" << endl;
        exit(1);
    }

    char reply[2000];
    // Receiving data from server
    if (recv(desc, reply, sizeof(reply), 0) < 0) {
        cout << "Failed to receive data from server" << endl;
        exit(1);
    }
    return reply;
}

// ===================================== Peer As Server methods ====================================================

// Server Peer --> Listening for Client Peer connections
void Peer::connectToPeer() {
    log.setFileName("peer_log_" + to_string(peerPort) + ".txt");
    log.clearLog();
    if ((peerServer_desc = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
        perror("Create socket");
        exit(1);
    }
    log.printLog("Socket Created\n");

    // Set the socket options
    // IP Address and port can be reused .i.e. value of those two options are being set to 1
    int option_value = 1;
    if (setsockopt(peerServer_desc, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &option_value, sizeof(option_value)) == -1) {
        perror("Socket options");
        exit(1);
    }
    log.printLog("Socket options set\n");

    // Assign the IP address and port number 
    peerServerAddr.sin_family = AF_INET;
    peerServerAddr.sin_port = htons(peerPort);
    if (inet_pton(AF_INET, &peerIP[0], &peerServerAddr.sin_addr.s_addr) != 1) {
        perror("pton");
        exit(1);
    }

    // Bind socket to the tracker port
    if (bind(peerServer_desc, (struct sockaddr*)&peerServerAddr, sizeof(peerServerAddr)) < 0) {
        perror("Bind socket");
        exit(1);
    }
    log.printLog("Binded the socket\n");

    // Listen for the connectToPeputsers
    listen(peerServer_desc, 10);
    log.printLog("Listening...\n");

    int p = sizeof(struct sockaddr_in);
    pthread_t id;
    while (true) {
        // Accepting the connection
        if ((peerPeer_desc = accept(peerServer_desc, (struct sockaddr*)&peerPeerAddr, (socklen_t*)&p)) < 0) {
            perror("Accept peer Connection");
            exit(1);
        }
        log.printLog("========================================================\n");
        log.printLog("Incoming connection from IP: " + string(inet_ntoa(peerPeerAddr.sin_addr))
            + " PORT: " + to_string(ntohs(peerPeerAddr.sin_port)) + "\n");
        log.printLog("========================================================\n");

        // Creating threads for each peer
        threadArgs* args = new threadArgs();
        args->p = this;
        args->desc = peerPeer_desc;
        pthread_create(&id, NULL, &Peer::peerServerHandler, (void*)args);
    }
    pthread_join(id, NULL);
}

// Function handler for Server Peer --> Thread for each Client Peer 
void Peer::peerAsServer(int desc) {
    // while (true) {
    // }
    char command[200];
    memset(command, 0, sizeof(command));
    // Read command from peer
    recv(desc, command, sizeof(command), 0);
    if (strlen(command) > 0) {
        log.printLog(command);
        log.printLog("\n");
        processCommand(command, desc);
    }
    close(desc);
}

// Processing the command sent to the Server Peer 
void Peer::processCommand(string command, int desc) {
    vector<string> words = splitString(command);
    string reply;
    if (words[0] == "send_message") {
        reply = "Received your message";
    }
    else if (words[0] == "get_chunk_info") {
        if (loggedIn == false) {
            reply = "Offline";
        }
        else {
            fileInfo info = files[words[1]];
            reply += to_string(info.noOfChunks);
            for (int i = 0; i < info.availableChunks.size(); i++) {
                reply += " " + to_string(int(info.availableChunks[i]));
            }
        }
    }
    log.printLog("Sent Reply\n");
    send(desc, &reply[0], reply.length(), 0);
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

    return 0;
}
