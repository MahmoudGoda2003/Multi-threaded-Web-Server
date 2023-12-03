#include <iostream>
#include <fstream>
#include <sstream>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include "Practical.h"

using namespace std;

const string FILE_NAME = "input";

void parse(const string& line, string& method, string& path, string& host) {
    istringstream iss(line);
    iss >> method >> path >> host;
}

void client_get(int sock, const char *path, const char *host) {
    // Construct the HTTP request
    char request[BUFSIZE];
    snprintf(request, BUFSIZE,
             "GET %s HTTP/1.1\r\n"
             "Host: %s\r\n"
             "Connection: keep-alive\r\n",
             "Content-Length: %s\r\n\r\n",
             path, host);

    ssize_t numBytes = send(sock, request, strlen(request), 0);
    if (numBytes < 0)
        DieWithSystemMessage("send() failed");

    char buffer[BUFSIZE];
    while ((numBytes = recv(sock, buffer, BUFSIZE - 1, 0)) > 0) {
        buffer[numBytes] = '\0';
        fputs(buffer, stdout);
    }

    if (numBytes < 0)
        DieWithSystemMessage("recv() failed");

    close(sock);
}

void client_post(int sock, const char *path, const char *host) {
    // Construct the HTTP request
    char httpRequest[BUFSIZE];
    snprintf(httpRequest, BUFSIZE,
             "GET %s HTTP/1.1\r\n"
             "Host: %s\r\n"
             "Connection: close\r\n\r\n",
             path, host);

    // Send the HTTP request to the server
    ssize_t numBytes = send(sock, httpRequest, strlen(httpRequest), 0);
    if (numBytes < 0)
        DieWithSystemMessage("send() failed");



    char buffer[BUFSIZE];
    // Receive and print the HTTP response from the server
    while ((numBytes = recv(sock, buffer, BUFSIZE - 1, 0)) > 0) {
        buffer[numBytes] = '\0';
        fputs(buffer, stdout);
    }

    if (numBytes < 0)
        DieWithSystemMessage("recv() failed");

    close(sock);
}

int main(int argc, char *argv[]) {
    if (argc < 2 || argc > 3)
        DieWithUserMessage("Parameter(s)", "<Server Address> <Host> <Path> [<Server Port>]");

    char* servIP = argv[1];
    in_port_t servPort = (argc == 3) ? atoi(argv[2]) : 80;

    ifstream inputFile(FILE_NAME);
    if (!inputFile.is_open()) {
        cerr << "Failed to open the file: " << FILE_NAME << endl;
        exit(1);
    }

    string line;

    while (getline(inputFile, line)) {
        string method, path, host;
        parse(line, method, path, host);
        cout << "method: " << method << endl;
        cout << "path: " << path << endl;
        cout << "host: " << host << endl;

        if (method != "client_get" && method != "client_post") {
            cerr << "Invalid method: " << method << endl;
            inputFile.close();
            exit(1);
        }

        int sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
        if (sock < 0)
            DieWithSystemMessage("socket() failed");

        struct sockaddr_in servAddr{};
        CreateServerAddress(servAddr, servIP, servPort);
        
        if (connect(sock, (struct sockaddr *) &servAddr, sizeof(servAddr)) < 0)
            DieWithSystemMessage("connect() failed");

        if (method == "client_get")
            client_get(sock, path.c_str(), host.c_str());
        else if (method == "client_post")
            client_post(sock, path.c_str(), host.c_str());
    }

    inputFile.close();
    exit(0);
}