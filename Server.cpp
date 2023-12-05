#include <iostream>
#include <cstdio>
#include <sstream>
#include <fstream>
#include <cstdlib>
#include <cstring>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <pthread.h>
#include <unordered_map>
#include <mutex>
#include <valarray>
#include "Utilities.h"


using namespace std;

// This code section defines constants, a mutex, and functions related to user management,
// including creating and decrementing user sockets and calculating a timeout.

// Constant for the server IP address.
const char* SERVER_IP = "127.0.0.1";

// Maximum number of pending connections.
static const int MAX_PENDING = 5;

// Mutex for managing the active user count.
static mutex activeUsersMutex;

// Variable to keep track of the active user count.
static int activeUsers = 0;

// Function to calculate a timeout value based on the current number of active users.
int getTimeInSeconds() {
    // Calculate the extra time based on the number of active users.
    int baseTime = 25;
    double userFactor = 0.3;

    // Calculate the timeout value based on the number of active users.
    int time = static_cast<int>(baseTime - userFactor * log(activeUsers + 1));
    return max(5, time);
}

// Function to create a user socket and set a receive timeout and maximum number of requests.
void *createUserSocket(int clntSock, int &maxRequests) {
    // Lock the mutex to safely increment the active user count.
    lock_guard<mutex> lock(activeUsersMutex);
    activeUsers++;
    // Calculate the maximum number of requests based on the current number of active users.
    maxRequests = max(30, (maxRequests/activeUsers) * 2);
    // Set the receive timeout based on the current number of active users.
    struct timeval timeout{getTimeInSeconds(), 0};
    if (setsockopt(clntSock, SOL_SOCKET, SO_RCVTIMEO, (char *) &timeout, sizeof(timeout)) < 0)
        DieWithSystemMessage("setSockopt() failed");

    return nullptr;
}

// Function to decrement the active user count.
void *decrementActiveUsers() {
    // Lock the mutex to safely decrement the active user count.
    lock_guard<mutex> lock(activeUsersMutex);

    // Decrement the active user count.
    activeUsers--;

    // Return nullptr as there is no specific action in this function.
    return nullptr;
}

// This function handles a GET request from the client, sending an HTTP response.
// It checks if the requested file exists, and if so, sends a 200 OK response with
// the file content; otherwise, it sends a 404 Not Found response.
void* serverGet(int clntSock, unordered_map<string, string>& map) {
    // Create an output stream for constructing the HTTP response.
    ostringstream responseStream;

    // Open the requested file in binary mode.
    ifstream fileStream(map["arg2"].c_str(), ios::binary);

    // Check if the file is not found.
    if (!fileStream.is_open()) {
        // Send a 404 Not Found response.
        responseStream << "HTTP/1.1 404 Not Found\r\n";
        responseStream << "Content-Type: Unknown\r\n";
        responseStream << "Content-Length: 0" << "\r\n";
        responseStream << "Connection: keep-alive" << "\r\n\r\n";

        // Get the response string and convert it to a C-style string.
        string responseString = responseStream.str();
        const char *response = responseString.c_str();

        // Send the response to the client.
        ssize_t numBytesSent;
        size_t responseBytes = strlen(response);
        numBytesSent = send(clntSock, response, responseBytes, 0);

        // Check for errors in the send operation.
        if (numBytesSent < 0)
            DieWithSystemMessage("send() failed");
    } else {
        // Send a 200 OK response and the file content.
        responseStream << "HTTP/1.1 200 OK\r\n";
        responseStream << "Content-Type: " << map["Content-Type"] << "\r\n";
        SendFile(clntSock, responseStream, fileStream);
    }

    // Return nullptr as there is no specific action in this function.
    return nullptr;
}

// This function handles a POST request from the client, receiving a file and sending an HTTP response.
// It uses the ReceiveFile function to save the received file, then sends a 200 OK response to the client.
void* serverPost(int clntSock, char *buffer, ssize_t numBytesRecv, size_t headerBytes, unordered_map<string, string> map) {
    // Create an output stream for constructing the HTTP response.
    ostringstream responseStream;

    // Receive the file from the client using the ReceiveFile function.
    ReceiveFile(clntSock, buffer, map["arg2"].c_str(), map["Content-Length"].c_str(), numBytesRecv, headerBytes);

    // Construct a 200 OK response with Content-Type and Content-Length.
    responseStream << "HTTP/1.1 200 OK\r\n";
    responseStream << "Content-Type: " << map["Content-Type"] << "\r\n";
    responseStream << "Content-Length: 0" << "\r\n";
    responseStream << "Connection: keep-alive" << "\r\n\r\n";

    // Get the response header string and convert it to a C-style string.
    string responseHeader = responseStream.str();
    ssize_t numBytesSent = send(clntSock, responseHeader.c_str(), responseHeader.size(), 0);

    // Check for errors in the send operation.
    if (numBytesSent < 0)
        DieWithSystemMessage("send() failed");

    // Return nullptr as there is no specific action in this function.
    return nullptr;
}

// This function handles a client connection, receives and processes requests,
// and terminates the connection when the maximum number of requests is reached.
void *HandleClient(void *arg) {
    // Cast the argument to an integer pointer to obtain the client socket.
    int *clntSockPtr = static_cast<int *>(arg);
    int clntSock = *clntSockPtr;

    // Define a maximum number of requests for the client connection.
    int maxRequests = MAXREQUESTS;

    // Buffer to store received data.
    char buffer[BUFSIZE];

    // Create a user socket for the client connection.
    createUserSocket(clntSock, maxRequests);

    // Loop to handle requests until the maximum number is reached.
    while (maxRequests--) {
        // Receive data from the client.
        ssize_t numBytesRecv = recv(clntSock, buffer, BUFSIZE - 1, 0);

        // If the received bytes are less than or equal to 0, terminate the loop.
        // (connection terminated from the client = 0, timeout = -1)
        if (numBytesRecv <= 0)
            break;

        // Null-terminate the received data and parse the header.
        buffer[numBytesRecv] = '\0';
        size_t headerBytes = 0;
        unordered_map<string, string> map = ParseHeader(buffer, headerBytes);

        // Adjust the file path if it starts with a slash.
        if (map["arg2"][0] == '/')
            map["arg2"] = "." + map["arg2"];

        // Print the received data to stdout.
        fputs(buffer, stdout);

        // Process the request based on the HTTP method.
        if (map["arg1"] == "GET")
            serverGet(clntSock, map);
        else if (map["arg1"] == "POST")
            serverPost(clntSock, buffer, numBytesRecv, headerBytes, map);
        else
            break;
    }

    // Decrement the active user count, print a termination message, close the socket, and exit the thread.
    decrementActiveUsers();
    cout << "Connection terminated " << clntSock << endl;
    close(clntSock);
    pthread_exit(nullptr);
}

// This main function sets up a server socket, binds it to a specified port, listens for incoming connections,
// and handles each connection in a separate thread using the HandleClient function.
int main(int argc, char *argv[]) {
    // Check for the correct number of command-line arguments.
    if (argc != 2)
        DieWithUserMessage("Parameter(s)", "<Server Port>");

    // Extract the server port from the command-line argument.
    in_port_t servPort = atoi(argv[1]);

    // Create a TCP socket for the server.
    int servSock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (servSock < 0)
        DieWithSystemMessage("socket() failed");

    // Set up the server address structure.
    struct sockaddr_in servAddr{};
    CreateServerAddress(servAddr, SERVER_IP, servPort);

    // Bind the server socket to the specified address and port.
    if (bind(servSock, (struct sockaddr*) &servAddr, sizeof(servAddr)) < 0)
        DieWithSystemMessage("bind() failed");

    // Listen for incoming connections with a maximum pending connection queue.
    if (listen(servSock, MAX_PENDING) < 0)
        DieWithSystemMessage("listen() failed");

    // Main server loop to handle incoming connections.
    while (true) {
        // Accept an incoming connection and obtain the client address.
        struct sockaddr_in clntAddr{};
        socklen_t clntAddrLen = sizeof(clntAddr);
        int clntSock = accept(servSock, (struct sockaddr *) &clntAddr, &clntAddrLen);

        // Check for errors in accepting the connection.
        if (clntSock < 0)
            DieWithSystemMessage("accept() failed");

        // Print the client's IP address and port.
        char clntName[INET_ADDRSTRLEN];
        if (inet_ntop(AF_INET, &clntAddr.sin_addr.s_addr, clntName, sizeof(clntName)) != nullptr)
            printf("Handling client %s/%d\n", clntName, ntohs(clntAddr.sin_port));
        else
            puts("Unable to get client address");

        // Create a new thread to handle the client connection using the HandleClient function.
        pthread_t thread;
        if (pthread_create(&thread, nullptr, HandleClient, &clntSock) != 0)
            perror("pthread_create() failed");
        else
            pthread_detach(thread);
    }

    // Close the server socket.
    close(servSock);
    return 0;
}
