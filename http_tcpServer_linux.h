#pragma once

#include <stdio.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <iostream>
#include <sys/select.h>
#include <cstdlib>
#include <vector>
#include <algorithm>

namespace http {
    class TcpServer {
    public:
        TcpServer(std::string ip_address, int port);
        ~TcpServer();
        void startListen();

    private:
        std::string m_ip_address;
        int m_port;
        int m_socket;
        int m_new_socket;
        long m_incomingMessage;
        struct sockaddr_in m_socketAddress;
        unsigned int m_socketAddress_len;
        std::string m_serverMessage;
        std::string m_receivedMessage;
        std::vector<int> m_clientSockets;

        int startServer();
        void closeServer();
        void acceptConnection(int &new_socket);
        std::string buildResponse();
        void sendResponse();
        void handleRequest(const std::string& request);
        void exitWithError(const char* error);
        void log(const std::string& message);
        void handleChatSession();
        void broadcastMessage(const std::string& message, int senderSocket = -1);
    };
}