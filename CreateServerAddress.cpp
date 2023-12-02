#include <cstring>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include "Practical.h"

void CreateServerAddress(struct sockaddr_in& servAddr, const char* servIP, in_port_t servPort) {
    memset(&servAddr, 0, sizeof(servAddr));
    servAddr.sin_family = AF_INET;
    int rtnVal = inet_pton(AF_INET, servIP, &servAddr.sin_addr.s_addr);
    if (rtnVal == 0)
        DieWithUserMessage("inet_pton() failed", "invalid address string");
    else if (rtnVal < 0)
        DieWithSystemMessage("inet_pton() failed");
    servAddr.sin_port = htons(servPort);
}