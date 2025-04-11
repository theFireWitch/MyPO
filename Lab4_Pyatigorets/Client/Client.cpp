#include <winsock2.h>
#include <ws2tcpip.h>
#include <iostream>
#include <chrono>

#pragma comment(lib, "Ws2_32.lib")
#define PORT 666
#define SERVERADDR "127.0.0.1"
using namespace std;

struct Message {
    uint32_t length;
    char data[1024];
};

int main() {

    WSAData wsaData;
    if (WSAStartup(0x0202, &wsaData)) {
        printf("Error WSAStartup %d\n", WSAGetLastError());
        return -1;
    }

    SOCKET clientSocket;
    if ((clientSocket = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        printf("Error socket %d\n", WSAGetLastError());
        WSACleanup();
        return -1;
    }

    sockaddr_in dest_addr;
    dest_addr.sin_family = AF_INET;
    dest_addr.sin_port = htons(PORT);
    inet_pton(AF_INET, SERVERADDR, &dest_addr.sin_addr);

    if (connect(clientSocket, (sockaddr*)&dest_addr, sizeof(dest_addr))) {
        printf("Connect error %d\n", WSAGetLastError());
        return -1;
    }
    cout << "Conection is successful!" << endl;

    string messageToSend = "hello from client";
    Message sendMessage;
    sendMessage.length = htonl(messageToSend.length());
    strncpy_s(sendMessage.data, messageToSend.c_str(), sizeof(sendMessage.data) - 1);
    sendMessage.data[sizeof(sendMessage.data) - 1] = '\0';

    size_t bytesSent = send(clientSocket, (char*)&sendMessage, sizeof(sendMessage.length) + messageToSend.length(), 0);
    if (bytesSent == -1) {
        std::cerr << "Error sending message." << std::endl;
    }

    cout << "Data sent successfully" << endl;

    closesocket(clientSocket);
    WSACleanup();
    return 0;
}
