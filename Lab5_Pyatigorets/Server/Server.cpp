#include <winsock2.h>
#include <ws2tcpip.h>
#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <thread>
#include <map>
#pragma comment(lib, "ws2_32.lib")

using namespace std;

#define PORT 8080
#define ROOT_DIR "html"
atomic<int> index_global(0);

struct Client {
    uint32_t client_number;
    SOCKET socket;
    Client(SOCKET newsocket, int newnumber) : socket(newsocket), client_number(newnumber) {}
    Client() {}
};
map<int, Client> Clients;

string readFile(const string& path) {
    ifstream file(path, ios::binary);
    if (!file.is_open()) return "";
    ostringstream ss;
    ss << file.rdbuf();
    return ss.str();
}

void sendResponse(SOCKET client, const string& status, string& content) {
    ostringstream response;
    response << "HTTP/1.1 " << status << "\r\nContent-Length: " << content.size() << "\r\nContent-Type: text/html\r\n\r\n";
    response << content;
    send(client, response.str().c_str(), response.str().length(), 0);
}

void handleClient(Client* client) {
    while (true) {
        char buffer[4096];
        int received = recv(client->socket, buffer, sizeof(buffer) - 1, 0);
        if (received <= 0) {
            break;
        }
        buffer[received] = '\0';

        istringstream request(buffer);
        string method, path;
        request >> method >> path;

        if (path == "/") path = "/index.html";
        string file = string(ROOT_DIR) + path;
        string content = readFile(file);

        if (content.empty()) {
            string body = "<html><body>404 Not Found</body></html>";
            sendResponse(client->socket, "404 Not Found", body);
        }
        else {
            sendResponse(client->socket, "200 OK", content);
        }
    }
    cout << "Client number " << client->client_number << " disconected." << endl;
    Clients.erase(client->client_number);
    closesocket(client->socket);
}

int main() {
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        cerr << "WSAStartup failed." << endl;
        return 1;
    }

    SOCKET serverSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (serverSocket == INVALID_SOCKET) {
        cerr << "Socket creation failed." << endl;
        WSACleanup();
        return 1;
    }

    sockaddr_in serverAddr;
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_addr.s_addr = INADDR_ANY;
    serverAddr.sin_port = htons(PORT);

    if (bind(serverSocket, (SOCKADDR*)&serverAddr, sizeof(serverAddr)) == SOCKET_ERROR) {
        cerr << "Bind failed." << endl;
        closesocket(serverSocket);
        WSACleanup();
        return 1;
    }

    if (listen(serverSocket, SOMAXCONN) == SOCKET_ERROR) {
        cerr << "Listen failed." << endl;
        closesocket(serverSocket);
        WSACleanup();
        return 1;
    }

    cout << "Server listening on port " << PORT << "...\n";

    while (true) {
        struct sockaddr_in clientAddr;
        int clientAddrLen = sizeof(clientAddr);
        SOCKET clientSocket = accept(serverSocket, (struct sockaddr*)&clientAddr, &clientAddrLen);
        if (clientSocket == INVALID_SOCKET) {
            cerr << "Accept failed\n";
            continue;
        }
        char client_addr[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &(clientAddr.sin_addr), client_addr, INET_ADDRSTRLEN);
        cout << "Connection accepted from " << client_addr << ":" << ntohs(clientAddr.sin_port);
        
        index_global.fetch_add(1);
        Client newClient(clientSocket, index_global.load());
        Clients.insert({ index_global.load(), newClient });
        cout << "  Client number is " << newClient.client_number << endl;

        thread(handleClient, &Clients[index_global.load()]).detach();
    }

    closesocket(serverSocket);
    WSACleanup();
    return 0;
}
