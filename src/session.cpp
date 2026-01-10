#include "session.h"
#include "server.h"
#include <iostream>

Session::Session(boost::asio::ip::tcp::socket socket, Server& server)
    : m_ws(std::move(socket)),
    m_pingTimer(m_ws.get_executor()),
    m_server(server) {
}


void Session::start() {
    auto self = shared_from_this();
    m_ws.async_accept([this, self](boost::system::error_code ec) {
        if (ec) {
            std::cerr << "Handshake failed: " << ec.message() << "\n";
            m_server.removeSession(self);
            return;
        }
        std::cout << "Handshake complete!\n";
        
        // Set up pong handler before starting ping
        m_ws.control_callback([this, self](boost::beast::websocket::frame_type kind, boost::string_view payload) {
            if (kind == boost::beast::websocket::frame_type::pong) {
                markPongReceived();
            }
        });
        
        startPing();
        doRead();
    });
}

void Session::doRead() {
    auto self = shared_from_this();
    m_ws.async_read(m_buffer, [this, self](boost::system::error_code ec, std::size_t bytes) {
        if (ec) {
            std::cerr << "Read error: " << ec.message() << "\n";
            m_server.removeSession(self);
            return;
        }
        auto data = static_cast<const char*>(m_buffer.data().data());
        std::string msg(data, bytes);
        m_buffer.consume(bytes);
        handleMessage(msg);
        doRead();
        });
}

void Session::handleMessage(const std::string& msg) {
    std::cout << "Handling message: " << msg << "\n";

    m_server.onClientMessage(shared_from_this(), msg);
}

void Session::send(const std::string& msg) {
    auto self = shared_from_this();
    {
        std::lock_guard<std::mutex> lock(m_writeMutex);
        m_writeQueue.push_back(msg);
        if (m_writing) return;
        m_writing = true;
    }
    doWrite();
}

void Session::doWrite() {
    auto self = shared_from_this();
    std::string& msg = m_writeQueue.front();

    m_ws.async_write(boost::asio::buffer(msg), [this, self](boost::system::error_code ec, std::size_t) {
        std::lock_guard<std::mutex> lock(m_writeMutex);
        if (ec) {
            std::cerr << "Send error: " << ec.message() << "\n";
            m_server.removeSession(self);
            return;
        }
        m_writeQueue.pop_front();
        if (!m_writeQueue.empty())
            doWrite();
        else
            m_writing = false;
        });
}

void Session::close() {
    auto self = shared_from_this();
    m_ws.async_close(boost::beast::websocket::close_code::normal, [this, self](boost::system::error_code ec) {
        if (ec)
            std::cerr << "Close error: " << ec.message() << "\n";
        m_server.removeSession(self);
        });
}

void Session::startPing() {
    auto self = shared_from_this();
    m_pingTimer.expires_after(std::chrono::seconds(30));
    m_pingTimer.async_wait([this, self](boost::system::error_code ec) {
        if (!ec) {
            if (!m_pongReceived) {
                std::cerr << "[WARN] Heartbeat timeout\n";
                close();
                return;
            }
            m_pongReceived = false;

            // Send ping only if connection is still open
            if (m_ws.is_open()) {
                m_ws.async_ping({}, [this, self](boost::system::error_code ec) {
                    if (ec) {
                        std::cerr << "Ping error: " << ec.message() << "\n";
                        close();
                        return;
                    }
                    startPing(); // schedule next ping
                });
            }
        }
        });
}


void Session::markPongReceived() {
    m_pongReceived = true;
}