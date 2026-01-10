#pragma once
#include <boost/asio.hpp>
#include <boost/beast/core.hpp>
#include <boost/beast/websocket.hpp>
#include <deque>
#include <mutex>
#include <string>
#include <memory>
#include "session.h"
#include "server.h"
#include <iostream>

class Server; // forward declaration

class Session : public std::enable_shared_from_this<Session> {
public:
    Session(boost::asio::ip::tcp::socket socket, Server& server);

    void start();
    void send(const std::string& msg);
    void close();
    void startPing();
    void markPongReceived();

private:
    void doRead();
    void doWrite();
    void handleMessage(const std::string& msg);


    boost::beast::websocket::stream<boost::asio::ip::tcp::socket> m_ws;
    boost::beast::flat_buffer m_buffer;

    std::deque<std::string> m_writeQueue;
    bool m_writing = false;
    std::mutex m_writeMutex;

    boost::asio::steady_timer m_pingTimer;
    bool m_pongReceived = true;

    Server& m_server;
};