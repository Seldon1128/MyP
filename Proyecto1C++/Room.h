#ifndef ROOM_H
#define ROOM_H

#include <iostream>
#include <map>
#include <vector>
#include <mutex>
#include <string>
#include <nlohmann/json.hpp> // Para trabajar con JSON

// Crear alias de json
using json = nlohmann::json;

struct ClientInfo {
    int socket;
    std::string name;
    std::string status;
};

struct Room {
    std::string name;
    std::vector<ClientInfo> clientsRoom;
    std::vector<std::string> usernamesInvited; // Lista de nombres de Invitados a la sala

    // Métodos estáticos
    static std::map<std::string, Room> rooms;
    static std::mutex rooms_mutex;

    static bool room_exists(const std::string& room_name);
    static void handle_new_room(const std::string& room_name);
    static void handle_join_room(const std::string& room_name, ClientInfo& client);
    static void handle_room_text(const std::string& room_name, const std::string& message, const ClientInfo& sender);
    static void send_message(int socket, const std::string& message);
    static bool is_user_in_room(const std::string& room_name, const std::string& client_name);
    static bool is_user_invited(const std::string& room_name, const std::string& client_name);
    static void add_user_to_invited(const std::string& room_name, const std::string& client_name);
    static void broadcast_to_room(const std::string& room_name, const json& message_json);
};

#endif // ROOM_H
