#include <iostream>
#include <thread>
#include <string>
#include <arpa/inet.h>
#include <unistd.h>
#include <nlohmann/json.hpp>

using json = nlohmann::json;

void listen_for_messages(int socket) {
    char buffer[1024];
    while (true) {
        ssize_t recv_len = recv(socket, buffer, sizeof(buffer), 0);
        if (recv_len <= 0) {
            close(socket);
            break;
        }

        buffer[recv_len] = '\0';
        std::string received_message(buffer);

        // Parsear el mensaje JSON recibido
        json message_json = json::parse(received_message);
        std::cout << "Received: " << message_json["message"] << std::endl;
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

    // Pedir el nombre de usuario y enviar el mensaje de identificación
    std::string username;
    std::cout << "Bienvenido. Por favor, ingresa tu nombre: ";
    std::getline(std::cin, username);

    // Crear un JSON de identificación
    json identify_message;
    identify_message["type"] = "IDENTIFY";
    identify_message["username"] = username;

    std::string msg_to_send = identify_message.dump();
    send(client_socket, msg_to_send.c_str(), msg_to_send.length(), 0);

    std::string message;
    while (true) {
        std::getline(std::cin, message);

        // Crear un objeto JSON para enviar el mensaje
        json message_json;
        message_json["message"] = message;

        msg_to_send = message_json.dump();
        send(client_socket, msg_to_send.c_str(), msg_to_send.length(), 0);
    }

    close(client_socket);
    return 0;
}
