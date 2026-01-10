#pragma once
#include <boost/asio.hpp>
#include <memory>
#include <string>
#include "server.h"
#include <unordered_map>
#include "GameProtocol.h"   // NEW include
class TwitchClient : public std::enable_shared_from_this<TwitchClient> {
public:
    TwitchClient(boost::asio::io_context& io,
        Server& server,
        const std::string& oauth,
        const std::string& nick,
        const std::string& channel);

    void connect();
    void disconnect();
    void setCurrentRoom(const std::string& channel, const std::string& roomName);
    void setGameProtocol(std::shared_ptr<GameProtocol> gp) { gameProtocol_ = gp; }

private:
    void login();
    void doRead();
    void send(const std::string& msg);

    boost::asio::ip::tcp::resolver m_resolver;
    boost::asio::ip::tcp::socket m_socket;
    boost::asio::streambuf m_buffer;

    Server& m_server;
    std::string m_oauth;
    std::string m_nick;
    std::string m_channel;
    std::unordered_map<std::string, std::string> m_channelRooms; // Track current room per channel

    std::shared_ptr<GameProtocol> gameProtocol_;
};
