#include "GameProtocol.h"
#include "room.h"
#include "server.h"
#include "roomManager.h"
#include <iostream>
#include <algorithm>

GameProtocol::GameProtocol(Server* server)
    : server_(server) {
}

void GameProtocol::handleCommand(const std::string& username, const std::string& msg, const std::string& channel) {
    std::cout << "[DEBUG] GameProtocol::handleCommand called with username: " << username << ", msg: " << msg << ", channel: " << channel << std::endl;
    
    if (!server_) {
        std::cout << "[DEBUG] No server set in GameProtocol!" << std::endl;
        return;
    }
        
    // Get the current room for this channel
    Room* room = server_->getRoomManager().getCurrentRoom(channel);
    if (!room) {
        std::cout << "[TWITCH] No mapped room for Twitch channel: " << channel << std::endl;
        return;
    }
    std::cout << "[DEBUG] Found current room for channel " << channel << std::endl;

    std::string lower = msg;
    std::transform(lower.begin(), lower.end(), lower.begin(), ::tolower);

    // --- Join ---
    if (lower.rfind("!join", 0) == 0) {
        std::cout << "[PROTO] " << username << " joined the game\n";
        room->join(nullptr, username); // Twitch users have no Session - this will broadcast the join message
        return;
    }

    // --- Guess ---
    if (lower.rfind("!guess ", 0) == 0) {
        std::string guess = lower.substr(7);
        std::cout << "[PROTO] " << username << " guessed: " << guess << "\n";
        if (room) {
            room->handleGuess(username, guess);
        } else {
            std::cout << "[ERROR] Room is null when processing guess from " << username << std::endl;
        }
        return;
    }

    // --- Start round (streamer only) ---
    if (lower == "!start") {
        std::string word = ""; // TODO: fetch from FastAPI
        std::cout << "[PROTO] Starting round with word: " << word << "\n";
        room->startRound(word);
        return;
    }

    // --- Fallback: treat any message as a guess attempt ---
    room->handleGuess(username, lower);
}
