#include <winsock2.h>
#include <ws2tcpip.h>
#include <iostream>
#include <chrono>
#include <string>

#pragma comment(lib, "Ws2_32.lib")
#define PORT 666
#define SERVERADDR "127.0.0.1"
using namespace std;

struct Message {
    uint32_t length;
    char data[128];
};

struct StartData {
    uint32_t array_size;
    uint32_t thraed_count;
};

void send_data(int a, int b, SOCKET* clientSocket) {
    StartData newData;
    newData.array_size = htonl(a);
    newData.thraed_count = htonl(b);
    size_t bytesSent = send(*clientSocket, (char*)&newData, sizeof(newData), 0);
    if (bytesSent == -1) {
        cerr << "Error sending message." << endl;
    }
    else if (bytesSent == 0) {
        cout << "Disconnected." << endl;
    }
}

void send_message(const string& messageToSend, SOCKET* clientSocket) {
    Message sendMessage;
    sendMessage.length = htonl(messageToSend.length());
    strncpy_s(sendMessage.data, messageToSend.c_str(), sizeof(sendMessage.data) - 1);
    sendMessage.data[sizeof(sendMessage.data) - 1] = '\0';

    size_t bytesSent = send(*clientSocket, (char*)&sendMessage, sizeof(sendMessage.length) + messageToSend.length(), 0);
    if (bytesSent == -1) {
        cerr << "Error sending message." << endl;
    }
    cout << messageToSend << endl;
}

int receiveMessage(SOCKET* clientSocket, Message* receivedMessage) {
    size_t bytesRead = recv(*clientSocket, (char*)&receivedMessage->length, sizeof(receivedMessage->length), 0);
    if (bytesRead == -1) {
        cerr << "Error receiving message length." << endl;
        return -1;
    }
    else if (bytesRead == 0) {
        cout << "Disconnected." << endl;
        return -1;
    }
    receivedMessage->length = ntohl(receivedMessage->length);
    //receivedMessage->data = new char[receivedMessage->length + 1];
    bytesRead = recv(*clientSocket, receivedMessage->data, receivedMessage->length, 0);
    if (bytesRead == -1) {
        cerr << "Error receiving message data." << endl;
        return -1;
    }
    else if (bytesRead == 0) {
        cout << "Disconnected." << endl;
        return -1;
    }
    receivedMessage->data[receivedMessage->length] = '\0';
    cout << receivedMessage->data << endl;
    return 0;
}

int main() {
    char end;
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

    Message newmes;
    send_message(string("hello from client"), &clientSocket);
    receiveMessage(&clientSocket, &newmes);

    send_message(string("CONNECTION"), &clientSocket);
    receiveMessage(&clientSocket, &newmes);
    Sleep(800);

    send_message(string("SEND_DATA"), &clientSocket);
    receiveMessage(&clientSocket, &newmes);
    Sleep(500);

    int arrSize = 40000, threads = 5;
    send_data(arrSize, threads, &clientSocket);

    send_message(string("START"), &clientSocket);
    receiveMessage(&clientSocket, &newmes);
    Sleep(800);

    send_message(string("GET_RESULT"), &clientSocket);
    receiveMessage(&clientSocket, &newmes);

    int k = 0;
    while (strstr(newmes.data, "RESULT") == NULL && k < 10) {
        Sleep(100);
        send_message(string("GET_RESULT"), &clientSocket);
        receiveMessage(&clientSocket, &newmes);
        k++;
    }
    /*send_message(string("GET_STATUS"), &clientSocket);
    receiveMessage(&clientSocket, &newmes);*/
    cout << "This was a successful connection!" << endl;
    closesocket(clientSocket);
    WSACleanup();
    return 0;
}