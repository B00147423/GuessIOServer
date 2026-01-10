#include "room.h"
#include "session.h"   // full definition of Session
#include <iostream>
#include <unordered_map>
#include <chrono>
#include <thread>
using json = nlohmann::json;

Room::Room() : nextPlayerId(1), m_lastActivity(std::chrono::steady_clock::now()) {}

void Room::updateActivity() {
    m_lastActivity = std::chrono::steady_clock::now();
}

std::chrono::steady_clock::time_point Room::getLastActivity() const {
    return m_lastActivity;
}

// Default constructor is now defined in header

void Room::join(std::shared_ptr<Session> s, const std::string& username) {
    nlohmann::json joinMsg;
    bool isNewPlayer = false;

    {
        std::lock_guard<std::mutex> lock(m_mutex);

        if (players.find(username) == players.end()) {
            Player p{ nextPlayerId++, username, 0 };
            players[username] = p;
            isNewPlayer = true;
            std::cout << "[DEBUG] Room::join - Adding new player: " << username << " with ID: " << p.id << std::endl;

            joinMsg = {
                {"type", "join"},
                {"payload", {
                    {"id", p.id},
                    {"username", p.username}
                }}
            };
        } else {
            std::cout << "[DEBUG] Room::join - Player " << username << " already exists" << std::endl;
        }

        if (s) {
            m_sessions.insert(s);
        }
        
        // Update activity timestamp
        updateActivity();
    }

    // Broadcast outside of mutex lock to avoid deadlock
    if (isNewPlayer) {
        std::cout << "[DEBUG] Room::join - Broadcasting join message for: " << username << std::endl;
        broadcast(joinMsg.dump());
    }

    if (s) {
        replayPlayers(s);
        replayHistory(s);
    }
}


bool Room::leave(std::shared_ptr<Session> s) {
    std::lock_guard<std::mutex> lock(m_mutex);

    // just remove the session
    auto it = m_sessions.find(s);
    if (it != m_sessions.end()) {
        m_sessions.erase(it);
    }

    return m_sessions.empty();
}

void Room::resetLobby() {
    std::lock_guard<std::mutex> lock(m_mutex);
    players.clear();
    nextPlayerId = 1;
}

void Room::broadcast(const std::string& msg) {
    for (auto& s : m_sessions) {
        if (s) s->send(msg);
    }
}

bool Room::empty() {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_sessions.empty() && players.empty();
}

void Room::startRound(const std::string& word) {
    std::lock_guard<std::mutex> lock(m_mutex);

    currentRound.word = word;
    currentRound.hint = std::string(word.size(), '_');
    currentRound.startTime = std::chrono::steady_clock::now();
    currentRound.active = true;

    nlohmann::json msg = {
        {"type", "round_start"},
        {"payload", {
            {"word", currentRound.word},
            {"hint", currentRound.hint},
            {"time", currentRound.duration}
        }}
    };
    broadcast(msg.dump());
    
    // Start server-side timer
    startServerTimer();
}

void Room::handleGuess(const std::string& username, const std::string& guess) {
    std::lock_guard<std::mutex> lock(m_mutex);

    if (!currentRound.active) {
        std::cout << "[ROOM] Guess from " << username << " ignored - no active round" << std::endl;
        return;
    }
    
    std::cout << "[ROOM] Processing guess from " << username << ": " << guess << " (correct word: " << currentRound.word << ")" << std::endl;

    if (guess == currentRound.word) {
        // award points
        if (players.find(username) != players.end()) {
            players[username].score += 100; // basic scoring
        }

        nlohmann::json correctMsg = {
            {"type", "guess"},
            {"payload", {
                {"user", username},
                {"word", guess},
                {"correct", true},
                {"score", players.find(username) != players.end() ? players[username].score : 0}
            }}
        };
        broadcast(correctMsg.dump());

        // end round - call endRoundInternal to avoid deadlock
        endRoundInternal();
    }
    else {
        nlohmann::json wrongMsg = {
            {"type", "guess"},
            {"payload", {
                {"user", username},
                {"word", guess},
                {"correct", false}
            }}
        };
        broadcast(wrongMsg.dump());
    }
}

void Room::endRound() {
    std::lock_guard<std::mutex> lock(m_mutex);
    endRoundInternal();
}

void Room::endRoundInternal() {
    if (!currentRound.active) return;
    currentRound.active = false;

    nlohmann::json endMsg = {
        {"type","round_end"},
        {"payload", {
            {"word", currentRound.word},
            {"scores", nlohmann::json::object()}
        }}
    };

    for (auto& [username, p] : players) {
        endMsg["payload"]["scores"][username] = p.score;
    }

    broadcast(endMsg.dump());
}

void Room::startServerTimer() {
    // Start a thread to check timer every second
    std::thread([this]() {
        for (int i = 0; i < currentRound.duration; ++i) {
            std::this_thread::sleep_for(std::chrono::seconds(1));
            
            // Check if round is still active
            {
                std::lock_guard<std::mutex> lock(m_mutex);
                if (!currentRound.active) {
                    return; // Round ended early (correct guess)
                }
            }
        }
        
        // Timer expired - end the round
        std::cout << "[ROOM] Server timer expired - ending round" << std::endl;
        endRound();
    }).detach();
}

void Room::checkTimer() {
    std::lock_guard<std::mutex> lock(m_mutex);
    
    if (!currentRound.active) return;
    
    auto now = std::chrono::steady_clock::now();
    auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(now - currentRound.startTime);
    
    if (elapsed.count() >= currentRound.duration) {
        std::cout << "[ROOM] Timer check - ending round" << std::endl;
        endRoundInternal();
    }
}

bool Room::hasPlayer(const std::string& username) {
    std::lock_guard<std::mutex> lock(m_mutex);
    return players.find(username) != players.end();
}

// room.cpp
void Room::addStroke(const json& stroke) {
    std::lock_guard<std::mutex> lock(m_mutex);
    strokeHistory.push_back(stroke);
    updateActivity();
}

void Room::clearHistory() {
    std::lock_guard<std::mutex> lock(m_mutex);
    strokeHistory.clear();
}

void Room::replayHistory(std::shared_ptr<Session> s) {
    std::vector<json> strokesCopy;

    {
        std::lock_guard<std::mutex> lock(m_mutex);
        strokesCopy = strokeHistory; // Copy the strokes
    }

    // Send strokes outside of mutex lock
    for (auto& stroke : strokesCopy) {
        std::cout << "[DEBUG] Replaying stroke to " << (s ? "session" : "null") << "\n";
        if (s) s->send(stroke.dump());
    }
}

void Room::replayPlayers(std::shared_ptr<Session> s) {
    std::vector<std::pair<int, std::string>> playerData;

    {
        std::lock_guard<std::mutex> lock(m_mutex);
        for (auto& [username, p] : players) {
            playerData.push_back({ p.id, p.username });
        }
    }

    // Send player data outside of mutex lock
    for (auto& [id, username] : playerData) {
        nlohmann::json joinMsg = {
            {"type", "join"},
            {"payload", {
                {"id", id},
                {"username", username}
            }}
        };
        if (s) s->send(joinMsg.dump());
    }
}

std::unordered_set<std::string> Room::getPlayerUsernames() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    std::unordered_set<std::string> usernames;
    for (const auto& [username, p] : players) {
        usernames.insert(username);
    }
    return usernames;
}