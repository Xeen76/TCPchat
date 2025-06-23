# TCP Chat App

A simple multi-client TCP chat application where multiple clients and the server can chat together. All messages are broadcast to every participant and shown in the server console.

## Features
- Multiple clients can connect and chat simultaneously.
- The server can send messages to all clients from its console.
- All messages (from any client or the server) are shown in the server console and broadcast to everyone.
- Dockerized for easy setup and cross-platform support (Linux/Windows).

## Project Structure
- `server_linux.cpp`, `http_tcpServer_linux.cpp`, `http_tcpServer_linux.h`: Linux server implementation.
- `server.cpp`, `http_tcpserver.cpp`, `http_tcpserver.h`: Windows server implementation.
- `client.cpp`: Client implementation (works on both platforms).
- `Dockerfile`, `docker-compose.yaml`: Docker setup for building and running the app.
- `commands`: Helper file with common Docker commands.

## Getting Started

### Prerequisites
- [Docker](https://www.docker.com/get-started) and [Docker Compose](https://docs.docker.com/compose/)

### Build and Run with Docker
1. **Build the Docker image:**
   ```sh
   docker-compose build
   ```
2. **Start the server container:**
   ```sh
   docker-compose up
   ```
3. **Attach to the server console (to send messages as the server):**
   ```sh
   docker attach tcp_chatapp-http-server-1
   ```
4. **Run a new client (repeat in new terminals for multiple clients):**
   ```sh
   docker exec -it tcp_chatapp-http-server-1 ./build/ChatClient
   ```

### Usage
- **Server Console:** Type messages and press Enter to broadcast to all clients. All messages (from clients and server) are displayed here.
- **Client:** Type messages and press Enter to send to all participants. Type `exit` to leave the chat.
- **To stop the server:** Type `exit` in the server console or press `Ctrl+C` in the terminal running `docker-compose up`.

## Development (Without Docker)
You can also build and run the server/client directly on your system if you have a C++11 compiler:

### Linux
```sh
g++ -std=c++11 -o HttpServer server_linux.cpp http_tcpServer_linux.cpp
./HttpServer

g++ -std=c++11 -o ChatClient client.cpp
./ChatClient
```

### Windows
Use a C++11-compatible compiler (e.g., MSVC or MinGW) to build `server.cpp`/`http_tcpserver.cpp` and `client.cpp`.

## License
This project is for educational purposes. 

![Chat App Screenshot](C:\Users\manan\OneDrive\Desktop\TCP_ChatApp\Screenshots\scrnshot_1.png)