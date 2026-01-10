#include "TwitchClient.h"
#include "server.h"
#include <iostream>
#include <nlohmann/json.hpp>

using json = nlohmann::json;

TwitchClient::TwitchClient(boost::asio::io_context& io,
    Server& server,
    const std::string& oauth,
    const std::string& nick,
    const std::string& channel)
    : m_resolver(io),
    m_socket(io),
    m_server(server),
    m_oauth(oauth),
    m_nick(nick),
    m_channel(channel),
    m_channelRooms() {
}

void TwitchClient::connect() {
    auto self = shared_from_this();
    auto endpoints = m_resolver.resolve("irc.chat.twitch.tv", "6667");
    boost::asio::async_connect(m_socket, endpoints,
        [self](boost::system::error_code ec, const auto&) {
            if (!ec) {
                self->login();
            }
            else {
                std::cerr << "Twitch connect error: " << ec.message() << "\n";
            }
        });
}

void TwitchClient::login() {
    auto self = shared_from_this();
    m_buffer.consume(m_buffer.size());

    send("CAP REQ :twitch.tv/tags twitch.tv/commands twitch.tv/membership\r\n");
    send("PASS " + m_oauth + "\r\n");
    send("NICK " + m_nick + "\r\n");
    send("JOIN " + m_channel + "\r\n");

    doRead();
}

void TwitchClient::send(const std::string& msg) {
    auto self = shared_from_this();
    auto buffer = std::make_shared<std::string>(msg);
    boost::asio::async_write(m_socket, boost::asio::buffer(*buffer),
        [self, buffer](boost::system::error_code ec, std::size_t) {
            if (ec) {
                std::cerr << "Twitch send error: " << ec.message() << "\n";
            }
        });
}

void TwitchClient::disconnect() {
    if (m_socket.is_open()) {
        std::string partCmd = "PART " + m_channel + "\r\n";
        boost::asio::write(m_socket, boost::asio::buffer(partCmd));

        std::string quitCmd = "QUIT\r\n";
        boost::asio::write(m_socket, boost::asio::buffer(quitCmd));

        boost::system::error_code ec;
        m_socket.close(ec);

        if (!ec) {
            std::cout << "[INFO] Disconnected from channel " << m_channel << "\n";
        }
        else {
            std::cout << "[ERROR] Failed to close socket for " << m_channel
                << ": " << ec.message() << "\n";
        }
    }
}

void TwitchClient::doRead() {
    auto self = shared_from_this();
    boost::asio::async_read_until(m_socket, m_buffer, "\r\n",
        [self](boost::system::error_code ec, std::size_t) {
            if (!ec) {
                std::istream is(&self->m_buffer);
                std::string line;

                while (std::getline(is, line)) {
                    if (!line.empty() && line.back() == '\r')
                        line.pop_back();
                    if (line.empty()) continue;

                    std::cout << "[RAW] " << line << "\n";

                    // PING/PONG
                    if (line.rfind("PING", 0) == 0) {
                        self->send("PONG :tmi.twitch.tv\r\n");
                        continue;
                    }

                    // Connected
                    if (line.find(" 001 ") != std::string::npos) {
                        json okMsg = {
                            {"type","status"},
                            {"status","ok"},
                            {"message","Bot connected to Twitch IRC"},
                            {"channel", self->m_channel}
                        };
                        self->m_server.onClientMessage(nullptr, okMsg.dump());
                        continue;
                    }

                    // PRIVMSG (chat)
                    if (line.find("PRIVMSG") != std::string::npos) {
                        // Extract username
                        std::string username;
                        if (line[0] == '@') {
                            size_t dnPos = line.find("display-name=");
                            if (dnPos != std::string::npos) {
                                size_t end = line.find(';', dnPos);
                                if (end == std::string::npos) end = line.find(' ', dnPos);
                                if (end != std::string::npos)
                                    username = line.substr(dnPos + 13, end - (dnPos + 13));
                            }
                            if (username.empty()) {
                                size_t loginPos = line.find("login=");
                                if (loginPos != std::string::npos) {
                                    size_t end = line.find(';', loginPos);
                                    if (end == std::string::npos) end = line.find(' ', loginPos);
                                    if (end != std::string::npos)
                                        username = line.substr(loginPos + 6, end - (loginPos + 6));
                                }
                            }
                        }
                        if (username.empty()) {
                            size_t exMark = line.find('!');
                            if (exMark != std::string::npos && exMark > 1)
                                username = line.substr(1, exMark - 1);
                        }

                        // Extract message
                        std::string message;
                        size_t lastColon = line.rfind(':');
                        if (lastColon != std::string::npos)
                            message = line.substr(lastColon + 1);

                        std::cout << "[CHAT] " << username << ": " << message << "\n";

                        // Forward into GameProtocol
                        if (self->gameProtocol_) {
                            self->gameProtocol_->handleCommand(username, message, self->m_channel);
                        }
                    }
                }

                self->doRead();
            }
            else {
                std::cerr << "Twitch read error: " << ec.message() << "\n";
            }
        });
}

void TwitchClient::setCurrentRoom(const std::string& channel, const std::string& roomName) {
    m_channelRooms[channel] = roomName;
}
