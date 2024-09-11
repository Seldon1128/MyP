#ifndef ROOM_H
#define ROOM_H

#include <iostream>
#include <map>
#include <vector>
#include <mutex>
#include <string>

struct ClientInfo {
    int socket;
    std::string name;
    std::string status;
};

struct Room {
    std::string name;
    std::vector<ClientInfo> clientsRoom;

    // Métodos estáticos
    static std::map<std::string, Room> rooms;
    static std::mutex rooms_mutex;

    static bool room_exists(const std::string& room_name);
    static void handle_new_room(const std::string& room_name);
    static void handle_join_room(const std::string& room_name, ClientInfo& client);
    static void handle_room_text(const std::string& room_name, const std::string& message, const ClientInfo& sender);
    static void send_message(int socket, const std::string& message);
};

#endif // ROOM_H
