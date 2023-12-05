#include <iostream>
#include <sstream>
#include <unordered_map>

using namespace std;

unordered_map<string, string> ParseHeader(const string &header) {
    unordered_map<string, string> map;
    istringstream iss(header);
    string key, value, line;
    iss >> map["arg1"] >> map["arg2"] >> map["arg3"];
    // Get the position after reading the initial values
    streampos initialPos = iss.tellg();

    while (getline(iss, line) && !line.empty()) {
        istringstream lineStream(line);
        getline(lineStream, key, ':');
        getline(lineStream >> ws, value);
        map[key] = value;
    }

    // Get the position after the while loop
    streampos remainingPos = iss.tellg();

    // Get a pointer to the remaining characters in the original string
    const char* remainingChars = header.c_str() + static_cast<size_t>(remainingPos);

    // Print the remaining characters (body of the HTTP request)
    cout << "Body of the HTTP request: " << remainingChars << endl;

    return map;
}

int main() {
    ostringstream requestStream;
    requestStream << "POST " << "/path" << " HTTP/1.1\r\n";
    requestStream << "Content-Type: " << "text/html" << "\r\n";
    requestStream << "Content-Length: " << 123 << "\r\n";
    requestStream << "Connection: keep-alive" << "\r\n\r\n";
    requestStream << "This is the body of the HTTP request.";

    string header = requestStream.str();

    unordered_map<string, string> result = ParseHeader(header);

    // Do something with the result if needed

    return 0;
}
