#include <sys/socket.h> // Para la función send
#include <string>       // Para std::string
#include <iostream>     // Para std::cout, std::cerr, etc.
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

