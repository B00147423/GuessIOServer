#pragma once
#include <string>
#include <memory>

class Room;
class Server;

class GameProtocol {
public:
    GameProtocol(Server* server);

    // Handle raw command from Twitch or WebSocket
    void handleCommand(const std::string& username, const std::string& msg, const std::string& channel);

private:
    Server* server_;
};
