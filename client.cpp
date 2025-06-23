#include <iostream>
#include <string>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <cstring>
#include <sys/select.h>
#include <cstdio>

class ChatClient {
private:
    static const int BUFFER_SIZE = 30720;
    int sock;
    struct sockaddr_in server;
    
public:
    ChatClient(const char* ip_address, int port) {
        sock = socket(AF_INET, SOCK_STREAM, 0);
        if (sock == -1) {
            std::cout << "Could not create socket" << std::endl;
            return;
        }
        
        server.sin_addr.s_addr = inet_addr(ip_address);
        server.sin_family = AF_INET;
        server.sin_port = htons(port);
        
        if (connect(sock, (struct sockaddr *)&server, sizeof(server)) < 0) {
            std::cout << "Connect failed" << std::endl;
            return;
        }
        
        std::cout << "Connected to server" << std::endl;
    }
    
    void chat() {
        char message[BUFFER_SIZE];
        char server_reply[BUFFER_SIZE];
        fd_set readfds;
        
        std::cout << "Chat session started. Type your messages and press Enter to send.\n";
        std::cout << "Type 'exit' to end the session.\n\n";

        while(true) {
            FD_ZERO(&readfds);
            FD_SET(STDIN_FILENO, &readfds);
            FD_SET(sock, &readfds);

            select(sock + 1, &readfds, NULL, NULL, NULL);

            if (FD_ISSET(sock, &readfds)) {
                memset(server_reply, 0, BUFFER_SIZE);
                int receive_size = recv(sock, server_reply, BUFFER_SIZE, 0);
                
                if (receive_size <= 0) {
                    std::cout << "[HTTP] Server disconnected\n";
                    break;
                }

                std::string reply(server_reply);
                std::cout << "\n[HTTP Response Received] Length: " << receive_size << " bytes\n";
                std::cout << "Raw data: " << reply << "\n";

                if (reply == "exit") {
                    std::cout << "[HTTP] Server ended the session\n";
                    break;
                }
                std::cout << "Server: " << server_reply << std::endl;
            }

            if (FD_ISSET(STDIN_FILENO, &readfds)) {
                memset(message, 0, BUFFER_SIZE);
                if (fgets(message, BUFFER_SIZE, stdin) != NULL) {
                    message[strcspn(message, "\n")] = 0;
                    
                    if (strcmp(message, "exit") == 0) {
                        std::cout << "[HTTP] Sending exit request...\n";
                        send(sock, message, strlen(message), 0);
                        break;
                    }
                    
                    int bytesSent = send(sock, message, strlen(message), 0);
                    if (bytesSent < 0) {
                        std::cout << "[HTTP] Send failed\n";
                        break;
                    }
                    std::cout << "\n[HTTP Request Sent] Length: " << bytesSent << " bytes\n";
                    std::cout << "Raw data: " << message << "\n";
                }
            }
        }
        
        close(sock);
    }
};

int main() {
    ChatClient client("127.0.0.1", 8080);
    client.chat();
    return 0;
} 