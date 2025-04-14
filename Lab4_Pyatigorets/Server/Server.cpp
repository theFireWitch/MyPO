#include <winsock2.h>
#include <ws2tcpip.h>
#include <iostream>
#include <chrono>
#include <thread>
#include <vector>
#include <mutex>
#include <string>
#include <algorithm>
#include <map>

#pragma comment(lib, "Ws2_32.lib")
#define PORT 666
using namespace std;

atomic<int> client_count(0);
mutex text_mutex;

struct Message {
    uint32_t length;
    char data[128];
};

struct StartData {
    uint32_t array_size = 0;
    uint32_t thraed_count = 0;
};

struct ResultData {
    double time = 0.0;
    vector<int> result_array;
};

struct Client {
    SOCKET socket;
    int client_number;
    bool working = false;
    StartData start_data;
    ResultData result_data;
    Client(SOCKET newsocket, int newnumber) : socket(newsocket), client_number(newnumber) {}
    Client() {}
};

map<int, Client> Clients;

int randGen() {
    return rand() % 1001;
}

int NullGen() {
    return 0;
}

void MultyThreadFunc(int start, int end, vector<int>& A, vector<int>& B, int k, vector<int>& C) {
    for (int i = start; i < end; i++) {
        C[i] = A[i] - k * B[i];
    }
}

void send_message(SOCKET* clientSocket, string* messageToSend, bool sending) {
    Message sendMessage;
    sendMessage.length = htonl(messageToSend->length());
    strncpy_s(sendMessage.data, messageToSend->c_str(), sizeof(sendMessage.data) - 1);
    sendMessage.data[sizeof(sendMessage.data) - 1] = '\0';

    size_t bytesSent = send(*clientSocket, (char*)&sendMessage, sizeof(sendMessage.length) + messageToSend->length(), 0);
    if (bytesSent == -1) {
        cerr << "SERVER> Error sending message." << endl;
    }
    if (sending) {
        lock_guard<mutex> _(text_mutex);
        cout << *messageToSend << endl;
    }
}

int receiveMessage(Client* ThisClient, Message* receivedMessage) {
    size_t bytesRead = recv(ThisClient->socket, (char*)&receivedMessage->length, sizeof(receivedMessage->length), 0);
    if (bytesRead == -1) {
        lock_guard<mutex> _(text_mutex);
        cerr << "SERVER> Error receiving message length from client " << ThisClient->client_number << endl;
        return 1;
    }
    else if (bytesRead == 0) {
        return 1;
    }
    receivedMessage->length = ntohl(receivedMessage->length);
    //receivedMessage->data = new char[receivedMessage->length + 1];
    bytesRead = recv(ThisClient->socket, receivedMessage->data, receivedMessage->length, 0);
    if (bytesRead == -1) {
        lock_guard<mutex> _(text_mutex);
        cerr << "SERVER> Error receiving message data from client " << ThisClient->client_number << endl;
        return 1;
    }
    else if (bytesRead == 0) {
        return 1;
    }
    receivedMessage->data[receivedMessage->length] = '\0';
    lock_guard<mutex> _(text_mutex);
    cout << "CLIENT" << ThisClient->client_number << "> " << receivedMessage->data << endl;
    return 0;
}

void WorkThread(Client* ThisClient) {
    int size = ThisClient->start_data.array_size, thread_count = ThisClient->start_data.thraed_count;
    int ElementsForOneThread = size / thread_count + ((size % thread_count == 0) ? 0 : 1);
    vector<int> arrayA(size);
    vector<int> arrayB(size);
    ThisClient->result_data.result_array.resize(size);
    int k = rand() % 10;
    generate(arrayA.begin(), arrayA.end(), randGen);
    generate(arrayB.begin(), arrayB.end(), randGen);
    generate(ThisClient->result_data.result_array.begin(), ThisClient->result_data.result_array.end(), NullGen);
    vector<thread> Workers;
    auto start = chrono::high_resolution_clock::now();
    for (int i = 0; i < ThisClient->start_data.thraed_count; i++) {
        int startElement = ElementsForOneThread * i;
        int endElement = (i == thread_count - 1) ? size : (startElement + ElementsForOneThread > size) ? size : startElement + ElementsForOneThread;
        Workers.emplace_back(thread(MultyThreadFunc, startElement, endElement, ref(arrayA), ref(arrayB), k, ref(ThisClient->result_data.result_array)));
    }
    for (auto& t : Workers) {
        if (t.joinable()) {
            t.join();
        }
    }
    auto end = chrono::high_resolution_clock::now();
    ThisClient->result_data.time = chrono::duration_cast<chrono::nanoseconds>(end - start).count() * 1e-9;
    ThisClient->working = false;
}

void ClilentThread(Client* ThisClient) {
    
    while (true) {
        Message receivedMessage;
        if (!receiveMessage(ThisClient, &receivedMessage))
        {
            if (strcmp(receivedMessage.data, "CONNECTION") == 0) {
                string tempMessage = "SERVER> Connection is stable. Your number is " + to_string(ThisClient->client_number);
                send_message(&ThisClient->socket, &tempMessage, true);
            }
            else if (strcmp(receivedMessage.data, "SEND_DATA") == 0) {
                if (!ThisClient->working) {
                    string tempMessage = "SERVER> Waiting for data";
                    text_mutex.lock();
                    cout << tempMessage << " from client number " << ThisClient->client_number << "..." << endl;
                    text_mutex.unlock();
                    send_message(&ThisClient->socket, &tempMessage, false);

                    int bytesRead = recv(ThisClient->socket, (char*)&ThisClient->start_data, sizeof(ThisClient->start_data), 0);
                    if (bytesRead == -1) {
                        lock_guard<mutex> _(text_mutex);
                        cerr << "SERVER> Error receiving data from client " << ThisClient->client_number << endl;
                        continue;
                    }
                    else if (bytesRead == 0) {
                        lock_guard<mutex> _(text_mutex);
                        cout << "SERVER> Client " << ThisClient->client_number << " disconnected." << endl;
                        break;
                    }
                    ThisClient->start_data.array_size = ntohl(ThisClient->start_data.array_size);
                    ThisClient->start_data.thraed_count = ntohl(ThisClient->start_data.thraed_count);
                    cout << "SERVER> Data received: array_size = " << ThisClient->start_data.array_size << ", thraed_count = " << ThisClient->start_data.thraed_count << endl;
                }
                else {
                    string tempMessage = "SERVER> We are working, please wait to the end of process";
                    send_message(&ThisClient->socket, &tempMessage, true);
                }
            }
            else if (strcmp(receivedMessage.data, "START") == 0) {
                if (!ThisClient->working) {
                    if (ThisClient->start_data.array_size == 0 || ThisClient->start_data.thraed_count == 0) {
                        string tempMessage = "SERVER> Please send valid data first";
                        send_message(&ThisClient->socket, &tempMessage, true);
                    }
                    else {
                        string tempMessage = "SERVER> Process started";
                        text_mutex.lock();
                        cout << tempMessage << " from client number " << ThisClient->client_number << "..." << endl;
                        text_mutex.unlock();
                        send_message(&ThisClient->socket, &tempMessage, false);
                        ThisClient->working = true;
                        thread t(WorkThread, ThisClient);
                        t.detach();
                    }
                }
                else {
                    string tempMessage = "SERVER> We are already working, please wait to the end of process";
                    send_message(&ThisClient->socket, &tempMessage, true);
                }
            }
            else if (strcmp(receivedMessage.data, "GET_STATUS") == 0) {
                if (ThisClient->working) {
                    string tempMessage = "SERVER> Status: In progress...";
                    send_message(&ThisClient->socket, &tempMessage, true);
                    }
                else if (ThisClient->start_data.array_size == 0 || ThisClient->start_data.thraed_count == 0 || ThisClient->result_data.result_array.empty()) {
                    string tempMessage = "SERVER> Status: Process didn't started";
                    send_message(&ThisClient->socket, &tempMessage, true);
                }
                else {
                    string tempMessage = "SERVER> Status: Process complete!";
                    send_message(&ThisClient->socket, &tempMessage, true);
                }
            }
            else if (strcmp(receivedMessage.data, "GET_RESULT") == 0) {
                if (ThisClient->working || ThisClient->start_data.array_size == 0 || ThisClient->start_data.thraed_count == 0) {
                    string tempMessage = "SERVER> No result yet :(";
                    send_message(&ThisClient->socket, &tempMessage, true);
                }
                else {
                    string tempMessage = "SERVER> Sent data: threads = " + to_string(ThisClient->start_data.thraed_count) + ", array size = " + to_string(ThisClient->start_data.array_size) + ". RESULT: time = " + to_string(ThisClient->result_data.time);
                    send_message(&ThisClient->socket, &tempMessage, true);
                }
            }
            else {
                string tempMessage = "SERVER> Please, send a valid command ;)";
                send_message(&ThisClient->socket, &tempMessage, true);
            }
        }
        else {
            lock_guard<mutex> _(text_mutex);
            cout << "SERVER> Client " << ThisClient->client_number << " disconnected" << endl;
            break;
        }
    }
    closesocket(ThisClient->socket);
}

int main() {

    WSAData wsaData;
    if (WSAStartup(0x0202, &wsaData)) {
        printf("SERVER> Error WSAStartup %d\n", WSAGetLastError());
        return -1;
    }
    SOCKET myserversocket;
    if ((myserversocket = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        printf("SERVER> Error socket %d\n", WSAGetLastError());
        WSACleanup();
        return -1;
    }
    sockaddr_in local_addr;
    local_addr.sin_family = AF_INET;
    local_addr.sin_port = htons(PORT);
    local_addr.sin_addr.s_addr = 0;

    if (bind(myserversocket, (sockaddr*)&local_addr, sizeof(local_addr))) {
        printf("SERVER> Error bind %d\n", WSAGetLastError());
        closesocket(myserversocket);
        WSACleanup();
        return -1;
    }
    if (listen(myserversocket, 0x100)) {
        printf("SERVER> Error listen %d\n", WSAGetLastError());
        closesocket(myserversocket);
        WSACleanup();
        return -1;
    }

    cout << "SERVER> Waiting for clients..." << endl;
    
    while (true) {
        SOCKET clientSocket = accept(myserversocket, nullptr, nullptr);
        client_count.fetch_add(1);
        Client newClient(clientSocket, client_count.load());
        Clients.insert({ client_count.load(), newClient });
        cout << "SERVER> Client " << newClient.client_number << " connected successfully!" << endl;
        thread thread(ClilentThread, &Clients[client_count.load()]);
        thread.detach();
    }
    closesocket(myserversocket);
    WSACleanup();
    return 0;
}



