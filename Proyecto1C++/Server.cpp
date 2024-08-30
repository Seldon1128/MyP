#include <iostream>
#include <vector>
#include <thread>
#include <string>
#include <mutex>
#include <unistd.h>
#include <netinet/in.h>

std::vector<int> clients;
std::mutex clients_mutex;

void handle_client(int client_socket) {
    char buffer[1024];
    std::string welcome_msg = "Welcome to the chat!";
    send(client_socket, welcome_msg.c_str(), welcome_msg.length(), 0);

    while (true) {
        ssize_t recv_len = recv(client_socket, buffer, sizeof(buffer), 0);
        if (recv_len <= 0) {
            std::lock_guard<std::mutex> lock(clients_mutex);
            clients.erase(std::remove(clients.begin(), clients.end(), client_socket), clients.end());
            close(client_socket);
            break;
        }

        buffer[recv_len] = '\0';
        std::string message(buffer);

        std::lock_guard<std::mutex> lock(clients_mutex);
        for (int client : clients) {
            if (client != client_socket) {
                send(client, message.c_str(), message.length(), 0);
            }
        }
    }
}

int main() {
    int server_socket = socket(AF_INET, SOCK_STREAM, 0);

    sockaddr_in server_address;
    server_address.sin_family = AF_INET;
    server_address.sin_port = htons(1234);
    server_address.sin_addr.s_addr = INADDR_ANY;

    bind(server_socket, (sockaddr*)&server_address, sizeof(server_address));
    listen(server_socket, 5);

    std::cout << "Server is listening on port 1234..." << std::endl;

    while (true) {
        int client_socket = accept(server_socket, nullptr, nullptr);
        std::cout << "New client connected!" << std::endl;

        std::lock_guard<std::mutex> lock(clients_mutex);
        clients.push_back(client_socket);

        std::thread client_thread(handle_client, client_socket);
        client_thread.detach();
    }

    close(server_socket);
    return 0;
}
