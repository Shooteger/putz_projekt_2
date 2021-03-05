//          Copyright Maurice Putz 2021.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE)

#include <iostream>
#include <asio.hpp>
#include "client.h"

using namespace std;

string Client::receive_data(asio::ip::tcp::socket& socket) {
  //only temporarily, here the sliding window algorithm will take place;
  asio::streambuf sbf;
  asio::read_until(socket, sbf, "\n");
  string data = asio::buffer_cast<const char*>(sbf.data());
  return data;
}

void Client::send_data(asio::ip::tcp::socket& socket, const string message) {
  write(socket, asio::buffer(message + "\n"));
}

Client::Client(asio::io_context& io_context, int port, string ip_adress) {
    asio::ip::tcp::socket socket(io_context);
    socket.connect(asio::ip::tcp::endpoint(asio::ip::address::from_string(ip_adress), port));
    send_data(socket, "tesxt");
    string response = receive_data(socket);
    cout << "[Client]" << response << "\n";
}

