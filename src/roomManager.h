    #pragma once
#include <unordered_map>
#include <unordered_set>
#include <memory>
#include <mutex>
#include <string>
#include <nlohmann/json.hpp>
#include "room.h"

class Server;   // forward declare
class Session;  // forward declare

class RoomManager {
public:
    RoomManager() : m_server(nullptr) {}
    void setServer(Server* server) { m_server = server; }
    void joinRoom(const std::string& roomId, std::shared_ptr<Session> s, const std::string& username);
    void leaveAll(std::shared_ptr<Session> s);
    void onMessage(std::shared_ptr<Session> s, const std::string& jsonMsg);
    Room* getCurrentRoom(const std::string& channel);

private:
    void handleJoin(std::shared_ptr<Session> s, const nlohmann::json& j, const std::string& roomId);
    void handleLeave(std::shared_ptr<Session> s, const nlohmann::json& j, const std::string& roomId);

    void handleChat(std::shared_ptr<Session> s, const nlohmann::json& j, const std::string& roomId);
    void handleStartRound(std::shared_ptr<Session> s, const nlohmann::json& j, const std::string& roomId);
    void handleGuess(std::shared_ptr<Session> s, const nlohmann::json& j, const std::string& roomId);
    void handleEndRound(const std::string& roomId);
    void handleStopBot(const nlohmann::json& j);
    void handleSpawnBot(const nlohmann::json& j);
    void handleMapTwitchRoom(const nlohmann::json& j);
    void handleStatus(const std::string& jsonMsg);


    void handleDraw(std::shared_ptr<Session> s, const nlohmann::json& j, const std::string& roomId);
    void handleClear(std::shared_ptr<Session> s, const nlohmann::json& j, const std::string& roomId);

    // NEW: Handle state restoration
    void handleRestoreState(std::shared_ptr<Session> s, const std::string& roomId);
    void cleanupAbandonedRooms(); // NEW: Clean up empty rooms
    void cleanupExpiredRooms(); // NEW: Clean up rooms inactive for 1+ hours

    std::unordered_map<std::string, Room> m_rooms;
    std::unordered_map<std::string, std::unordered_set<std::string>> m_joinedUsers;
    std::unordered_map<std::string, std::string> m_roomChannels; // Track which channel each room belongs to
    mutable std::mutex m_mutex;
    Server* m_server;
};
