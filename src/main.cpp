#include "server.h"
#include "TwitchBotManager.h"
#include "GameProtocol.h"
#include "roomManager.h"
#include <boost/asio.hpp>
#include <thread>
#include <vector>
#include <iostream>
#include <csignal>
#include <atomic>
#include <fstream>
#include "libs/json.hpp"
#include <cstdlib>
#include <grpcpp/grpcpp.h>
#include "grpc_server.h"

// global running flag
std::atomic<bool> running(true);

// load JSON config
nlohmann::json loadConfig(const std::string& path) {
    std::ifstream f(path);
    if (!f.is_open()) {
        throw std::runtime_error("Could not open config file: " + path);
    }
    nlohmann::json j;
    f >> j;
    return j;
}

// get environment variable with fallback
std::string getEnvVar(const std::string& key, const std::string& defaultValue = "") {
    char* val = nullptr;
    size_t len = 0;
    errno_t err = _dupenv_s(&val, &len, key.c_str());
    
    if (err == 0 && val != nullptr) {
        std::string result(val);
        free(val); // _dupenv_s allocates memory that needs to be freed
        return result;
    }
    
    if (val != nullptr) {
        free(val);
    }
    return defaultValue;
}

// signal handler
void handleSignal(int) {
    running = false;
}

int main() {
    try {
        std::cout << "Starting server...\n";
        signal(SIGINT, handleSignal);
        std::signal(SIGTERM, handleSignal);

        std::cout << "Creating io_context...\n";
        boost::asio::io_context io;

        std::cout << "Creating server...\n";
        ::Server server(io, 9001);

        std::cout << "Creating TwitchBotManager...\n";
        TwitchBotManager botManager(io, server);

        std::cout << "Setting bot manager...\n";
        server.setBotManager(&botManager);

        // Create a GameProtocol and set it on the bot manager
        std::cout << "Creating GameProtocol...\n";
        auto gameProtocol = std::make_shared<GameProtocol>(&server);
        botManager.setGameProtocol(gameProtocol);


        std::cout << "Starting server...\n";
        server.start();
        std::cout << "Server started successfully on port 9001\n";

        // load secrets from environment variables first, then config.json as fallback
        std::string oauth = getEnvVar("TWITCH_OAUTH");
        std::string nick = getEnvVar("TWITCH_NICK");
        std::string channel = getEnvVar("TWITCH_CHANNEL");
        
        // If environment variables are not set, try to load from config.json
        if (oauth.empty() || nick.empty() || channel.empty()) {
            try {
                auto cfg = loadConfig("config.json");
                if (oauth.empty()) oauth = cfg.value("TWITCH_OAUTH", "");
                if (nick.empty()) nick = cfg.value("TWITCH_NICK", "");
                if (channel.empty()) channel = cfg.value("TWITCH_CHANNEL", "");
            } catch (const std::exception& e) {
                std::cout << "Warning: Could not load config.json, using environment variables only\n";
            }
        }

        // spawn bot
        std::cout << "Spawning Twitch bot for channel " << channel << "...\n";
        bool botSpawned = server.spawnBot(oauth, nick, channel);

        if (botSpawned) {
            std::cout << "Twitch bot spawned successfully!\n";
        }
        else {
            std::cout << "Failed to spawn Twitch bot!\n";
        }


        // Start gRPC server in a separate thread
        std::thread grpcThread(StartGrpcServer);
        grpcThread.detach();
        std::cout << "[gRPC] Server thread launched.\n";
        // thread pool
        unsigned int numThreads = std::thread::hardware_concurrency();
        if (numThreads == 0) numThreads = 4;
        std::vector<std::thread> pool;
        pool.reserve(numThreads);
        for (unsigned int i = 0; i < numThreads; ++i)
            pool.emplace_back([&io]() { io.run(); });

        // main loop
        while (running) {
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }

        server.broadcast(R"({"type":"system","payload":"server shutting down"})");


        io.stop();
        for (auto& t : pool) t.join();
    }
    catch (const std::exception& e) {
        std::cerr << "Fatal error: " << e.what() << "\n";
    }
}
