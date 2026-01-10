#include "TwitchBotManager.h"
#include "TwitchClient.h"
#include "server.h"
#include <iostream>

bool TwitchBotManager::spawnBot(const std::string& oauth,
    const std::string& nick,
    const std::string& channel) {
    auto it = m_bots.find(channel);
    if (it != m_bots.end()) {
        std::cout << "[WARN] Bot for channel " << channel
            << " already exists, ignoring spawn.\n";
        return false;
    }

    auto bot = std::make_shared<TwitchClient>(m_io, m_server, oauth, nick, channel);

    // attach GameProtocol
    if (gameProtocol_) {
        bot->setGameProtocol(gameProtocol_);
    }

    m_bots[channel] = bot;
    bot->connect();

    std::cout << "[INFO] Bot spawned for channel " << channel << "\n";
    return true;
}

void TwitchBotManager::stopBot(const std::string& channel) {
    auto it = m_bots.find(channel);
    if (it != m_bots.end()) {
        std::cout << "[INFO] Stopping bot for channel " << channel << "\n";
        it->second->disconnect();
        m_bots.erase(it);
    }
    else {
        std::cout << "[WARN] Tried to stop bot for channel "
            << channel << " but none exists.\n";
    }
}

void TwitchBotManager::setCurrentRoom(const std::string& channel, const std::string& roomName) {
    std::cout << "[DEBUG] setCurrentRoom called for channel: " << channel << ", room: " << roomName << std::endl;
    std::cout << "[DEBUG] m_bots size: " << m_bots.size() << std::endl;
    for (auto& pair : m_bots) {
        std::cout << "[DEBUG] Bot in map: " << pair.first << std::endl;
    }
    
    auto it = m_bots.find(channel);
    if (it != m_bots.end()) {
        it->second->setCurrentRoom(channel, roomName);
        std::cout << "[DEBUG] Set current room for channel " << channel
            << " to: " << roomName << std::endl;
    }
    else {
        std::cout << "[WARN] No bot found for channel " << channel
            << " when setting room " << roomName << std::endl;
    }
}
