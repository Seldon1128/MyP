#include <sys/socket.h> // Para la función send
#include <string>       // Para std::string
#include <iostream>     // Para std::cout, std::cerr, etc.
#include <nlohmann/json.hpp> // Para trabajar con JSON
#include "Room.h"       // Para la definición de Room y ClientInfo

std::map<std::string, Room> Room::rooms;
std::mutex Room::rooms_mutex;


//Hacer un nuevo cuarto
void Room::handle_new_room(const std::string& room_name) {
    std::lock_guard<std::mutex> lock(rooms_mutex);
    if (rooms.find(room_name) == rooms.end()) {
        rooms[room_name] = Room{room_name};
    }
}

//Entrar a un cuarto
void Room::handle_join_room(const std::string& room_name, ClientInfo& client) {
    std::lock_guard<std::mutex> lock(rooms_mutex);
    if (rooms.find(room_name) != rooms.end()) {
        rooms[room_name].clientsRoom.push_back(client);
    }
}

//Mensaje en un cuarto
void Room::handle_room_text(const std::string& room_name, const std::string& message, const ClientInfo& sender) {
    std::lock_guard<std::mutex> lock(rooms_mutex);
    if (rooms.find(room_name) != rooms.end()) {
        // Enviar mensaje a todos los clientes en la sala
        for (const auto& client : rooms[room_name].clientsRoom) {
            // Supongo que tienes una función send_message para enviar el mensaje
            send_message(client.socket, message);  // Esta función debe ser definida en otro lugar
        }
    }
}

// send_message
void Room::send_message(int socket, const std::string& message) {
    send(socket, message.c_str(), message.size(), 0);
}

// Implementación del método para verificar si un cuarto existe
bool Room::room_exists(const std::string& room_name) {
    std::lock_guard<std::mutex> lock(rooms_mutex);
    return rooms.find(room_name) != rooms.end();
}

// Verificar si un usuario está en el cuarto
bool Room::is_user_in_room(const std::string& room_name, const std::string& client_name) {
    std::lock_guard<std::mutex> lock(rooms_mutex);
    auto it = rooms.find(room_name);
    if (it != rooms.end()) {
        const Room& room = it->second;
        for (const auto& client : room.clientsRoom) {
            if (client.name == client_name) {
                return true; // Usuario encontrado en la lista de clientes
            }
        }
    }
    return false; // Usuario no encontrado en la lista de clientes
}

// Verificar si un usuario está en la lista de invitados
bool Room::is_user_invited(const std::string& room_name, const std::string& client_name) {
    std::lock_guard<std::mutex> lock(rooms_mutex);
    auto it = rooms.find(room_name);
    if (it != rooms.end()) {
        const Room& room = it->second;
        for (const auto& invited_name : room.usernamesInvited) {
            if (invited_name == client_name) {
                return true; // Usuario encontrado en la lista de invitados
            }
        }
    }
    return false; // Usuario no encontrado en la lista de invitados
}

// Método para agregar un usuario a la lista de invitados de un cuarto
void Room::add_user_to_invited(const std::string& room_name, const std::string& client_name) {
    // Proteger acceso concurrente a los cuartos con mutex
    std::lock_guard<std::mutex> lock(rooms_mutex);

    // Buscar si el cuarto existe
    auto it = rooms.find(room_name);
    
    if (it != rooms.end()) {  // Si el cuarto existe
        Room& room = it->second;  // Obtenemos la referencia al cuarto
        
        // Verificamos si el usuario ya está en la lista de invitados
        if (std::find(room.usernamesInvited.begin(), room.usernamesInvited.end(), client_name) == room.usernamesInvited.end()) {
            // Si no está, lo añadimos
            room.usernamesInvited.push_back(client_name);
        } else {
            // Si ya está, no lo añadimos
        }
    } else {
        // El cuarto no existe.
    }
}

// Función para enviar un JSON a todos los clientes en un cuarto excepto al emisor
void Room::broadcast_to_room(const std::string& room_name, const json& message_json, int sender_socket) {
    std::lock_guard<std::mutex> lock(rooms_mutex); // Proteger acceso concurrente a 'rooms'

    // Convertir el JSON en una cadena
    std::string message_str = message_json.dump();

    // Obtener el cuarto (ya se asume que existe)
    Room& room = rooms[room_name];

    // Enviar el mensaje a todos los clientes en el cuarto excepto al que lo mandó
    for (const auto& client : room.clientsRoom) {
        if (client.socket != sender_socket) {
            send(client.socket, message_str.c_str(), message_str.length(), 0);
        }
    }
}



