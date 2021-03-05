//          Copyright Maurice Putz 2021.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE)

#pragma once

#include <iostream>
#include <asio.hpp>

class Client {
  public:
    std::string receive_data(asio::ip::tcp::socket& socket);
    void send_data(asio::ip::tcp::socket& socket, const std::string message);
    Client(asio::io_context& io_context, int port, std::string ip_adress);
    Client(){};
  private:
    asio::io_context io_context;
    int port;
    std::string ip_address;
};