#include "http_tcpServer_linux.h"
#include <sstream>
#include <unistd.h>
#include <arpa/inet.h>
#include <cstring>
#include <sys/select.h>
#include <vector>
#include <algorithm>

namespace {
    const int BUFFER_SIZE = 30720;
}

namespace http {
    TcpServer::TcpServer(std::string ip_address, int port) : 
        m_ip_address(ip_address), m_port(port), m_socket(), 
        m_new_socket(), m_incomingMessage(),
        m_socketAddress(), m_socketAddress_len(sizeof(m_socketAddress))
    {
        m_socketAddress.sin_family = AF_INET;
        m_socketAddress.sin_port = htons(m_port);
        m_socketAddress.sin_addr.s_addr = inet_addr(m_ip_address.c_str());
        
        if (startServer() != 0) {
            std::ostringstream ss;
            ss << "Failed to start server with PORT: " << ntohs(m_socketAddress.sin_port);
            log(ss.str());
        }
    }

    TcpServer::~TcpServer() {
        closeServer();
    }

    void TcpServer::startListen() {
        if (listen(m_socket, 20) < 0) {
            exitWithError("Socket listen failed");
        }

        std::ostringstream ss;
        ss << "\n*** Chat Server Listening on ADDRESS: " << inet_ntoa(m_socketAddress.sin_addr)
           << " PORT: " << ntohs(m_socketAddress.sin_port) << " ***\n\n";
        log(ss.str());

        fd_set readfds;
        int max_sd;
        char buffer[BUFFER_SIZE];

        while (true) {
            FD_ZERO(&readfds);
            FD_SET(m_socket, &readfds); // Listen for new connections
            FD_SET(STDIN_FILENO, &readfds); // Listen for server input
            max_sd = m_socket;

            // Add client sockets to set
            for (int clientSock : m_clientSockets) {
                if (clientSock > 0) {
                    FD_SET(clientSock, &readfds);
                    if (clientSock > max_sd) max_sd = clientSock;
                }
            }

            int activity = select(max_sd + 1, &readfds, NULL, NULL, NULL);
            if (activity < 0) {
                log("select error");
                continue;
            }

            // New connection
            if (FD_ISSET(m_socket, &readfds)) {
                int new_socket;
                acceptConnection(new_socket);
                m_clientSockets.push_back(new_socket);
                std::cout << "[SERVER] New client connected: " << new_socket << std::endl;
                std::string welcome = "[SERVER] Welcome to the chat!";
                write(new_socket, welcome.c_str(), welcome.size());
            }

            // Server input
            if (FD_ISSET(STDIN_FILENO, &readfds)) {
                memset(buffer, 0, BUFFER_SIZE);
                if (fgets(buffer, BUFFER_SIZE, stdin) != NULL) {
                    buffer[strcspn(buffer, "\n")] = 0;
                    std::string message = std::string("[SERVER]: ") + buffer;
                    std::cout << message << std::endl;
                    broadcastMessage(message);
                    if (strcmp(buffer, "exit") == 0) {
                        for (int clientSock : m_clientSockets) {
                            write(clientSock, "exit", 4);
                        }
                        break;
                    }
                }
            }

            // Client messages
            std::vector<int> disconnected;
            for (int clientSock : m_clientSockets) {
                if (FD_ISSET(clientSock, &readfds)) {
                    memset(buffer, 0, BUFFER_SIZE);
                    int bytesReceived = read(clientSock, buffer, BUFFER_SIZE);
                    if (bytesReceived <= 0) {
                        std::cout << "[SERVER] Client disconnected: " << clientSock << std::endl;
                        disconnected.push_back(clientSock);
                        continue;
                    }
                    buffer[bytesReceived] = 0;
                    std::string message = std::string("[Client ") + std::to_string(clientSock) + "]: " + buffer;
                    std::cout << message << std::endl;
                    if (strcmp(buffer, "exit") == 0) {
                        write(clientSock, "exit", 4);
                        disconnected.push_back(clientSock);
                    } else {
                        broadcastMessage(message, clientSock);
                    }
                }
            }
            // Remove disconnected clients
            for (int sock : disconnected) {
                close(sock);
                m_clientSockets.erase(std::remove(m_clientSockets.begin(), m_clientSockets.end(), sock), m_clientSockets.end());
            }
        }
        // Cleanup
        for (int sock : m_clientSockets) close(sock);
        close(m_socket);
    }

    void TcpServer::broadcastMessage(const std::string& message, int senderSocket) {
        for (int clientSock : m_clientSockets) {
            if (clientSock != senderSocket) {
                write(clientSock, message.c_str(), message.size());
            }
        }
    }

    void TcpServer::handleChatSession()
    {
        char buffer[BUFFER_SIZE];
        fd_set readfds;
        
        std::cout << "Chat session started. Type your messages and press Enter to send.\n";
        std::cout << "Type 'exit' to end the session.\n\n";

        while(true)
        {
            FD_ZERO(&readfds);
            FD_SET(STDIN_FILENO, &readfds);
            FD_SET(m_new_socket, &readfds);

            select(m_new_socket + 1, &readfds, NULL, NULL, NULL);

            if (FD_ISSET(m_new_socket, &readfds)) {
                memset(buffer, 0, BUFFER_SIZE);
                int bytesReceived = read(m_new_socket, buffer, BUFFER_SIZE);
                
                if (bytesReceived <= 0) {
                    std::cout << "[HTTP] Client disconnected\n";
                    break;
                }

                std::string message(buffer);
                std::cout << "\n[HTTP Request Received] Length: " << bytesReceived << " bytes\n";
                std::cout << "Raw data: " << message << "\n";

                if (message == "exit") {
                    std::cout << "[HTTP] Client requested to end session\n";
                    break;
                }
                std::cout << "Client: " << message << std::endl;
            }

            if (FD_ISSET(STDIN_FILENO, &readfds)) {
                memset(buffer, 0, BUFFER_SIZE);
                if (fgets(buffer, BUFFER_SIZE, stdin) != NULL) {
                    buffer[strcspn(buffer, "\n")] = 0;
                    std::string message(buffer);
                    
                    if (message == "exit") {
                        std::cout << "[HTTP] Ending session...\n";
                        write(m_new_socket, "exit", 4);
                        break;
                    }
                    
                    int bytesSent = write(m_new_socket, buffer, strlen(buffer));
                    std::cout << "\n[HTTP Response Sent] Length: " << bytesSent << " bytes\n";
                    std::cout << "Raw data: " << buffer << "\n";
                }
            }
        }
    }

    void TcpServer::handleRequest(const std::string& request) {
        m_receivedMessage = request;
    }

    std::string TcpServer::buildResponse()
    {
        std::ostringstream html;
        html << "<!DOCTYPE html><html lang=\"en\"><body>"
             << "<h1>Message Exchange</h1>"
             << "<p>Received message: " << m_receivedMessage << "</p>"
             << "<form method='POST'>"
             << "<input type='text' name='message' placeholder='Enter message'>"
             << "<input type='submit' value='Send'>"
             << "</form>"
             << "</body></html>";

        std::string htmlFile = html.str();
        std::ostringstream ss;
        ss << "HTTP/1.1 200 OK\nContent-Type: text/html\nContent-Length: " << htmlFile.size() << "\n\n"
           << htmlFile;

        return ss.str();
    }

    int TcpServer::startServer() {
        m_socket = socket(AF_INET, SOCK_STREAM, 0);
        if (m_socket < 0) {
            exitWithError("Cannot create socket");
            return 1;
        }

        if (bind(m_socket, (struct sockaddr *)&m_socketAddress, sizeof(m_socketAddress)) < 0) {
            exitWithError("Cannot connect socket to address");
            return 1;
        }

        return 0;
    }

    void TcpServer::closeServer() {
        close(m_socket);
        exit(0);
    }

    void TcpServer::acceptConnection(int &new_socket) {
        new_socket = accept(m_socket, (struct sockaddr *)&m_socketAddress,
                          &m_socketAddress_len);
        if (new_socket < 0) {
            std::ostringstream ss;
            ss << "Server failed to accept incoming connection from ADDRESS: " 
               << inet_ntoa(m_socketAddress.sin_addr) 
               << "; PORT: " << ntohs(m_socketAddress.sin_port);
            exitWithError(ss.str().c_str());
        }
    }

    void TcpServer::exitWithError(const char* error) {
        std::cerr << error << std::endl;
        exit(1);
    }

    void TcpServer::log(const std::string& message) {
        std::cout << message;
    }
}