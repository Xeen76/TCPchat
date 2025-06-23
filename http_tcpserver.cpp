#include <http_tcpserver.h>
#include <iostream>
#include <sstream>
#include <unistd.h>
#include <vector>
#include <algorithm>

namespace
{
    const int BUFFER_SIZE = 30720;

    void log(const std::string &message)
    {
        std::cout << message << std::endl;
    }

    void exitWithError(const std::string &errorMessage)
    {
        std::cout << WSAGetLastError() << std::endl;
        log("ERROR: " + errorMessage);
        exit(1);
    }
}

namespace http
{

    TcpServer::TcpServer(std::string ip_address, int port) : m_ip_address(ip_address), m_port(port), m_socket(), m_new_socket(),
                                                             m_incomingMessage(),
                                                             m_socketAddress(), m_socketAddress_len(sizeof(m_socketAddress)),
                                                             m_serverMessage(buildResponse()), m_wsaData()
    {
        m_socketAddress.sin_family = AF_INET;
        m_socketAddress.sin_port = htons(m_port);
        m_socketAddress.sin_addr.s_addr = inet_addr(m_ip_address.c_str());

        if (startServer() != 0)
        {
            std::ostringstream ss;
            ss << "Failed to start server with PORT: " << ntohs(m_socketAddress.sin_port);
            log(ss.str());
        }
    }

    TcpServer::~TcpServer()
    {
        closeServer();
    }

    int TcpServer::startServer()
    {
        if (WSAStartup(MAKEWORD(2, 0), &m_wsaData) != 0)
        {
            exitWithError("WSAStartup failed");
        }

        m_socket = socket(AF_INET, SOCK_STREAM, 0);
        if (m_socket < 0)
        {
            exitWithError("Cannot create socket");
            return 1;
        }

        if (bind(m_socket, (sockaddr *)&m_socketAddress, m_socketAddress_len) < 0)
        {
            exitWithError("Cannot connect socket to address");
            return 1;
        }

        return 0;
    }

    void TcpServer::closeServer()
    {
        closesocket(m_socket);
        closesocket(m_new_socket);
        WSACleanup();
        exit(0);
    }

    void TcpServer::startListen()
    {
        if (listen(m_socket, 20) < 0)
        {
            exitWithError("Socket listen failed");
        }

        std::ostringstream ss;
        ss << "\n*** Listening on ADDRESS: " << inet_ntoa(m_socketAddress.sin_addr) << " PORT: " << ntohs(m_socketAddress.sin_port) << " ***\n\n";
        log(ss.str());

        fd_set readfds;
        int max_sd;
        char buffer[BUFFER_SIZE];

        while (true)
        {
            FD_ZERO(&readfds);
            FD_SET(m_socket, &readfds); // Listen for new connections
            FD_SET(0, &readfds); // Listen for server input (stdin)
            max_sd = m_socket;

            // Add client sockets to set
            for (SOCKET clientSock : m_clientSockets) {
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
                SOCKET new_socket;
                acceptConnection(new_socket);
                m_clientSockets.push_back(new_socket);
                std::cout << "[SERVER] New client connected: " << new_socket << std::endl;
                std::string welcome = "[SERVER] Welcome to the chat!";
                send(new_socket, welcome.c_str(), welcome.size(), 0);
            }

            // Server input
            if (FD_ISSET(0, &readfds)) {
                memset(buffer, 0, BUFFER_SIZE);
                if (fgets(buffer, BUFFER_SIZE, stdin) != NULL) {
                    buffer[strcspn(buffer, "\n")] = 0;
                    std::string message = std::string("[SERVER]: ") + buffer;
                    std::cout << message << std::endl;
                    broadcastMessage(message);
                    if (strcmp(buffer, "exit") == 0) {
                        for (SOCKET clientSock : m_clientSockets) {
                            send(clientSock, "exit", 4, 0);
                        }
                        break;
                    }
                }
            }

            // Client messages
            std::vector<SOCKET> disconnected;
            for (SOCKET clientSock : m_clientSockets) {
                if (FD_ISSET(clientSock, &readfds)) {
                    memset(buffer, 0, BUFFER_SIZE);
                    int bytesReceived = recv(clientSock, buffer, BUFFER_SIZE, 0);
                    if (bytesReceived <= 0) {
                        std::cout << "[SERVER] Client disconnected: " << clientSock << std::endl;
                        disconnected.push_back(clientSock);
                        continue;
                    }
                    buffer[bytesReceived] = 0;
                    std::string message = std::string("[Client ") + std::to_string(clientSock) + "]: " + buffer;
                    std::cout << message << std::endl;
                    if (strcmp(buffer, "exit") == 0) {
                        send(clientSock, "exit", 4, 0);
                        disconnected.push_back(clientSock);
                    } else {
                        broadcastMessage(message, clientSock);
                    }
                }
            }
            // Remove disconnected clients
            for (SOCKET sock : disconnected) {
                closesocket(sock);
                m_clientSockets.erase(std::remove(m_clientSockets.begin(), m_clientSockets.end(), sock), m_clientSockets.end());
            }
        }
        // Cleanup
        for (SOCKET sock : m_clientSockets) closesocket(sock);
        closesocket(m_socket);
    }

    void TcpServer::acceptConnection(SOCKET &new_socket)
    {
        new_socket = accept(m_socket, (sockaddr *)&m_socketAddress, &m_socketAddress_len);
        if (new_socket < 0)
        {
            std::ostringstream ss;
            ss << "Server failed to accept incoming connection from ADDRESS: " << inet_ntoa(m_socketAddress.sin_addr) << "; PORT: " << ntohs(m_socketAddress.sin_port);
            exitWithError(ss.str());
        }
    }

    std::string TcpServer::buildResponse()
    {
        std::string htmlFile = "<!DOCTYPE html><html lang=\"en\"><body><h1> HOME </h1><p> Hello from your Server :) </p></body></html>";
        std::ostringstream ss;
        ss << "HTTP/1.1 200 OK\nContent-Type: text/html\nContent-Length: " << htmlFile.size() << "\n\n"
           << htmlFile;

        return ss.str();
    }

    void TcpServer::sendResponse()
    {
        int bytesSent;
        long totalBytesSent = 0;

        while (totalBytesSent < m_serverMessage.size())
        {
            bytesSent = send(m_new_socket, m_serverMessage.c_str(), m_serverMessage.size(), 0);
            if (bytesSent < 0)
            {
                break;
            }
            totalBytesSent += bytesSent;
        }

        if (totalBytesSent == m_serverMessage.size())
        {
            log("------ Server Response sent to client ------\n\n");
        }
        else
        {
            log("Error sending response to client.");
        }
    }

    void TcpServer::broadcastMessage(const std::string& message, SOCKET senderSocket) {
        for (SOCKET clientSock : m_clientSockets) {
            if (clientSock != senderSocket) {
                send(clientSock, message.c_str(), message.size(), 0);
            }
        }
    }

} // namespace http