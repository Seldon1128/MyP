#include <iostream>
#include <thread>
#include <string>
#include <arpa/inet.h>
#include <unistd.h>
#include <nlohmann/json.hpp>
#include <algorithm>
#include <cctype>
#include <locale>

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
        } else if (message_json["type"] == "RESPONSE" && message_json["operation"] == "TEXT" && message_json["result"] == "NO_SUCH_USER"){
            std::cout << "El nombre de usuario '" << message_json["extra"] << "' no existe." << std::endl;
        } else if (message_json["type"] == "RESPONSE" && message_json["operation"] == "NEW_ROOM" && message_json["result"] == "SUCCESS"){
            std::cout << "El cuarto " << message_json["extra"] << " se creo exitosamente." << std::endl;
        } else  if (message_json["type"] == "RESPONSE" && message_json["operation"] == "NEW_ROOM" && message_json["result"] == "ROOM_ALREADY_EXISTS"){
            std::cout << "El nombre del cuarto " << message_json["extra"] << " ya existe en el servidor." << std::endl;
        } else if (message_json["type"] == "PUBLIC_TEXT_FROM"){ 
            std::string username = message_json["username"];
            std::string text = message_json["text"];
            std::cout << username << ": " << text << std::endl;
        }else if(message_json["type"] == "TEXT_FROM"){
            std::string username_priv = message_json["username"];
            std::string text_priv = message_json["text"];
            std::cout << "Mensaje Privado -> " << username_priv << ": " << text_priv << std::endl;
        } else if (message_json["type"] == "RESPONSE" && message_json["operation"] == "INVITE" && message_json["result"] == "NO_SUCH_USER"){
            std::cout << "Invitacion fallida. El nombre de usuario " << message_json["extra"] << " no existe." << std::endl;
        } else if (message_json["type"] == "RESPONSE" && message_json["operation"] == "INVITE" && message_json["result"] == "NO_SUCH_ROOM"){
            std::cout << "Invitacion fallida. El cuarto " << message_json["extra"] << " no existe." << std::endl;
        } else if(message_json["type"] == "RESPONSE" && message_json["operation"] == "JOIN_ROOM" && message_json["result"] == "SUCCESS"){
            std::string roomname = message_json["extra"];
            std::cout << "Cuarto: "<< roomname << " -> Ingreso exitoso a la sala."<< std::endl;
        } else if(message_json["type"] == "JOINED_ROOM"){
            std::string roomname = message_json["roomname"];
            std::string username = message_json["username"];
            std::cout << "Cuarto: "<< roomname << " -> el usuario: " << username << " ha ingresado a la sala."<< std::endl;
        } else if (message_json["type"] == "RESPONSE" && message_json["operation"] == "JOIN_ROOM" && message_json["result"] == "NO_SUCH_ROOM"){
            std::cout << "Ingreso fallido. El cuarto " << message_json["extra"] << " no existe." << std::endl;
        } else if (message_json["type"] == "RESPONSE" && message_json["operation"] == "JOIN_ROOM" && message_json["result"] == "NOT_INVITED"){
            std::cout << "Ingreso fallido. No estas invitado al cuarto: " << message_json["extra"] << "." << std::endl;
        } else if (message_json["type"] == "INVITATION"){
            std::string username_inv = message_json["username"];
            std::string roomname_inv = message_json["roomname"];
            std::cout << "Invitacion a sala -> " << username_inv << ": " << roomname_inv << std::endl;
        } else if (message_json["type"] == "USER_LIST"){
            std::cout << "Usuarios conectados y sus estados:\n";
            for (auto it = message_json["users"].begin(); it != message_json["users"].end(); ++it) {
                std::string username = it.key();  // Nombre del usuario
                std::string status = it.value();  // Estado del usuario
                std::cout << username << ": " << status << std::endl;
            }
        } else if(message_json["type"] == "NEW_STATUS"){
            std::string username = message_json["username"];
            std::string status = message_json["status"];
            std::cout << username << " ha cambiado de estado: " << status << std::endl;
        } else {
            // Mostrar otros tipos de mensajes no manejados explícitamente
            std::cout << "Received: " << message_json.dump() << std::endl;
        }
    }
}
// Función para eliminar espacios al inicio y al final de una cadena
std::string trim(const std::string& str) {
    // Encuentra la primera posición que no sea un espacio en blanco
    size_t start = str.find_first_not_of(" \t\n\r\f\v");
    // Encuentra la última posición que no sea un espacio en blanco
    size_t end = str.find_last_not_of(" \t\n\r\f\v");
    // Si no se encuentra ningún carácter que no sea espacio, retorna una cadena vacía.
    if (start == std::string::npos) {
        return ""; 
    }

    // Retorna la subcadena que está entre los caracteres que no son espacios
    return str.substr(start, end - start + 1);
}

// Función para convertir una cadena a mayúsculas
std::string to_upper(const std::string& str) {
    std::string result = str;
    // Recorre cada carácter de la cadena y conviértelo a mayúsculas
    std::transform(result.begin(), result.end(), result.begin(), [](unsigned char c) { return std::toupper(c); });
    return result;
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
    do{
        std::cout << "Bienvenido. Por favor, ingresa tu nombre: ";
        std::getline(std::cin, username);

        if (username.length() > 8){
            std::cout << "El nombre excede el límite de 8 caracteres. Inténtalo de nuevo.\n";
        }

    }while(username.length() > 8); // Repetir hasta que el nombre sea valido

    // Crear un JSON de identificación
    json identify_message;
    identify_message["type"] = "IDENTIFY";
    identify_message["username"] = username;

    std::string msg_to_send = identify_message.dump();
    send(client_socket, msg_to_send.c_str(), msg_to_send.length(), 0);

    std::string message;
    while (true) {
        std::getline(std::cin, message);

        // Eliminar espacios en blanco al inicio y al final, y convertir a mayúsculas
        std::string trimmed_message = trim(message);
        std::string uppercase_message = to_upper(trimmed_message);

        // Crear un objeto JSON para enviar el mensaje
        json message_json;

        // Agregar siguiente codigo para identificar el tipo de mensajes
        if (uppercase_message == "/USERS"){
            message_json["type"] = "USERS";
        } else if(uppercase_message == "/STATUS"){
            message_json["type"] = "STATUS";
            std::string opcion;
            std::cout << "Seleccina el estado al que quieres cambiar: \n 1. ACTIVE \n 2. BUSY \n 3. AWAY \n";
            std::getline(std::cin, opcion);
            std::string opcion1 = trim(opcion);
            if (opcion1 == "1"){
                message_json["status"] = "ACTIVE";
            } else if (opcion1 == "2"){
                message_json["status"] = "BUSY";
            } else if (opcion1 == "3"){
                message_json["status"] = "AWAY";
            } else {
                std::cout << "Opción no válida. Inténtalo de nuevo. Regresa al chat principal." << std::endl;
                continue;
            }
        } else if(uppercase_message == "/TEXT"){
            message_json["type"] = "TEXT";
            std::string username_msg;
            std::string mensaje_priv;
            std::cout << "Ingresa el username del usuario: ";
            std::getline(std::cin, username_msg);
            std::string username_msg1 = trim(username_msg);
            std::cout << "Ingresa el mensaje: ";
            std::getline(std::cin, mensaje_priv);
            message_json["username"] = username_msg1;
            message_json["text"] = mensaje_priv;
        }else if(uppercase_message == "/NEW_ROOM"){
            message_json["type"] = "NEW_ROOM";
            std::string roomname_msg;
            do{
                std::cout << "Ingresa el nombre del nuevo cuarto: ";
                std::getline(std::cin, roomname_msg);
                if (roomname_msg.length() > 16){
                    std::cout << "El nombre excede el límite de 16 caracteres. Inténtalo de nuevo.\n";
                }
            }while(roomname_msg.length() > 16); // Repetir hasta que el nombre sea valido
            message_json["roomname"] = roomname_msg;
        }else if(uppercase_message == "/INVITE"){
            message_json["type"] = "INVITE";
            std::string room_name;
            std::string invite_username;
            std::cout << "Ingresa el nombre del cuarto: ";
            std::getline(std::cin, room_name);
            std::cout << "Ingresa el username del usuario: ";
            std::getline(std::cin, invite_username);
            message_json["roomname"] = room_name;
            message_json["usernames"] = invite_username;
        } else if (uppercase_message == "/JOIN_ROOM"){
            message_json["type"] = "JOIN_ROOM";
            std :: string room_name;
            std::cout << "Ingresa el nombre del cuarto: ";
            std::getline(std::cin, room_name);
            message_json["roomname"] = room_name;
        }else{
            message_json["type"] = "PUBLIC_TEXT";
            message_json["text"] = message;
        }

        msg_to_send = message_json.dump();
        send(client_socket, msg_to_send.c_str(), msg_to_send.length(), 0);
    }
    close(client_socket);
    return 0;
}
