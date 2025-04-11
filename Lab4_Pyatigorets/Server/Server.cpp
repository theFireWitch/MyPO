#include <winsock2.h>
#include <ws2tcpip.h>
#include <iostream>
#include <chrono>

#pragma comment(lib, "Ws2_32.lib")
#define PORT 666
using namespace std;

struct Message {
    uint32_t length;
    char data[1024];
};

int main() {

    char buff[10];
    int temp = 0;
    int BytesReceived = 0;

    if (WSAStartup(0x0202, (WSAData*)&buff[0])) {
        printf("Error WSAStartup %d\n", WSAGetLastError());
        return -1;
    }

    SOCKET mysocket;
    if ((mysocket = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        printf("Error socket %d\n", WSAGetLastError());
        WSACleanup();
        return -1;
    }

    sockaddr_in local_addr;
    local_addr.sin_family = AF_INET;
    local_addr.sin_port = htons(PORT);
    local_addr.sin_addr.s_addr = 0;

    if (bind(mysocket, (sockaddr*)&local_addr, sizeof(local_addr))) {
        printf("Error bind %d\n", WSAGetLastError());
        closesocket(mysocket);
        WSACleanup();
        return -1;
    }

    if (listen(mysocket, 0x100)) {
        printf("Error listen %d\n", WSAGetLastError());
        closesocket(mysocket);
        WSACleanup();
        return -1;
    }

    cout << "Waiting for client..." << endl;

    sockaddr_in client_addr;
    int client_addr_size = sizeof(client_addr);
    SOCKET clientSocket = accept(mysocket, (sockaddr*)&client_addr, &client_addr_size);

    Message receivedMessage;
    int i = 0;
    while (i==0) {
        size_t bytesRead = recv(clientSocket, (char*)&receivedMessage.length, sizeof(receivedMessage.length), 0);
        if (bytesRead == -1) {
            std::cerr << "Error receiving message length." << std::endl;
            break;
        }
        else if (bytesRead == 0) {
            std::cout << "Client disconnected." << std::endl;
            break;
        }

        receivedMessage.length = ntohl(receivedMessage.length);

        if (receivedMessage.length > sizeof(receivedMessage.data)) {
            std::cerr << "Received message length exceeds buffer size." << std::endl;
            break;
        }
        bytesRead = recv(clientSocket, receivedMessage.data, receivedMessage.length, 0);
        if (bytesRead == -1) {
            std::cerr << "Error receiving message data." << std::endl;
            break;
        }
        else if (bytesRead == 0) {
            std::cout << "Client disconnected." << std::endl;
            break;
        }

        receivedMessage.data[receivedMessage.length] = '\0';

        std::cout << "Received message: " << receivedMessage.data << std::endl;

        i++;
    }

    cout << "Bytes received: " << BytesReceived << endl;

    closesocket(clientSocket);
    closesocket(mysocket);
    WSACleanup();
    return 0;
}

