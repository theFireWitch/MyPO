#include <winsock2.h>
#include <ws2tcpip.h>
#include <iostream>
#include <chrono>
#include <thread>
#include <vector>

#pragma comment(lib, "Ws2_32.lib")
#define PORT 666
using namespace std;

struct Message {
    uint32_t length;
    char* data;
};

void MultyThreadFunc(int start, int end, vector<int>& A, vector<int>& B, int k, vector<int>& C) {
    for (int i = start; i < end; i++) {
        C[i] = A[i] - k * B[i];
    }
}
int receiveMessage(SOCKET* clientSocket, Message* receivedMessage) {
   
    int i = 0;
    while (i != 2) {
        size_t bytesRead = recv(*clientSocket, (char*)&receivedMessage->length, sizeof(receivedMessage->length), 0);
        if (bytesRead == -1) {
            cerr << "Error receiving message length." << endl;
            return -1;
        }
        else if (bytesRead == 0) {
            cout << "Client disconnected." << endl;
            return -1;
        }

        receivedMessage->length = ntohl(receivedMessage->length);
        cout << receivedMessage->length << endl;

        receivedMessage->data = new char[receivedMessage->length + 1];
        bytesRead = recv(*clientSocket, receivedMessage->data, receivedMessage->length, 0);
        if (bytesRead == -1) {
            cerr << "Error receiving message data." << endl;
            return -1;
        }
        else if (bytesRead == 0) {
            cout << "Client disconnected." << endl;
            return -1;
        }

        receivedMessage->data[receivedMessage->length] = '\0';

        cout << "Received message: " << receivedMessage->data << endl;

        i++;
    }
}

void ClilentThread(SOCKET* clientSocket) {
    Message receivedMessage;
    receiveMessage(clientSocket, &receivedMessage);
    closesocket(*clientSocket);
}

int main() {

    WSAData wsaData;
    if (WSAStartup(0x0202, &wsaData)) {
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

    cout << "Waiting for clients..." << endl;

   // while (true) {
        SOCKET clientSocket = accept(mysocket, nullptr, nullptr);
    //    thread();
    //}
    ClilentThread(&clientSocket);

    closesocket(mysocket);
    WSACleanup();
    return 0;
}
