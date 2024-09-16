#include <iostream>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <cstring>

#define PORT 8080  // The port number to connect to the server

// Function to send a command to the server and get the response
void sendCommand(const std::string& command) {
    int sock = 0;
    struct sockaddr_in serv_addr;
    char buffer[1024] = {0};

    // Create a socket
    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        std::cerr << "Socket creation error" << std::endl;
        return;
    }
    std::cout << "Socket created successfully" << std::endl;

    // Define the server address
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(PORT);

    // Convert address from text to binary form
    if (inet_pton(AF_INET, "127.0.0.1", &serv_addr.sin_addr) <= 0) {
        std::cerr << "Invalid address/ Address not supported" << std::endl;
        return;
    }
    std::cout << "Address conversion successful" << std::endl;

    // Connect to the server
    if (connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
        std::cerr << "Connection failed" << std::endl;
        return;
    }
    std::cout << "Connected to the server" << std::endl;

    // Send the command to the server
    send(sock, command.c_str(), command.length(), 0);
    std::cout << "Command sent: " << command << std::endl;

    // Read the response from the server
    int valread = read(sock, buffer, 1024);
    std::cout << "Server response: " << buffer << std::endl;

    // Close the socket
    close(sock);
}

int main() {
    std::string command;
    std::cout << "Welcome to the MST Client!" << std::endl;
    std::cout << "Available commands:" << std::endl;
    std::cout << "ADD u v weight    -> Adds an edge between vertices u and v with the given weight." << std::endl;
    std::cout << "REMOVE u v        -> Removes the edge between vertices u and v." << std::endl;
    std::cout << "UPDATE u v weight -> Updates the weight of the edge between vertices u and v." << std::endl;
    std::cout << "SOLVE PRIM        -> Solves the MST using Prim's algorithm." << std::endl;
    std::cout << "SOLVE KRUSKAL     -> Solves the MST using Kruskal's algorithm." << std::endl;
    std::cout << "Type 'exit' to quit." << std::endl;

    while (true) {
        std::cout << "\nEnter command: ";
        std::getline(std::cin, command);

        if (command == "exit") {
            std::cout << "Exiting..." << std::endl;
            break;
        }

        // Send the user's command to the server
        sendCommand(command);
    }

    return 0;
}
