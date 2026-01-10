#include "roomManager.h"
#include "session.h"
#include "server.h"
#include "TwitchClient.h"      // fixes TwitchClient errors
#include <iostream>

using json = nlohmann::json;

void RoomManager::joinRoom(const std::string& roomId, std::shared_ptr<Session> s, const std::string& username) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_rooms[roomId].join(s, username);
}

void RoomManager::leaveAll(std::shared_ptr<Session> s) {
    std::lock_guard<std::mutex> lock(m_mutex);
    for (auto& pair : m_rooms) {
        const std::string& id = pair.first;
        Room& room = pair.second;

        if (room.leave(s)) {
            json sysMsg = {
                {"type", "system"},
                {"room", id},
                {"payload", "Streamer disconnected, lobby cleared"}
            };
            room.broadcast(sysMsg.dump());

            // clear players too if streamer disconnects
            room.resetLobby();
        }
    }
}

static std::string normalizeRoom(const std::string& roomId) {
    if (!roomId.empty() && roomId[0] == '#')
        return roomId.substr(1);
    return roomId;
}
void RoomManager::handleJoin(std::shared_ptr<Session> s, const nlohmann::json& j, const std::string& roomId) {
    // Clean up any abandoned rooms first
    cleanupAbandonedRooms();
    
    // Also clean up expired rooms (inactive for 1+ hours)
    cleanupExpiredRooms();

    std::string username;

    // Accept both string and object payloads
    if (j["payload"].is_string()) {
        username = j["payload"].get<std::string>();
    }
    else if (j["payload"].is_object()) {
        username = j["payload"].value("username", "");
    }

    if (username.empty()) return;

    Room* room;
    bool isNewRoom = false;
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        auto it = m_rooms.find(roomId);
        if (it == m_rooms.end()) {
            // This is a new room being created
            isNewRoom = true;
            std::cout << "[ROOM] Creating new room: " << roomId << std::endl;
        } else {
            // Room exists - just update bot's current room, don't reset players
            std::cout << "[ROOM] Reconnecting to existing room: " << roomId << std::endl;
            
            // Update the bot's current room to this room
            if (m_server) {
                std::string channel = j.value("channel", "");
                if (!channel.empty()) {
                    m_roomChannels[roomId] = channel;
                    m_server->setCurrentRoom("#" + channel, roomId);
                    std::cout << "[ROOM] Updated bot's current room to: " << roomId << std::endl;
                }
            }
        }
        room = &m_rooms[roomId];
    }
    
    // If this is a new room, set it as the current room for the Twitch bot
    if (isNewRoom && m_server) {
        // Extract channel from the join message
        std::string channel = j.value("channel", "");
        if (!channel.empty()) {
            // Clear any existing room for this channel first
            for (auto it = m_roomChannels.begin(); it != m_roomChannels.end();) {
                if (it->second == channel) {
                    std::cout << "[ROOM] Removing old room entry: " << it->first << " for channel " << channel << std::endl;
                    it = m_roomChannels.erase(it);
                } else {
                    ++it;
                }
            }
            
            // Store the channel this room belongs to
            m_roomChannels[roomId] = channel;
            std::cout << "[ROOM] Room " << roomId << " belongs to channel " << channel << std::endl;
            
            // Set this as the current room for that channel's Twitch bot
            m_server->setCurrentRoom("#" + channel, roomId);
            std::cout << "[ROOM] Bot connected to new room: " << roomId << std::endl;
        } else {
            std::cout << "[WARN] No channel specified for new room " << roomId << std::endl;
        }
    }

    if (room->hasPlayer(username)) {
        std::cout << "[SPAM] Duplicate join from " << username << " (replaying state)\n";
        if (s) {
            room->join(s, username);     // attach new session
            room->replayPlayers(s);      // send full player list
            room->replayHistory(s);      // send all strokes
        }
        return;
    }

    // First-time join
    room->join(s, username);
}




void RoomManager::handleLeave(std::shared_ptr<Session> s, const nlohmann::json& j, const std::string& roomId) {
    auto it = m_rooms.find(roomId);
    if (it != m_rooms.end()) {
        Room& room = it->second;

        // Check if this is a streamer leaving (intentional) vs refresh (unintentional)
        bool isStreamerLeaving = j.value("intentional", false);
        
        if (isStreamerLeaving) {
            // Streamer clicked back to menu - clear all players
            for (auto& [uname, p] : room.getPlayers()) {
                nlohmann::json leaveMsg = {
                    {"type", "leave"},
                    {"payload", {
                        {"id", p.id},
                        {"username", uname}
                    }}
                };
                room.broadcast(leaveMsg.dump());
            }
            room.resetLobby();
        }
        
        room.leave(s);

        // Clean up abandoned rooms
        if (room.empty()) {
            std::cout << "[ROOM] Room " << roomId << " is empty, removing it" << std::endl;
            m_rooms.erase(it);
        }
    }
}


void RoomManager::handleChat(std::shared_ptr<Session>, const json& j, const std::string& roomId) {
    std::string payload = j.value("payload", "");
    if (!roomId.empty() && !payload.empty()) {
        std::cout << "[DEBUG] handleChat called with payload=" << payload << std::endl; // test line
        json chatMsg = { {"type","chat"}, {"room",roomId}, {"payload",payload} };
        m_rooms[roomId].broadcast(chatMsg.dump());
    }
}

void RoomManager::handleEndRound(const std::string& roomId) {
    std::lock_guard<std::mutex> lock(m_mutex);
    auto it = m_rooms.find(roomId);
    if (it != m_rooms.end()) {
        std::cout << "[ROOM] Ending round for room: " << roomId << std::endl;
        it->second.endRound();
    } else {
        std::cout << "[WARN] Attempted to end round for non-existent room: " << roomId << std::endl;
    }
}

void RoomManager::handleStopBot(const json& j) {
    std::string channel = j.value("channel", "");
    if (m_server) {
        // Clear all players from the current room before stopping the bot
        Room* currentRoom = getCurrentRoom(channel);
        if (currentRoom) {
            std::cout << "[ROOM] Clearing all players from room before stopping bot for channel: " << channel << std::endl;
            currentRoom->resetLobby();
        }
        
        m_server->stopBot(channel);
        std::cout << "ADMIN Stopped Twitch bot for channel: " << channel << "\n";
    }
}

void RoomManager::handleSpawnBot(const json& j) {
    std::string oauth = j.value("oauth", "");
    std::string nick = j.value("nick", "");
    std::string channel = j.value("channel", "");

    if (m_server) {
        bool spawned = m_server->spawnBot(oauth, nick, channel);
        if (spawned) {
            std::cout << "ADMIN Spawned Twitch bot for channel: " << channel << "\n";
        }
        else {
            std::cout << "[ADMIN] Bot for channel " << channel << " already exists, ignoring spawn.\n";
        }
    }
}

void RoomManager::handleMapTwitchRoom(const json& j) {
    std::string twitchName = j.value("payload", json::object()).value("twitch_name", "");
    std::string roomId = j.value("payload", json::object()).value("room_id", "");
    
    if (twitchName.empty() || roomId.empty()) {
        std::cout << "[ERROR] map_twitch_room missing required fields: twitch_name=" << twitchName << ", room_id=" << roomId << std::endl;
        return;
    }
    
    std::cout << "[ROOM] Mapping Twitch channel " << twitchName << " to room " << roomId << std::endl;
    
    // Store the mapping
    m_roomChannels[roomId] = twitchName;
    
    // Set the current room for the Twitch bot
    if (m_server) {
        m_server->setCurrentRoom("#" + twitchName, roomId);
        std::cout << "[ROOM] Set current room for Twitch bot #" << twitchName << " to " << roomId << std::endl;
    }
}

void RoomManager::handleStatus(const std::string& jsonMsg) {
    if (m_server) {
        m_server->broadcast(jsonMsg);
    }
}

void RoomManager::handleDraw(std::shared_ptr<Session> s, const json& j, const std::string& roomId) {
    if (roomId.empty()) return;

    json drawMsg = {
        {"type", "draw"},
        {"room", roomId},
        {"payload", j["payload"]}
    };

    // store in room history
    m_rooms[roomId].addStroke(drawMsg);

    // broadcast to all
    m_rooms[roomId].broadcast(drawMsg.dump());
}

void RoomManager::handleClear(std::shared_ptr<Session> s, const json& j, const std::string& roomId) {
    if (roomId.empty()) return;

    json clearMsg = {
        {"type", "clear"},
        {"room", roomId}
    };

    // clear room history
    m_rooms[roomId].clearHistory();

    // broadcast clear
    m_rooms[roomId].broadcast(clearMsg.dump());
}

void RoomManager::handleRestoreState(std::shared_ptr<Session> s, const std::string& roomId) {
    std::cout << "[DEBUG] handleRestoreState called for room: " << roomId << std::endl;
    if (roomId.empty() || !s) return;

    auto it = m_rooms.find(roomId);
    if (it != m_rooms.end()) {
        const Room& room = it->second;

        // Get data outside of any potential locks
        std::vector<std::string> playerUsernames;
        std::vector<json> strokeHistory;

        {
            std::lock_guard<std::mutex> lock(m_mutex);
            // Fix: Get the data first, then copy it properly
            auto usernames = room.getPlayerUsernames();
            playerUsernames.assign(usernames.begin(), usernames.end());
            strokeHistory = room.getStrokeHistory();
        }

        // Send current state back to client
        json response;
        response["type"] = "current_state";
        response["payload"]["players"] = playerUsernames;
        response["payload"]["strokes"] = strokeHistory;
        
        // Include round state if active
        const Round& currentRound = room.getCurrentRound();
        if (currentRound.active) {
            auto now = std::chrono::steady_clock::now();
            auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(now - currentRound.startTime);
            int timeLeft = currentRound.duration - elapsed.count();
            if (timeLeft < 0) timeLeft = 0;
            
            response["payload"]["round"] = {
                {"active", true},
                {"word", currentRound.word},
                {"hint", currentRound.hint},
                {"timeLeft", timeLeft}
            };
        }

        std::cout << "[DEBUG] About to send state with " << strokeHistory.size() << " strokes" << std::endl;
        s->send(response.dump());
        std::cout << "[STATE] Sent current state to client for room: " << roomId << std::endl;
    }
    else {
        std::cout << "[DEBUG] Room not found: " << roomId << std::endl;
    }
}

void RoomManager::cleanupAbandonedRooms() {
    std::lock_guard<std::mutex> lock(m_mutex);

    auto it = m_rooms.begin();
    while (it != m_rooms.end()) {
        if (it->second.empty()) {
            std::cout << "[ROOM] Cleaning up abandoned room: " << it->first << std::endl;
            it = m_rooms.erase(it);
        }
        else {
            ++it;
        }
    }
}

void RoomManager::cleanupExpiredRooms() {
    std::lock_guard<std::mutex> lock(m_mutex);
    
    auto now = std::chrono::steady_clock::now();
    auto oneHour = std::chrono::hours(1);
    
    auto it = m_rooms.begin();
    while (it != m_rooms.end()) {
        auto lastActivity = it->second.getLastActivity();
        if (now - lastActivity > oneHour) {
            std::cout << "[ROOM] Cleaning up expired room: " << it->first 
                      << " (inactive for " << std::chrono::duration_cast<std::chrono::minutes>(now - lastActivity).count() << " minutes)" << std::endl;
            
            // Remove from room channels tracking
            m_roomChannels.erase(it->first);
            
            it = m_rooms.erase(it);
        } else {
            ++it;
        }
    }
}


Room* RoomManager::getCurrentRoom(const std::string& channel) {
    std::lock_guard<std::mutex> lock(m_mutex);
    
    // Normalize channel name (remove # if present)
    std::string normalizedChannel = channel;
    if (!normalizedChannel.empty() && normalizedChannel[0] == '#') {
        normalizedChannel = normalizedChannel.substr(1);
    }
    
    // Find the room for this channel
    for (auto& pair : m_roomChannels) {
        if (pair.second == normalizedChannel) {
            auto it = m_rooms.find(pair.first);
            if (it != m_rooms.end()) {
                return &it->second;
            }
        }
    }
    return nullptr;
}

void RoomManager::onMessage(std::shared_ptr<Session> s, const std::string& jsonMsg) {
    try {
        auto j = json::parse(jsonMsg);
        std::string type = j.value("type", "");
        std::string roomId = normalizeRoom(j.value("room", ""));

        if (type == "join")        handleJoin(s, j, roomId);
        else if (type == "leave")  handleLeave(s, j, roomId);
        else if (type == "chat")   handleChat(s, j, roomId);
        else if (type == "start_round") handleStartRound(s, j, roomId);
        else if (type == "guess") handleGuess(s, j, roomId);
        else if (type == "end_round") handleEndRound(roomId);
        else if (type == "stop_bot")  handleStopBot(j);
        else if (type == "spawn_bot") handleSpawnBot(j);
        else if (type == "map_twitch_room") handleMapTwitchRoom(j);
        else if (type == "status")    handleStatus(jsonMsg);
        else if (type == "pong" && s) s->markPongReceived();
        else if (type == "draw")      handleDraw(s, j, roomId);
        else if (type == "clear")     handleClear(s, j, roomId);
        else if (type == "get_state") handleRestoreState(s, roomId);
        else {
            std::cerr << "[WARN] Unknown type: " << type << " msg=" << jsonMsg << "\n";
        }
    }
    catch (const std::exception& e) {
        std::cerr << "[ERROR] onMessage parse failed: " << e.what()
            << " raw=" << jsonMsg << "\n";
    }
}

void RoomManager::handleStartRound(std::shared_ptr<Session> s, const nlohmann::json& j, const std::string& roomId) {
    std::string word = j.value("payload", nlohmann::json::object()).value("word", "apple");
    m_rooms[roomId].startRound(word);
}

void RoomManager::handleGuess(std::shared_ptr<Session> s, const nlohmann::json& j, const std::string& roomId) {
    // Only allow guesses from Twitch chat, not direct WebSocket messages
    // This prevents cheating by sending direct WebSocket messages
    std::cout << "[SECURITY] Blocked direct guess attempt from WebSocket session" << std::endl;
    return;
}


