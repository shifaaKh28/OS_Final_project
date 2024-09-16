#include "server.hpp"

int main() {
    MSTServer server(8080);  // Create server object with port 8080
    server.start();          // Start the server
    return 0;
}
