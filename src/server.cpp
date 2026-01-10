#include "server.h"
#include "session.h"
#include "TwitchBotManager.h"
#include <iostream>

Server::Server(boost::asio::io_context& io, int port)
    : m_acceptor(io, boost::asio::ip::tcp::endpoint(boost::asio::ip::tcp::v4(), port)),
    m_acceptSocket(io),
    m_roomManager(),
    m_botManager(nullptr) {
    m_roomManager.setServer(this);
}

void Server::setBotManager(TwitchBotManager* botManager) {
    m_botManager = botManager;
}

bool Server::spawnBot(const std::string& oauth,
    const std::string& nick,
    const std::string& channel) {
    if (m_botManager) {
        return m_botManager->spawnBot(oauth, nick, channel);
    }
    return false;
}
bool Server::stopBot(const std::string& channel) {
    if (m_botManager) {
        m_botManager->stopBot(channel);
        return true;
    }
    return false;
}

void Server::setCurrentRoom(const std::string& channel, const std::string& roomName) {
    if (m_botManager) {
        m_botManager->setCurrentRoom(channel, roomName);
    }
}
void Server::start() {
    doAccept();
}

void Server::doAccept() {
    m_acceptor.async_accept(m_acceptSocket,
        [this](boost::system::error_code ec) {
            if (!ec) {
                auto session = std::make_shared<Session>(std::move(m_acceptSocket), *this);
                addSession(session);
                session->start();
            }
            doAccept();
        });
}


void Server::addSession(std::shared_ptr<Session> session) {
    // TODO: lock + insert into sessions_
    std::lock_guard<std::mutex> lock(m_sessionsMutex);
    m_sessions.insert(session);
    
}

void Server::removeSession(std::shared_ptr<Session> session) {
    std::lock_guard<std::mutex> lock(m_sessionsMutex);
    if (m_sessions.find(session) != m_sessions.end()) {
        m_sessions.erase(session);
    }
}

void Server::broadcast(std::string msg) {
    std::lock_guard<std::mutex> lock(m_sessionsMutex);

    for (auto& s : m_sessions) {
        s->send(msg);
    }
}

void Server::onClientMessage(std::shared_ptr<Session> s, const std::string& msg) {
    if (!s) {
        // Message came from Twitch: inject directly into RoomManager
        m_roomManager.onMessage(nullptr, msg);
        return;
    }
    m_roomManager.onMessage(s, msg);
}
