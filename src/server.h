#pragma once
#include <boost/asio.hpp>
#include <memory>
#include <unordered_set>
#include <mutex>
#include "session.h"
#include "roomManager.h"

// Forward declarations to avoid circular dependency
class TwitchBotManager; 

class Server {

public:
	Server(boost::asio::io_context& io, int port);
	void start();

	
	void addSession(std::shared_ptr<Session> session);
	void removeSession(std::shared_ptr<Session> session);
	void broadcast(std::string msg);
	void onClientMessage(std::shared_ptr<Session> s, const std::string& msg);
	void setBotManager(TwitchBotManager* botManager);
	bool spawnBot(const std::string& oauth,
		const std::string& nick,
		const std::string& channel);
	bool stopBot(const std::string& channel);
	void setCurrentRoom(const std::string& channel, const std::string& roomName); // Set current room for specific channel
	RoomManager& getRoomManager() { return m_roomManager; }
private:
	void doAccept();

	boost::asio::ip::tcp::acceptor m_acceptor;
	boost::asio::ip::tcp::socket m_acceptSocket;

	std::unordered_set<std::shared_ptr<Session>> m_sessions;
	std::mutex m_sessionsMutex;

	RoomManager m_roomManager;
	TwitchBotManager* m_botManager;
};