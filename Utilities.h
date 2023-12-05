#ifndef PRACTICAL_H_
#define PRACTICAL_H_

#include <cstdbool>
#include <sstream>
#include <cstdio>
#include <sys/socket.h>
#include <unordered_map>
#include <netinet/in.h>


using namespace std;
// Handle error with user msg
void DieWithUserMessage(const char *msg, const char *detail);
// Handle error with sys msg
void DieWithSystemMessage(const char *msg);
// Create server address
void CreateServerAddress(struct sockaddr_in& servAddr, const char* servIP, in_port_t servPort);
// Send file
void SendFile(int sock, ostringstream& requestStream, ifstream& fileStream);
// Receive file
void ReceiveFile(int sock, char *buffer, const char *path, const char *contentLength, ssize_t numBytesRecv, size_t headerBytes);
// Parse header
unordered_map<string, string> ParseHeader(const string &header, size_t& responseBytes);

// Constants
enum sizeConstants {
    BUFSIZE = 2048,
    MAXREQUESTS = 300,
};

#endif // PRACTICAL_H_