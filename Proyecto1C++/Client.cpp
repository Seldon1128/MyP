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

        // Manejar los diferentes tipos de mensajes
        if (message_json["type"] == "NEW_USER") {
            // Si el mensaje es de tipo NEW_USER, imprimir que un nuevo usuario se ha unido
            std::string username = message_json["username"];
            std::cout << "Nuevo usuario se ha unido: " << username << std::endl;
        } else if (message_json["type"] == "RESPONSE" && message_json["operation"] == "IDENTIFY") {
            // Si el mensaje es de tipo RESPONSE y la operación es IDENTIFY
            if (message_json["result"] == "SUCCESS") {
                std::string extra = message_json["extra"];
                std::cout << "Bienvenido, " << extra << "!" << std::endl;
            } else if (message_json["result"] == "USER_ALREADY_EXISTS") {
                std::cout << "El nombre de usuario '" << message_json["extra"] << "' ya está en uso." << std::endl;
                // Cerrar el socket y terminar el programa si el nombre ya existe
                close(socket);
                exit(0);  // Finaliza el programa
            }
        } else if (message_json["type"] == "PUBLIC_TEXT_FROM"){ 
            std::string username = message_json["username"];
            std::string text = message_json["text"];
            std::cout << username << ": " << text << std::endl;

        }else {
            // Mostrar otros tipos de mensajes no manejados explícitamente
            std::cout << "Received: " << message_json.dump() << std::endl;
        }
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

        // Agregar siguiente codigo para identificar el tipo de mensajes
        json message_json;
        message_json["type"] = "PUBLIC_TEXT";
        message_json["text"] = message;

        msg_to_send = message_json.dump();
        send(client_socket, msg_to_send.c_str(), msg_to_send.length(), 0);
    }

    close(client_socket);
    return 0;
}
