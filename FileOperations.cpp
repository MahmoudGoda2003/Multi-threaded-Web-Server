#include <fstream>
#include <sstream>
#include <iostream>
#include <experimental/filesystem>
#include "Utilities.h"


// The code creates an alias 'fs' for the 'std::experimental::filesystem' namespace.
using namespace std;
namespace fs = std::experimental::filesystem;

// This function sends a file over a socket by first preparing a request header
// with content length and connection details. It then sends the header, followed by
// the file's content, in chunks, until the entire file is sent.
void SendFile(int sock, ostringstream& requestStream, ifstream& fileStream) {
    // Get the size of the file
    fileStream.seekg(0, ios::end);
    long fileSize = fileStream.tellg();

    // Add Content-Length and Connection details to the request header
    requestStream << "Content-Length: " << fileSize << "\r\n";
    requestStream << "Connection: keep-alive" << "\r\n\r\n";

    // Reset the file stream position to the beginning
    fileStream.seekg(0, ios::beg);

    // Get the request header as a string
    string requestHeader = requestStream.str();

    // Send the request header
    ssize_t numBytesSent;
    numBytesSent = send(sock, requestHeader.c_str(), requestHeader.size(), 0);
    if (numBytesSent < 0)
        DieWithSystemMessage("send() failed");

    // Send the file content in chunks until the entire file is sent
    char buffer[BUFSIZE];
    while (fileStream.tellg() < fileSize) {
        fileStream.read(buffer, min((int) BUFSIZE, static_cast<int>(fileSize - fileStream.tellg())));
        numBytesSent = send(sock, buffer, fileStream.gcount(), 0);
        if (numBytesSent < 0)
            DieWithSystemMessage("send() failed");
    }

    // Close the file stream after finishing the send operation
    fileStream.close();
}


// This function receives a file over a socket, writes it to a specified path,
// and handles the reception process based on the provided buffer, path, content length,
// received bytes count, and header bytes count.
void ReceiveFile(int sock, char *buffer, const char *path, const char *contentLength, ssize_t numBytesRecv, size_t headerBytes) {
    // Extract the directory from the given path and create it if it doesn't exist
    string dir = fs::path(path).parent_path().string();
    if (!fs::exists(dir))
        if (!fs::create_directories(dir))
            DieWithSystemMessage("create_directories() failed");

    // Open the file at the specified path in binary mode
    ofstream fileStream(path, ios::binary);
    if (!fileStream.is_open())
        DieWithSystemMessage("open() failed");

    // Write the received data to the file, excluding the headers
    fileStream.write(buffer + headerBytes, (long)(numBytesRecv - headerBytes));

    // Continue receiving data until the total received bytes match the expected content length
    size_t totalBytesRecv = numBytesRecv;
    size_t responseBytes = stoi(contentLength);
    while (totalBytesRecv < responseBytes &&
           (numBytesRecv = recv(sock, buffer, BUFSIZE - 1, 0)) > 0) {
        buffer[numBytesRecv] = '\0';

        // Print the received data to the standard output (stdout)
        fputs(buffer, stdout);

        // Write the received data to the file
        fileStream.write(buffer, (long)numBytesRecv);
        totalBytesRecv += numBytesRecv;
    }

    // Check for errors in the receive operation
    if (numBytesRecv < 0)
        DieWithSystemMessage("recv() failed");

    // Close the file after finishing the receive operation
    fileStream.close();
}