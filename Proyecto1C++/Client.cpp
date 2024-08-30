#include <iostream>
#include <thread>
#include <string>
#include <arpa/inet.h>
#include <unistd.h>

void listen_for_messages(int socket) {
    char buffer[1024];
    while (true) {
        ssize_t recv_len = recv(socket, buffer, sizeof(buffer), 0);
        if (recv_len <= 0) {
            close(socket);
            break;
        }

        buffer[recv_len] = '\0';
        std::cout << buffer << std::endl;
    }
}

int main() {
    int client_socket = socket(AF_INET, SOCK_STREAM, 0);

    sockaddr_in server_address;
    server_address.sin_family = AF_INET;
    server_address.sin_port = htons(1234);
    inet_pton(AF_INET, "127.0.0.1", &server_address.sin_addr);

    connect(client_socket, (sockaddr*)&server_address, sizeof(server_address));

    std::thread listener_thread(listen_for_messages, client_socket);
    listener_thread.detach();

    std::string message;
    while (true) {
        std::getline(std::cin, message);
        send(client_socket, message.c_str(), message.length(), 0);
    }

    close(client_socket);
    return 0;
}
