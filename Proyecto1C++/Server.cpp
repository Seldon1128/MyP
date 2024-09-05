#include <iostream>
#include <vector>
#include <thread>
#include <string>
#include <mutex>
#include <unistd.h>
#include <netinet/in.h>
#include <algorithm>
#include <nlohmann/json.hpp>

using json = nlohmann::json;

std::vector<int> clients;
std::mutex clients_mutex;

void handle_client(int client_socket) {
    char buffer[1024];

    while (true) {
        ssize_t recv_len = recv(client_socket, buffer, sizeof(buffer), 0);
        if (recv_len <= 0) {
            std::lock_guard<std::mutex> lock(clients_mutex);
            clients.erase(std::remove(clients.begin(), clients.end(), client_socket), clients.end());
            close(client_socket);
            break;
        }

        buffer[recv_len] = '\0';
        std::string received_message(buffer);

        // Convertir el mensaje recibido en JSON
        json message_json = json::parse(received_message);

        // Verificar si el tipo de mensaje es IDENTIFY
        if (message_json.contains("type") && message_json["type"] == "IDENTIFY") {
            std::string username = message_json["username"];
            std::cout << "Usuario identificado: " << username << std::endl;

            // Aquí puedes hacer algo con el nombre del usuario, como almacenarlo o enviarlo a otros clientes

            // Crear y enviar un mensaje de bienvenida personalizado en formato JSON
            json welcome_msg;
            welcome_msg["message"] = "¡Bienvenido, " + username + "!";
            std::string welcome_str = welcome_msg.dump();
            send(client_socket, welcome_str.c_str(), welcome_str.length(), 0);
        } else {
            // Reenviar el mensaje a todos los clientes excepto al emisor
            std::lock_guard<std::mutex> lock(clients_mutex);
            for (int client : clients) {
                if (client != client_socket) {
                    std::string msg_to_send = message_json.dump();
                    send(client, msg_to_send.c_str(), msg_to_send.length(), 0);
                }
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
