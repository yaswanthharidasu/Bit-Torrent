#include"common.h"

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

    struct fileInfo {
        string hash;
        string location;
        int noOfChunks;
        long long lastChunkSize;
        vector<bool> availableChunks;
    };

    ///////////// FILE_NAME, INFO ////////
    unordered_map<string, fileInfo> files;

    ////////// FILE_NAME, GROUP_NAME ////////
    unordered_map<string, string> downloading;
    unordered_map<string, string> completed;

    // Thread heplers
    struct threadArgs {
        Peer* p;
        int desc;
    };

public:
    Peer();
    void parseArgs(int argc, char* argv[]);
    void displayInfo();

    // ================================ Peer <--> Tracker communication methods ===================
    void connectToTracker();
    void communicateWithTracker();
    void processInput(vector<string>& words, string& input);
    void processReply(vector<string>& words, string& reply);

    // ================================= Peer <--> Peer communication Methods =====================
    void connectToPeer();
    void peerAsServer(int desc);
    void processCommand(string command, int desc);
    string communicateWithPeer(string message, string ip, int port);

    // ================================= Download_file helpers ====================================
    void download(int noOfChunks, long long lastChunkSize, string hash, vector<string> users, string destination);
    void download_file(int noOfChunks, long long lastChunkSize, string hash, vector<string> users, string destination);
    void getFileInfo(vector<vector<string>>& fileData, string hash, string ip, int port);
    void download_chunk(int chunk_no, long long chunk_size, vector<string>& chunkData, unordered_set<string>& us, string hash, string destination);
    void show_downloads();

    // ===================================== Thread Helpers =======================================
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
        if (words[0] == SHOW_DOWNLOADS)
            continue;
        send(tracker_desc, &input[0], input.length(), 0);
        memset(reply, 0, sizeof(reply));
        recv(tracker_desc, reply, sizeof(reply), 0);
        if ((words[0] != DOWNLOAD_FILE) || (words[0] == DOWNLOAD_FILE && reply[0] != '$'))
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
        newFile.hash = hash;
        string file_name = getFileName(words[1]);
        chunkDetails(words[1], newFile.noOfChunks, newFile.lastChunkSize);
        // As this peer is uploading the file, it'll have all the chunks
        for (int i = 0; i < newFile.noOfChunks; i++) {
            newFile.availableChunks.push_back(true);
        }
        files[file_name] = newFile;
        input += " " + username + " " + hash + " " + to_string(newFile.noOfChunks) + " " + to_string(newFile.lastChunkSize);
    }
    else if (words[0] == SHOW_DOWNLOADS) {
        show_downloads();
    }
}

void Peer::processReply(vector<string>& words, string& reply) {
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
        vector<string> users = splitString(reply);

        long long lastChunkSize = stoll(users.back());
        users.pop_back();
        int noOfChunks = stoi(users.back());
        users.pop_back();
        // Removing no.of chunks
        string file_name = words[2];
        string destination = words[3] + "/" + file_name;

        // Creating file first
        FILE* fp = fopen(&destination[0], "a");
        fclose(fp);

        // Updating the download files list: file_name and group name
        downloading[file_name] = words[1];

        download(noOfChunks, lastChunkSize, file_name, users, destination);
        cout << KGRN "Started Downloading..." RESET << endl;
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

void Peer::show_downloads() {
    // Print the downloading files 
    for (auto it : downloading) {
        string msg = KYEL "[D] [" + it.second + "] " + it.first + RESET;
        cout << msg << endl;
    }
    // Print the completed files 
    for (auto it : completed) {
        string msg = KGRN "[C] [" + it.second + "] " + it.first + RESET;
        cout << msg << endl;
    }
}

void Peer::download(int noOfChunks, long long lastChunkSize, string file_name, vector<string> users, string destination) {
    // Multiple files can be downloaded at the same time.
    // Hence, creating thread for each download
    thread t(&Peer::download_file, this, noOfChunks, lastChunkSize, file_name, users, destination);
    t.detach();
}

void Peer::download_file(int noOfChunks, long long lastChunkSize, string file_name, vector<string> users, string destination) {
    fileInfo newFile;
    newFile.location = destination;
    newFile.noOfChunks = noOfChunks;
    newFile.lastChunkSize = lastChunkSize;
    vector<bool> availableChunks(noOfChunks, false);
    newFile.availableChunks = availableChunks;
    files[file_name] = newFile;

    // Creating string of vectors for each chunk
    // Chunk1 -> IP:PORT, IP:PORT....
    vector<vector<string>> fileData(noOfChunks);
    // Find which chunks are available at which peer
    vector<thread> userThreads;
    log.printLog("===================\nFile Info from Tracker: \n");
    // cout << "File info from Tracker:" << endl;
    for (int i = 0; i < users.size(); i++) {
        log.printLog("User: " + users[i] + "\n");
        // cout << "users[i]: " << users[i] << endl;
        int j = users[i].find_last_of(':');
        string ip = users[i].substr(0, j);
        int port = stoi(users[i].substr(j + 1));
        userThreads.push_back(thread(&Peer::getFileInfo, this, std::ref(fileData), file_name, ip, port));
    }
    for (int i = 0; i < userThreads.size(); i++) {
        userThreads[i].join();
    }
    userThreads.clear();

    log.printLog("===================\nChunk Info from Peers: \n");
    for (int i = 0; i < fileData.size(); i++) {
        // cout << "Chunk " << i << ": ";
        log.printLog("Chunk " + to_string(i) + " ");
        for (auto it : fileData[i]) {
            // cout << it << "\t";
            log.printLog(it + " ");
        }
        // cout << endl;
        log.printLog("\n");
    }

    // Sort the chunks based on their availability
    // Rarest chunks will be at starting indices after sorting
    sort(fileData.begin(), fileData.end(), compareSizes);
    unordered_set<string> us;

    // char file[noOfChunks - 1][CHUNK_SIZE];
    // char last[1][CHUNK_SIZE];

    // Now download each chunk 
    for (int i = 0; i < fileData.size(); i++) {
        if (i == fileData.size() - 1) {
            userThreads.push_back(thread(&Peer::download_chunk, this, i, lastChunkSize, std::ref(fileData[i]), std::ref(us), file_name, destination));
        }
        else {
            userThreads.push_back(thread(&Peer::download_chunk, this, i, CHUNK_SIZE, std::ref(fileData[i]), std::ref(us), file_name, destination));
        }
    }

    for (int i = 0; i < userThreads.size(); i++) {
        userThreads[i].join();
    }

    // File completed downloading. Adding it to the completed list
    completed[file_name] = downloading[file_name];
    // Removing from the downloading list
    downloading.erase(file_name);
}

void Peer::getFileInfo(vector<vector<string>>& fileData, string file_name, string ip, int port) {
    string message = GET_FILE_INFO;
    message += " " + file_name;
    string reply = communicateWithPeer(message, ip, port);
    // cout << "Reply: " << reply << endl;
    if (reply != OFFLINE) {
        // Reply: <no_of_chunks> <chunk_1_yes/no> <chunk_2> ...
        // Ex: 5 1 1 1 0 1  
        vector<string> values = splitString(reply);
        int noOfChunks = stoi(values[0]);
        values.erase(values.begin());
        for (int i = 0; i < values.size(); i++) {
            if (values[i][0] - '0' == 1)
                fileData[i].push_back(ip + ":" + to_string(port));
        }
    }
}

void Peer::download_chunk(int chunk_no, long long chunk_size, vector<string>& chunkData, unordered_set<string>& us, string file_name, string destination) {
    bool downloaded = false;
    int tries = 0;
    while (!downloaded) {
        // No peers to send the chunk
        while (chunkData.size() == 0 || (tries == 2 * chunkData.size())) {
            // Communicate with the tracker to get the chunk info
            log.printLog("File " + file_name + ": Waiting for others to upload \n");
            string message = GET_CHUNK_INFO " " + file_name + " " + username;
            string reply = communicateWithPeer(message, trackerIP, trackerPort);
            // cout << "download_chunk: " << reply << endl;
            reply = reply.substr(1);
            vector<string> words = splitString(reply);
            chunkData = words;
            // If still there are no peers to send the chunk
            if (chunkData.size() == 0) {
                this_thread::sleep_for(chrono::seconds(5));
            }
            tries = 0;
        }

        int i = rand() % chunkData.size();
        string message;
        // Command: download_chunk <file_hash> <destination> <chunk_no> <chunk_size>
        message += DOWNLOAD_CHUNK " " + file_name + " " + destination + " " + to_string(chunk_no) + " " + to_string(chunk_size);
        int j = chunkData[i].find_last_of(':');
        string ip = chunkData[i].substr(0, j);
        int port = stoi(chunkData[i].substr(j + 1));
        log.printLog("Downloading chunk " + to_string(chunk_no) + " from IP: " + ip + " PORT: " + to_string(port) + "\n");
        // cout << chunk_no << " " << ip << " " << port << endl;
        string reply = communicateWithPeer(message, ip, port);
        if (reply == DOWNLOAD_CHUNK_SUCCESS) {
            log.printLog("Reply for chunk " + to_string(chunk_no) + ": Download Success \n");
            // cout << "reply: " << reply << endl;
            files[file_name].availableChunks[chunk_no] = true;
            downloaded = true;
            break;
        }
        tries++;
    }
    // communicateWithPeer();
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
    // serverAddr.sin_addr.s_addr = INADDR_ANY;
    if (inet_pton(AF_INET, &ip[0], &serverAddr.sin_addr.s_addr) != 1) {
        perror("pton");
        exit(1);
    }

    // Assigning custom IP and port to the Client Peer
    // clientAddr.sin_family = AF_INET;
    // clientAddr.sin_port = htons(peerPort);
    // if (inet_pton(AF_INET, &peerIP[0], &clientAddr.sin_addr.s_addr) != 1) {
    //     perror("pton");
    //     exit(1);
    // }
    // // Binding the IP and port to the Client Peer
    // if (bind(desc, (struct sockaddr*)&clientAddr, sizeof(clientAddr)) != 0) {
    //     perror("client addr bind");
    //     exit(1);
    // }

    // Connecting to Peer server
    if (connect(desc, (struct sockaddr*)&serverAddr, sizeof(serverAddr)) == -1) {
        perror("Failed to connect server");
        exit(1);
    }

    if (send(desc, &message[0], message.length(), 0) < 0) {
        perror("Failed to send data to server");
        exit(1);
    }

    char reply[CHUNK_SIZE];
    // Receiving data from server
    if (recv(desc, reply, sizeof(reply), 0) < 0) {
        perror("Failed to receive data to server");
        exit(1);
    }
    close(desc);
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

    // Listen for the connections
    listen(peerServer_desc, 100);
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
    // pthread_join(id, NULL);
}

// Function handler for Server Peer --> Thread for each Client Peer 
void Peer::peerAsServer(int desc) {
    // while (true) {
    // }
    char command[2000];
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
    else if (words[0] == GET_FILE_INFO) {
        if (loggedIn == false) {
            reply = OFFLINE;
        }
        else {
            fileInfo info = files[words[1]];
            reply += to_string(info.noOfChunks);
            for (int i = 0; i < info.availableChunks.size(); i++) {
                reply += " " + to_string(int(info.availableChunks[i]));
            }
        }
    }
    else if (words[0] == GET_CHUNK_INFO) {

    }
    else if (words[0] == DOWNLOAD_CHUNK) {
        if (loggedIn == false) {
            reply = OFFLINE;
        }
        else {
            // Command: download_chunk <file_name> <destination> <chunk_no> <chunk_size>
            fileInfo reqFile = files[words[1]];
            int chunk_no = stoi(words[3]);
            long long chunkSize = stoll(words[4]);
            char buff[CHUNK_SIZE];
            // Checking if the peer has that chunk or not
            if (reqFile.availableChunks[chunk_no] == false) {
                reply = CHUNK_UNAVAILABLE;
            }
            else {
                FILE* src = fopen(&reqFile.location[0], "r");
                FILE* dest = fopen(&words[2][0], "r+");

                log.printLog("Chunk " + to_string(chunk_no) + " of size: " + to_string(chunkSize));
                fseek(src, (chunk_no * CHUNK_SIZE), SEEK_SET);
                fseek(dest, (chunk_no * CHUNK_SIZE), SEEK_SET);
                int readBytes = fread(buff, 1, chunkSize, src);
                int writeBytes = fwrite(buff, 1, chunkSize, dest);
                fclose(src);
                fclose(dest);
                if (readBytes == chunkSize && writeBytes == chunkSize)
                    reply = DOWNLOAD_CHUNK_SUCCESS;
                else
                    reply = DOWNLOAD_CHUNK_FAILED;
                log.printLog("\n");
            }
        }
    }
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
