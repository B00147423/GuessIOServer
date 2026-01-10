#pragma once
#include <unordered_map>
#include <memory>
#include <string>
#include "TwitchClient.h"
#include "server.h"
#include "GameProtocol.h"

class Server;

class TwitchBotManager {
public:
    TwitchBotManager(boost::asio::io_context& io, Server& server)
        : m_io(io), m_server(server) {
    }

    bool spawnBot(const std::string& oauth,
        const std::string& nick,
        const std::string& channel);
    void stopBot(const std::string& channel);
    void setCurrentRoom(const std::string& channel, const std::string& roomName);

    // attach a shared GameProtocol to all bots
    void setGameProtocol(std::shared_ptr<GameProtocol> gp) { gameProtocol_ = gp; }

private:
    boost::asio::io_context& m_io;
    Server& m_server;
    std::unordered_map<std::string, std::shared_ptr<TwitchClient>> m_bots;
    std::shared_ptr<GameProtocol> gameProtocol_; // NEW
};
