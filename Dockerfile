FROM gcc:4.9

COPY . /usr/src/http-server
WORKDIR /usr/src/http-server

RUN mkdir -p build/ && cd build/ && rm -rf * 
RUN cd build/ && gcc -std=c++11 -I.. -o HttpServer ../server_linux.cpp ../http_tcpServer_linux.cpp -lstdc++
RUN cd build/ && gcc -std=c++11 -I.. -o ChatClient ../client.cpp -lstdc++

EXPOSE 8080 