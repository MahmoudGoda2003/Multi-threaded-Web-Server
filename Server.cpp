#include <cstdio>
#include <cstdlib>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <thread>
#include <vector>
#include <cstring>
#include "Practical.h"

const char* SERVER_IP = "127.0.0.1";
static const int MAX_PENDING = 5;

using namespace std;

void HandleClient(int clntSocket) {
    char buffer[BUFSIZE];
    ssize_t numBytesRcvd;

    numBytesRcvd = recv(clntSocket, buffer, BUFSIZE - 1, 0);
    if (numBytesRcvd < 0)
        DieWithSystemMessage("recv() failed");
    else if (numBytesRcvd == 0)
        DieWithUserMessage("recv()", "connection closed prematurely");

    buffer[numBytesRcvd] = '\0';
    fputs(buffer, stdout);

    char response[BUFSIZE];
    snprintf(response, BUFSIZE,
             "HTTP/1.1 200 OK\r\n"
             "Content-Type: text/html\r\n"
             "\r\n"
             "<html><body><h1>Hello from the server!</h1></body></html>\r\n");

    ssize_t numBytesSent = send(clntSocket, response, strlen(response), 0);
    if (numBytesSent < 0)
        DieWithSystemMessage("send() failed");

    close(clntSocket);
}

int main(int argc, char *argv[]) {
    if (argc != 2)
        DieWithUserMessage("Parameter(s)", "<Server Port>");

    in_port_t servPort = atoi(argv[1]);

    int servSock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (servSock < 0)
        DieWithSystemMessage("socket() failed");

    struct sockaddr_in servAddr{};
    CreateServerAddress(servAddr, SERVER_IP, servPort);

    if (bind(servSock, (struct sockaddr*) &servAddr, sizeof(servAddr)) < 0)
        DieWithSystemMessage("bind() failed");

    if (listen(servSock, MAX_PENDING) < 0)
        DieWithSystemMessage("listen() failed");

    vector<thread> threads;

    while (true) {
        struct sockaddr_in clntAddr{};
        socklen_t clntAddrLen = sizeof(clntAddr);

        int clntSock = accept(servSock, (struct sockaddr *) &clntAddr, &clntAddrLen);
        if (clntSock < 0) {
            perror("accept() failed");
            break; // Terminate the server on accept error
        }

        char clntName[INET_ADDRSTRLEN];
        if (inet_ntop(AF_INET, &clntAddr.sin_addr.s_addr, clntName, sizeof(clntName)) != nullptr)
            printf("Handling client %s/%d\n", clntName, ntohs(clntAddr.sin_port));
        else
            puts("Unable to get client address");

        threads.emplace_back(thread(HandleClient, clntSock));
        threads.back().detach();  // Detach the thread to allow it to run independently
    }

    close(servSock); // Close the server socket on termination

    return 0;
}