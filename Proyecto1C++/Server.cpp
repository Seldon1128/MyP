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

struct ClientInfo {
    int socket;
    std::string name;
};

std::vector<ClientInfo> clients;
std::mutex clients_mutex;

void handle_client(int client_socket) {
    char buffer[1024];
    std::string client_name;

    while (true) {
        ssize_t recv_len = recv(client_socket, buffer, sizeof(buffer), 0);
        if (recv_len <= 0) {
            std::lock_guard<std::mutex> lock(clients_mutex);
            clients.erase(std::remove_if(clients.begin(), clients.end(), 
                [&](const ClientInfo& client) { return client.socket == client_socket; }), clients.end());
            close(client_socket);
            break;
        }

        buffer[recv_len] = '\0';
        std::string received_message(buffer);

        // Convertir el mensaje recibido en JSON
        json message_json = json::parse(received_message);

        // Verificar si el tipo de mensaje es IDENTIFY
        if (message_json.contains("type") && message_json["type"] == "IDENTIFY") {
            client_name = message_json["username"];

            std::lock_guard<std::mutex> lock(clients_mutex);
            auto it = std::find_if(clients.begin(), clients.end(), [&](const ClientInfo& client) { return client.name == client_name; });

            if (it != clients.end()) {
                // Si el nombre de usuario ya existe, enviar respuesta de error
                json response;
                response["type"] = "RESPONSE";
                response["operation"] = "IDENTIFY";
                response["result"] = "USER_ALREADY_EXISTS";
                response["extra"] = client_name;
                std::string response_str = response.dump();
                send(client_socket, response_str.c_str(), response_str.length(), 0);
            } else {
                // Si el nombre de usuario no existe, añadir al cliente y enviar respuesta de éxito
                clients.push_back({client_socket, client_name});

                json response;
                response["type"] = "RESPONSE";
                response["operation"] = "IDENTIFY";
                response["result"] = "SUCCESS";
                response["extra"] = client_name;
                std::string response_str = response.dump();
                send(client_socket, response_str.c_str(), response_str.length(), 0);

                // Notificar a los demás clientes que un nuevo usuario se ha conectado
                json new_user_msg;
                new_user_msg["type"] = "NEW_USER";
                new_user_msg["username"] = client_name;
                std::string new_user_str = new_user_msg.dump();

                for (const auto& client : clients) {
                    if (client.socket != client_socket) {
                        send(client.socket, new_user_str.c_str(), new_user_str.length(), 0);
                    }
                }
            }
        } else {
            // Reenviar el mensaje a todos los clientes excepto al emisor
            std::lock_guard<std::mutex> lock(clients_mutex);
            for (const auto& client : clients) {
                if (client.socket != client_socket) {
                    std::string msg_to_send = message_json.dump();
                    send(client.socket, msg_to_send.c_str(), msg_to_send.length(), 0);
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

        std::thread client_thread(handle_client, client_socket);
        client_thread.detach();
    }

    close(server_socket);
    return 0;
}
