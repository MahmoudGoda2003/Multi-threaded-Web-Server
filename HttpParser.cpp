#include <sstream>
#include <unordered_map>
#include <iostream>


using namespace std;

// This function parses an HTTP header string and extracts key-value pairs into an unordered_map.
// It also calculates the total response bytes, including the header and extra bytes for CRLF.
unordered_map<string, string> ParseHeader(const string &header, size_t &responseBytes) {
    // Initialize an unordered_map to store key-value pairs from the header
    unordered_map<string, string> map;

    // Use an istringstream to read the header line by line
    istringstream iss(header);
    string key, value, line;

    // Extract the initial set of values (commonly used for HTTP request/response)
    iss >> map["arg1"] >> map["arg2"] >> map["arg3"];

    // Save the initial position for calculating total response bytes
    streampos pos = iss.tellg();

    // Read the rest of the header and extract key-value pairs
    while (getline(iss, line) && !line.empty()) {
        istringstream lineStream(line);
        getline(lineStream, key, ':');
        getline(lineStream >> ws, value);
        map[key] = value;
    }

    // Calculate the total response bytes, including the header and extra bytes for CRLF
    responseBytes = header.find("\r\n\r\n", pos) + 4;

    // Return the populated unordered_map
    return map;
}
