#include"common.h"
class Peer {
    Log log;

    // IP & Port details of tracker and peer
    vector<string>trackerDetails;
    string peerIP, trackerIP;
    int peerPort, trackerPort;

    // Username and password of peer
    string username, password;

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

public:
    Peer();
    void parseArgs(int argc, char* argv[]);
    void displayInfo();

    // Peer <--> Tracker communication methods
    void connectToTracker();
    void communicateWithTracker();
    void processInput(vector<string>& words, string& input);
    void processReply(vector<string>& words, string& reply);

    // Peer <--> Peer communication Methods
    void connectToPeer();
    void peerAsServer(int desc);
    void processCommand(string command, int desc);

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
        words[0] == ACCEPT_REQUEST
        ) {
        input += " " + username;
    }
}

void Peer::processReply(vector<string>& words, string& reply) {
    if ((words[0] == CREATE_USER && reply == USER_REGISTER_SUCCESS) ||
        (words[0] == LOGIN && reply == LOGIN_SUCCESS)) {
        username = words[1];
        password = words[2];
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
        log.printLog("Incoming connection from IP: " + string(inet_ntoa(peerPeerAddr.sin_addr))
            + " PORT: " + to_string(ntohs(peerPeerAddr.sin_port)) + "\n");

        // Creating threads for each peer
        threadArgs* args = new threadArgs();
        args->p = this;
        args->desc = peerPeer_desc;
        pthread_create(&id, NULL, &Peer::peerServerHandler, (void*)args);
    }
}

void Peer::peerAsServer(int desc) {
    log.printLog("peerasServer\n");
    while (true) {
        char command[200];
        memset(command, 0, sizeof(command));
        // Read command from peer
        recv(desc, command, sizeof(command), 0);
        if (strlen(command) <= 0)
            continue;
        log.printLog(command);
        processCommand(command, desc);
    }
}

void Peer::processCommand(string command, int desc) {
    vector<string> words = splitString(command);
    string reply;
    if (words[0] == "send_message") {
        reply = "Received your message";
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
