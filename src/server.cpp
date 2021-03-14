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

int main() {
  Server server{};
  
  asio::io_context context;
  
  asio::error_code ec;
  asio::ip::tcp::acceptor server_acceptor(context, 
                                          asio::ip::tcp::endpoint(asio::ip::make_address("127.0.0.1", ec), 9999));
  if (!ec) {
    cout << "[Server] Client connected\n";
    asio::ip::tcp::socket socket(context);
    server_acceptor.accept(socket); //wait for input

    string number_of_sended_frames_tmp;
    string window_size_tmp;
    while(true) {
      server_acceptor.listen(1);
      //gets size if windows
      try {
          window_size_tmp = server.receive_data(socket);
          window_size_tmp.pop_back();
          server.send_data(socket, "[SERVER]WS_ACN");
      } catch (std::system_error const& ex) {
          cout << "[SERVER] Window size not valid\n"; //later logging here
          break;
      }
      
      //gets maximum of sent frames
      try {
          number_of_sended_frames_tmp = server.receive_data(socket);
          number_of_sended_frames_tmp.pop_back();
          server.send_data(socket, "[SERVER]F_ACN");
      } catch (std::system_error const& ex) {
          cout << "[SERVER] Number of receiving frames not valid\n"; //later logging here
          break;
      }

      int window_size = stoi(window_size_tmp);
      int number_of_sended_frames = stoi(number_of_sended_frames_tmp);

      int window_cnt = 1;
      int cnt = 0;
      while (cnt < number_of_sended_frames) {
        try {
          string res = server.receive_data(socket);
          res.pop_back();

          if (res == "exit") {
            cout << "[Server] Client disconneted\n";
            break;
          }

          ++window_cnt;
          if (window_cnt == window_size) {
            cout << "[Server] All frames received\n";
            server.send_data(socket, res);
            window_cnt = 1;
          }
        } catch (std::system_error const& ex) {
          //should be raised after last char receiving
          break;
        }
        cnt++;
      }

      server_acceptor.cancel(); //end client connection
      break;
    }
  } else {
    cout << "Connection failed to server with address:\n" << ec.message() << "\n";
  }

  return 0;
}