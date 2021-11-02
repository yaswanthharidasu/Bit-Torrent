#include"common.h"

class Peer {
    string username, password;
    string peerIP, trackerIP;
    int peerPort, trackerPort;
    int peer_desc, tracker_desc;
    struct sockaddr_in trackerAddr, peerAddr;
    vector<string>trackerDetails;
    // vector<string>myAdminGroups, myGroups;

public:
    Peer();
    void parseArgs(int argc, char* argv[]);
    void connectToTracker();
    void displayInfo();
    void communicateWithTracker();
    void processInput(vector<string>& words, string& input);
    void processReply(vector<string>& words, string& reply);
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
    // else if (words[0] == CREATE_GROUP && reply == GROUP_REGISTER_SUCCESS) {
    //     myAdminGroups.push_back(words[1]);
    // }
    // else if (words[0] == JOIN_GROUP && reply == GROUP_JOIN_SUCCESS) {
    //     myGroups.push_back(words[1]);
    // }
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
