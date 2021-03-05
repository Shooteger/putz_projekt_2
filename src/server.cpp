//          Copyright Maurice Putz 2021.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE)

#include <iostream>
#include <asio.hpp>
#include "server.h"

using namespace std;

string Server::receive_data(asio::ip::tcp::socket& socket) {
  //only temporarily, here the sliding window algorithm will take place;
  asio::streambuf sbf;
  asio::read_until(socket, sbf, "\n");
  string data = asio::buffer_cast<const char*>(sbf.data());
  return data;
}

void Server::send_data(asio::ip::tcp::socket& socket, const string message) {
  write(socket, asio::buffer(message + "\n"));
}

Server::Server(asio::io_context& io_context, int port, string ip_adress) {
  asio::error_code ec;
  asio::ip::tcp::acceptor server_acceptor(io_context, 
                                          asio::ip::tcp::endpoint(asio::ip::make_address(ip_adress, ec), port));

  if (!ec) {
    cout << "[Server] Client Connected\n";
    asio::ip::tcp::socket socket(io_context);
    server_acceptor.accept(socket); //wait for input

    while (true) {
      string res = receive_data(socket);//TEMP implementation here, later sliding window
      res.pop_back();

      if (res == "exit") {
        cout << "[Server] Disconneted\n";
        break;
      }
      cout << res << "\n";
      send_data(socket, res);
    }

  } else {
    cout << "Connection failed to server with address:\n" << ec.message() << "\n";
  }
}

int main() {
  asio::io_context context;
  Server server;
  server = Server(context, 9999, "127.0.0.1");
}