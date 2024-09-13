#include <iostream>
#include <vector>
#include <thread>
#include <string>
#include <mutex>
#include <unistd.h>
#include <netinet/in.h>
#include <algorithm>
#include <nlohmann/json.hpp>
#include "Room.h"

using json = nlohmann::json;

std::vector<ClientInfo> clients;
std::mutex clients_mutex;

void update_client_status(int client_socket, const std::string& new_status) {
    // Bloquear el mutex para asegurar acceso exclusivo a la lista de clientes
    std::lock_guard<std::mutex> lock(clients_mutex);

    // Buscar el cliente por su socket
    for (auto& client : clients) {
        if (client.socket == client_socket) {
            // Actualizar el estado del cliente
            client.status = new_status;
            return; // Salir de la función después de actualizar el estado
        }
    }
}

void handle_client(int client_socket) {
    char buffer[1024];
    std::string client_name;
    std::string client_status = "ACTIVE";

    while (true) {
        ssize_t recv_len = recv(client_socket, buffer, sizeof(buffer), 0);
        // if para en caso de cliente desconecto o error
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

        // Verificar el tipo de mensaje
        if (message_json.contains("type") && message_json["type"] == "IDENTIFY") {
            client_name = message_json["username"];

            std::lock_guard<std::mutex> lock(clients_mutex); 
            // Checar si el nombre de usuario existe
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
                clients.push_back({client_socket, client_name, client_status});

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
                // Representación en cadena del JSON
                std::string new_user_str = new_user_msg.dump();

                for (const auto& client : clients) {
                    if (client.socket != client_socket) {
                        send(client.socket, new_user_str.c_str(), new_user_str.length(), 0);
                    }
                }
            }
        } else if (message_json.contains("type") && message_json["type"] == "PUBLIC_TEXT"){
            // Reenviar a los demás usuarios mensaje publico
                json public_text_msg;
                public_text_msg["type"] = "PUBLIC_TEXT_FROM";
                public_text_msg["username"] = client_name;
                public_text_msg["text"] = message_json["text"];
                std::string public_msg_str = public_text_msg.dump();

                for (const auto& client : clients) {
                    if (client.socket != client_socket) {
                        send(client.socket, public_msg_str.c_str(), public_msg_str.length(), 0);
                    }
                }

        }else if(message_json.contains("type") && message_json["type"] == "TEXT"){
            // Manejo de Textos Privados 
            std::string target_username = message_json["username"];
            std::string message_text = message_json["text"];
            bool user_found = false;

            std::lock_guard<std::mutex> lock(clients_mutex); // Asegurar acceso exclusivo a la lista de clientes

            // Inicializar un iterador para recorrer la lista de clientes
            auto it = clients.begin();

            // Recorrer la lista de clientes para buscar al destinatario
            while (it != clients.end()) {
            // Comprobar si el nombre del cliente actual coincide con el nombre del destinatario
                if (it->name == target_username) {
                    user_found = true;  // Usuario encontrado
                    break;  // Salir del bucle
                }
                ++it;  // Avanzar al siguiente cliente en la lista
            }

            json private_text_msg;
            if (user_found){
                private_text_msg["type"] = "TEXT_FROM";
                private_text_msg["username"] = client_name;
                private_text_msg["text"] = message_text;
                std::string private_msg_str = private_text_msg.dump();
                send(it->socket, private_msg_str.c_str(), private_msg_str.length(), 0);
            } else {
                private_text_msg["type"] = "RESPONSE";
                private_text_msg["operation"] = "TEXT";
                private_text_msg["result"] = "NO_SUCH_USER";
                private_text_msg["extra"] = target_username;
                std::string private_msg_str = private_text_msg.dump();
                send(client_socket, private_msg_str.c_str(), private_msg_str.length(), 0);
            }

        }else if(message_json.contains("type") && message_json["type"] == "USERS"){
            // El servidor responde un diccionario con los nombres de usuario y sus estados
            json response;
            response["type"] = "USER_LIST";
            
            json users;
            {
                std::lock_guard<std::mutex> lock(clients_mutex);
                for (const auto& client : clients) {
                    users[client.name] = client.status;
                }
            }

            response["users"] = users;
            std::string response_str = response.dump();
            send(client_socket, response_str.c_str(), response_str.length(), 0);

        }else if (message_json.contains("type") && message_json["type"] == "STATUS"){
             std::string new_status = message_json["status"];
            // Verificar si el nuevo estado es igual al estado actual del cliente
            if (client_status == new_status) {
                continue; // Si el estado es el mismo, no hacer nada y continuar con el próximo ciclo
            }

            // Actualizar el estado del cliente
            update_client_status(client_socket, new_status);

            // Notificar a los demás clientes que un usuario se ha cambiado su estado
            json response;
            response["type"] = "NEW_STATUS";
            response["username"] = client_name;
            response["status"] = new_status;

            // Representación en cadena del JSON
            std::string response_str = response.dump();

            // Enviar el mensaje a todos los clientes excepto al que hizo el cambio
            for (const auto& client : clients) {
                if (client.socket != client_socket) {
                    send(client.socket, response_str.c_str(), response_str.length(), 0);
                }
            }
        } else if(message_json.contains("type") && message_json["type"] == "NEW_ROOM"){
            std::string new_room_name = message_json["roomname"];
            json response;

            if (Room::room_exists(new_room_name)) {
                //Caso en que el nombre del cuarto existe
                response["type"] = "RESPONSE";
                response["operation"] = "NEW_ROOM";
                response["result"] = "ROOM_ALREADY_EXISTS";
                response["extra"] = new_room_name;
            } else {
                //Caso en que el nombre del cuarto no existe se crea uno y se agrega a el
                response["type"] = "RESPONSE";
                response["operation"] = "NEW_ROOM";
                response["result"] = "SUCCESS";
                response["extra"] = new_room_name;
                Room::handle_new_room(new_room_name);
                // Crear e inicializar una instancia de ClientInfo para el cliente que se unirá al cuarto
                ClientInfo clientInfo;
                clientInfo.socket = client_socket;  // Asignar el socket del cliente
                clientInfo.name = client_name;      // Asignar el nombre del cliente
                clientInfo.status = client_status;  // Asignar el estado del cliente
                Room::handle_join_room(new_room_name, clientInfo);
            }
            std::string response_str = response.dump();
            send(client_socket, response_str.c_str(), response_str.length(), 0);

        } else if (message_json.contains("type") && message_json["type"] == "INVITE"){
            // Similar a Manejo de Textos Privados comprobación si el usuario existe
            std::string target_username = message_json["usernames"];
            std::string room_name = message_json["roomname"];
            bool user_found = false;

            std::lock_guard<std::mutex> lock(clients_mutex); // Asegurar acceso exclusivo a la lista de clientes

            // Inicializar un iterador para recorrer la lista de clientes
            auto it = clients.begin();

            // Recorrer la lista de clientes para buscar al destinatario
            while (it != clients.end()) {
            // Comprobar si el nombre del cliente actual coincide con el nombre del destinatario
                if (it->name == target_username) {
                    user_found = true;  // Usuario encontrado
                    break;  // Salir del bucle
                }
                ++it;  // Avanzar al siguiente cliente en la lista
            }

            if (user_found){
                // Checar si cuarto existe
                if(Room::room_exists(room_name)){
                    //Checar si el que envía invitacion esta en el cuarto
                    if (Room::is_user_in_room(room_name, client_name)){
                        // Checar si el usuario a invitar esta en el cuarto
                        if (Room::is_user_invited(room_name, target_username)){
                            // No hacer nada
                        } else {
                            // El usuario a invitar no esta en el cuarto y procedemos a invitarlo
                            // Mandar mensaje
                            json response;
                            response["type"] = "INVITATION";
                            response["username"] = client_name;
                            response["roomname"] = room_name;
                            std::string response_str = response.dump();
                            send(it->socket, response_str.c_str(), response_str.length(), 0);
                            // Agregarlo a la lista de invitados del cuarto
                            Room::add_user_to_invited(room_name, target_username);

                        }
                    } else {
                        // el que envia invitacion no esta en el cuarto
                    }

                } else {
                    // enviar json NO_SUCH_ROOM
                    json response;
                    response["type"] = "RESPONSE";
                    response["operation"] = "INVITE";
                    response["result"] = "NO_SUCH_ROOM";
                    response["extra"] = room_name;
                    std::string response_str = response.dump();
                    send(client_socket, response_str.c_str(), response_str.length(), 0);
                }
            } else {
                // enviar json NO_SUCH_USER
                json response;
                response["type"] = "RESPONSE";
                response["operation"] = "INVITE";
                response["result"] = "NO_SUCH_USER";
                response["extra"] = target_username;
                std::string response_str = response.dump();
                send(client_socket, response_str.c_str(), response_str.length(), 0);
            }

        } else if(message_json.contains("type") && message_json["type"] == "JOIN_ROOM"){
            std::string room_name = message_json["roomname"];
            if(Room::room_exists(room_name)){
                if(Room::is_user_invited(room_name, client_name)){
                    json response1;
                    response1["type"] = "JOINED_ROOM";
                    response1["roomname"] = room_name;
                    response1["username"] = client_name;

                    Room::broadcast_to_room(room_name, response1, client_socket);

                    // Crear e inicializar una instancia de ClientInfo para el cliente que se unirá al cuarto
                    ClientInfo clientInfo;
                    clientInfo.socket = client_socket;  // Asignar el socket del cliente
                    clientInfo.name = client_name;      // Asignar el nombre del cliente
                    clientInfo.status = client_status;  // Asignar el estado del cliente
                    Room::handle_join_room(room_name, clientInfo);
                    json response;
                    response["type"] = "RESPONSE";
                    response["operation"] = "JOIN_ROOM";
                    response["result"] = "SUCCESS";
                    response["extra"] = room_name;
                    std::string response_str = response.dump();
                    send(client_socket, response_str.c_str(), response_str.length(), 0);


                } else {
                    // JSON NOT_INVITED
                    json response;
                    response["type"] = "RESPONSE";
                    response["operation"] = "JOIN_ROOM";
                    response["result"] = "NOT_INVITED";
                    response["extra"] = room_name;
                    std::string response_str = response.dump();
                    send(client_socket, response_str.c_str(), response_str.length(), 0);
                }
            }else{
                // JSON NO_SUCH_ROOM
                json response;
                response["type"] = "RESPONSE";
                response["operation"] = "JOIN_ROOM";
                response["result"] = "NO_SUCH_ROOM";
                response["extra"] = room_name;
                std::string response_str = response.dump();
                send(client_socket, response_str.c_str(), response_str.length(), 0);
            }
        }else if(message_json.contains("type") && message_json["type"] == "ROOM_USERS"){
            std::string room_name = message_json["roomname"];
            if(Room::room_exists(room_name)){
                if(Room::is_user_in_room(room_name, client_name)){
                    // JSON para enviar la lista de usuarios del cuarto
                    json response;
                    response["type"] = "ROOM_USER_LIST";
                    response["roomname"] = room_name;
                    json users;

                    {
                        // Obtener la lista de usuarios en el cuarto
                        std::lock_guard<std::mutex> lock(Room::rooms_mutex);
                        const auto& room = Room::rooms[room_name];  // Obtiene la sala

                        // Bloquear el acceso a la lista global de clientes (usuarios del servidor)
                        std::lock_guard<std::mutex> lock_clients(clients_mutex);

                        // Recorre la lista de clientes en la sala
                        for (const auto& client : room.clientsRoom) {
                            // Buscar el estado del usuario en la lista global de clientes del servidor
                            auto it = std::find_if(clients.begin(), clients.end(), [&](const ClientInfo& c) {
                                return c.name == client.name;
                            });
                            // Si el usuario se encuentra en la lista global, agrega su estado
                            if (it != clients.end()) {
                                users[client.name] = it->status;  // Añadir el estado del usuario
                            } 
                        }
                    }
                    response["users"] = users;  // Añadir lista de usuarios con sus estados
                    std::string response_str = response.dump();
                    send(client_socket, response_str.c_str(), response_str.length(), 0);
                } else{
                    //json not joined
                    json response;
                    response["type"] = "RESPONSE";
                    response["operation"] = "ROOM_USERS";
                    response["result"] = "NOT_JOINED";
                    response["extra"] = room_name;
                    std::string response_str = response.dump();
                    send(client_socket, response_str.c_str(), response_str.length(), 0);
                }
            } else {
                //json no such room
                json response;
                response["type"] = "RESPONSE";
                response["operation"] = "ROOM_USERS";
                response["result"] = "NO_SUCH_ROOM";
                response["extra"] = room_name;
                std::string response_str = response.dump();
                send(client_socket, response_str.c_str(), response_str.length(), 0);
            }
        } else if(message_json.contains("type") && message_json["type"] == "ROOM_TEXT"){
            std::string room_name = message_json["roomname"];
            std::string text = message_json["text"];

            if(Room::room_exists(room_name)){
                if(Room::is_user_in_room(room_name, client_name)){
                    // JSON para enviar mensaje
                    json response;
                    response["type"] = "ROOM_TEXT_FROM";
                    response["roomname"] = room_name;
                    response["username"] = client_name;
                    response["text"] = text;

                    // Enviar el mensaje a todos los clientes en el cuarto excepto al emisor
                    Room::broadcast_to_room(room_name, response, client_socket);

                    
                } else{
                    //json not joined
                    json response;
                    response["type"] = "RESPONSE";
                    response["operation"] = "ROOM_TEXT";
                    response["result"] = "NOT_JOINED";
                    response["extra"] = room_name;
                    std::string response_str = response.dump();
                    send(client_socket, response_str.c_str(), response_str.length(), 0);
                }
            } else {
                //json no such room
                json response;
                response["type"] = "RESPONSE";
                response["operation"] = "ROOM_TEXT";
                response["result"] = "NO_SUCH_ROOM";
                response["extra"] = room_name;
                std::string response_str = response.dump();
                send(client_socket, response_str.c_str(), response_str.length(), 0);
            }

        } else {
            // Reenviar el mensaje a todos los clientes excepto al emisor, el json se presenta entero en este else
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
