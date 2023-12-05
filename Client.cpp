#include <iostream>
#include <fstream>
#include <sstream>
#include <cstdio>
#include <cstdlib>
#include <unistd.h>
#include <cstring>
#include <sys/socket.h>
#include <netinet/in.h>
#include "Utilities.h"


using namespace std;

// Constant string for the default file name "input".
const string FILE_NAME = "input";

// Mapping file extensions to MIME types using an unordered map.
const unordered_map<string, string> mimeMapper = {
        { "html", "text/html" },
        { "txt", "text/plain" },
        { "png", "image/png" },
        { "jpg", "image/jpeg" }
};

// Function to get MIME type based on the file extension.
const char *getMimeType(const char* filePath) {
    // Find the last dot in the file path to determine the file extension.
    const char* lastDot = strrchr(filePath, '.');

    // If a dot is found, extract the extension and check if it is in the mimeMapper.
    if (lastDot != nullptr) {
        const char *extension = lastDot + 1;
        auto it = mimeMapper.find(extension);
        if (it != mimeMapper.end())
            return (it->second).c_str();
    }

    // Default MIME type is "application/json" if the extension is not found.
    return "application/json";
}

// Function to parse the input line into HTTP method and path.
void ParseInput(const string& line, string& method, string& path) {
    istringstream iss(line);
    iss >> method >> path;
}

// This function performs a GET request to the server for the specified path.
// It constructs an HTTP request header, sends it to the server, receives the
// server's response, parses the header, prints it to the standard output, and
// proceeds to receive the file content.
void clientGet(int sock, const char *path) {
    // Construct the GET request header with path, content type, content length, and connection details.
    ostringstream requestStream;
    requestStream << "GET " << path << " HTTP/1.1\r\n";
    requestStream << "Content-Type: " << getMimeType(path) << "\r\n";
    requestStream << "Content-Length: 0" << "\r\n";
    requestStream << "Connection: keep-alive" << "\r\n\r\n";

    // Get the request header as a string
    string requestHeader = requestStream.str();

    // Send the GET request header to the server
    ssize_t numBytesSent = send(sock, requestHeader.c_str(), requestHeader.size(), 0);
    if (numBytesSent < 0)
        DieWithSystemMessage("send() failed");

    // Receive the server's response into a buffer
    char buffer[BUFSIZE];
    ssize_t numBytesRecv = recv(sock, buffer, BUFSIZE - 1, 0);

    // Null-terminate the received data and parse the header to extract relevant information
    buffer[numBytesRecv] = '\0';
    size_t headerBytes = 0;
    unordered_map<string, string> map = ParseHeader(buffer, headerBytes);

    // Print the server's response header to the standard output
    fputs(buffer, stdout);

    // Receive the file content based on the server's response header
    ReceiveFile(sock, buffer, path, map["Content-Length"].c_str(), numBytesRecv, headerBytes);
}

// This function performs a POST request to the server for the specified path.
// It constructs an HTTP request header, opens the file to be sent, attaches
// content type information, sends the file to the server, receives the server's
// response, and prints it to the standard output.
void clientPost(int sock, const char *path) {
    // Create an output stream to build the POST request header
    ostringstream requestStream;

    // Open the file to be sent in binary mode
    ifstream fileStream(path, ios::binary);

    // Check if the file is successfully opened
    if (!fileStream.is_open())
        DieWithSystemMessage("open() failed");

    // Construct the POST request header with path and content type
    requestStream << "POST " << path << " HTTP/1.1\r\n";
    requestStream << "Content-Type: " << getMimeType(path) << "\r\n";

    // Send the file using the SendFile function
    SendFile(sock, requestStream, fileStream);

    // Receive the server's response into a buffer
    char buffer[BUFSIZE];
    ssize_t numBytesRecv = recv(sock, buffer, BUFSIZE - 1, 0);

    // Check for errors in the receive operation
    if (numBytesRecv < 0)
        DieWithSystemMessage("recv() failed");

    // Null-terminate the received data and print it to the standard output
    buffer[numBytesRecv] = '\0';
    fputs(buffer, stdout);
}

// The main function establishes a TCP connection to a server using the provided
// command-line arguments (server IP and port). It reads input from a file line by
// line, extracts the method and path information, and performs corresponding client
// actions (GET or POST) based on the input. It then closes the input file, the socket,
// and exits.
int main(int argc, char *argv[]) {
    // Check for the correct number of command-line arguments
    if (argc != 3)
        DieWithUserMessage("Parameter(s)", "<Server IP> <Port>");

    // Extract server IP and port from command-line arguments
    char* servIP = argv[1];
    in_port_t servPort = atoi(argv[2]);

    // Create a TCP socket
    int sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (sock < 0)
        DieWithSystemMessage("socket() failed");

    // Set up the server address structure and connect to the server
    struct sockaddr_in servAddr{};
    CreateServerAddress(servAddr, servIP, servPort);

    if (connect(sock, (struct sockaddr *) &servAddr, sizeof(servAddr)) < 0)
        DieWithSystemMessage("connect() failed");

    // Open the input file and check if it is successfully opened
    ifstream inputFile(FILE_NAME);
    if (!inputFile.is_open())
        DieWithUserMessage("File open failed", FILE_NAME.c_str());

    // Read each line from the input file and perform corresponding client actions
    string line;
    while (getline(inputFile, line)) {
        string method, path;
        ParseInput(line, method, path);

        // Validate the method and perform the corresponding client action
        if (method != "client_get" && method != "client_post")
            DieWithUserMessage("Invalid method", method.c_str());

        if (method == "client_get")
            clientGet(sock, path.c_str());
        else if (method == "client_post")
            clientPost(sock, path.c_str());
    }

    // Close the input file and the socket, then exit the program
    inputFile.close();
    close(sock);
    exit(0);
}