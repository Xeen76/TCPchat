#ifndef INCLUDED_HTTP_TCPSERVER
#define INCLUDED_HTTP_TCPSERVER

#include <stdio.h>
#include <winsock.h>
#include <stdlib.h>
#include <string>
#include <vector>
#include <algorithm>

namespace http
{

    class TcpServer
    {
    public:
        TcpServer(std::string ip_address, int port);
        ~TcpServer();
        void startListen();

    private:
        std::string m_ip_address;
        int m_port;
        SOCKET m_socket;
        SOCKET m_new_socket;
        long m_incomingMessage;
        struct sockaddr_in m_socketAddress;
        int m_socketAddress_len;
        std::string m_serverMessage;
        WSADATA m_wsaData;
        std::string m_receivedMessage;
        std::vector<SOCKET> m_clientSockets; // Store all connected client sockets

        int startServer();
        void closeServer();
        void acceptConnection(SOCKET &new_socket);
        std::string buildResponse();
        void sendResponse();
        void handleRequest(const std::string& request);
        void broadcastMessage(const std::string& message, SOCKET senderSocket = INVALID_SOCKET); // New helper
    };

} // namespace http

#endif