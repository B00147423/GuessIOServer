#pragma once
#include <unordered_map>
#include <unordered_set>
#include <memory>
#include <mutex>
#include <string>
#include <chrono>
#include <nlohmann/json.hpp>

// forward declare only
class Session;
struct Player {
    int id;
    std::string username;
    int score;
};

struct Round {
    std::string word;      // secret word
    std::string hint;      // underscores for viewers
    bool active = false;
    std::chrono::steady_clock::time_point startTime;
    int duration = 60;     // seconds
};


class Room {
public:
    Room(); // Constructor declaration only
    void join(std::shared_ptr<Session> s, const std::string& username);  // match .cpp
    bool leave(std::shared_ptr<Session> s);
    void broadcast(const std::string& msg);
    bool empty();
    void endRound();
    void endRoundInternal(); // Internal version without mutex lock
    void startRound(const std::string& word);
    void handleGuess(const std::string& username, const std::string& guess);
    void startServerTimer(); // Server-side timer
    void checkTimer(); // Check if round should end
    void resetLobby();
    bool hasPlayer(const std::string& username);
    const std::unordered_map<std::string, Player>& getPlayers() const { return players; }
    std::unordered_set<std::string> getPlayerUsernames() const;
    const Round& getCurrentRound() const { return currentRound; }
    void addStroke(const nlohmann::json& stroke);
    void clearHistory();
    void replayHistory(std::shared_ptr<Session> s);
    void replayPlayers(std::shared_ptr<Session> s); // NEW
    
    // NEW: Simple getters for persistence
    const std::vector<nlohmann::json>& getStrokeHistory() const { return strokeHistory; }

    // Activity tracking
    void updateActivity();
    std::chrono::steady_clock::time_point getLastActivity() const;


private:
    std::string m_roomName;
    std::unordered_set<std::shared_ptr<Session>> m_sessions;
    mutable std::mutex m_mutex;
    std::unordered_map<std::string, Player> players;
    int nextPlayerId = 1;

    // NEW: store all strokes for this room
    std::vector<nlohmann::json> strokeHistory;
    std::chrono::steady_clock::time_point m_lastActivity; // Track last activity
    Round currentRound;
};
