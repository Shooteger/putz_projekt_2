//          Copyright Maurice Putz 2021.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE)

#include <iostream>
#include <stdio.h>
#include <stdlib.h>

#include <CLI11.hpp>
#define ASIO_STANDALONE
#include <asio.hpp>
#include "spdlog/sinks/basic_file_sink.h"
#include "spdlog/async.h"
#include "rang.hpp"

//ignore warning "-Wnon-virtual-dtor" from extern library "tabulate"
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wnon-virtual-dtor"
#include "tabulate.hpp"
#pragma GCC diagnostic pop

#include "server.h"

using namespace std;
using namespace tabulate;

//create logger for file
string home = getenv("HOME");
string logPath = home;
auto logger = spdlog::basic_logger_mt<spdlog::async_factory>("async_file_logger", logPath.append("/Desktop/connectsim/log.txt"));

//returns vector of ascii character
vector<char> create_random_ascii(string allowed_ascii_signs="") {
    vector<char> res;
    srand((int)time(0)); //starting point and time point of random seed
	int repeat = rand() % 127  + 1;   //how often random ascii sign should be repeated

    if (allowed_ascii_signs == "") {
        int i = 0;
        while(i++ < repeat) {
            res.push_back(static_cast<char>((rand() % (126-33)) + 33)); //all alphanumerical and displayable special signs of ascii are between 33 and 126
        }
    } else {
        vector<int> tmp_chars;
        for (size_t i=0; i < allowed_ascii_signs.length(); ++i) {
            tmp_chars.push_back(allowed_ascii_signs.at(i)); 
        }

        int j = 0;
        while(j++ < repeat) {
            int random_idx = rand() % tmp_chars.size();
            res.push_back(static_cast<char>(tmp_chars[random_idx]));
        }
    }
    return res;
}

string receive_data(asio::ip::tcp::socket& socket) {
  //only temporarily, here the sliding window algorithm will take place;
  asio::streambuf sbf;
  asio::read_until(socket, sbf, "\n");
  string data = asio::buffer_cast<const char*>(sbf.data());
  return data;
}

void send_data(asio::ip::tcp::socket& socket, const string message) {
  write(socket, asio::buffer(message + "\n"));
}


int main(int argc, char* argv[]) {
    string input_chars;
    string window_size = "3";
    bool a = false;
    
    CLI::App app {"Networking Simulator"};
    app.add_option("input_characters", input_chars,
         "Given characters will be random times send to server    Example: \"./connectsim asdf\"");
    app.add_option("-w,--windowsize", window_size,
         "Given number will be the size of the window of sliding window algorithm used for data transmission    Example: \"./connectsim 3\"");
    app.add_flag("-a,--allowed", a , "Show allowed character for input");

    //NOTE ADD WHICH ASCII CHARACTERS ARE ALLOWED! 33 until 129 in dec!
    cout << rang::fg::cyan;
    try {
        CLI11_PARSE(app, argc, argv);
    } catch(const CLI::ParseError &e) {
        logger->error("Program terminated because of parse exception: {0}", e.what());
        return app.exit(e);
    }

    if (!a) {
        const vector<char> ascii_vec = create_random_ascii(input_chars);
        asio::error_code ec;
        asio::io_context context;
        asio::ip::tcp::socket socket(context);
        socket.connect(asio::ip::tcp::endpoint(asio::ip::address::from_string("127.0.0.1", ec), 9999));

        if (!ec) {
            string response;
            string tmp;
            
            send_data(socket, window_size);
            response = receive_data(socket);
            response.pop_back();    //here is stucked an hour, freaking \n remove!!!
            cout << response;

            if (response == "[SERVER]WS_ACN") { //check if window size acn
                
                send_data(socket, to_string(ascii_vec.size()));
                response = receive_data(socket);
                response.pop_back();

                if (response == "[SERVER]F_ACN") { //check if frames count acn
                    for (size_t i=0; i < ascii_vec.size(); ++i) {
                        tmp = "";
                        tmp.push_back(ascii_vec.at(i));
                        send_data(socket, tmp);
                        response = receive_data(socket);
                        cout << "[Client] " << response;
                    
                    }
                    socket.close(ec);
                    cout << "[Client] Disconnected!\n";
                } else {
                    socket.close();
                    cout << rang::fg::red;
                    throw std::invalid_argument("[Client] Server responded with wrong ACN for number of frames\nFor security measures connection is closed.");
                    cout << rang::style::reset;
                }
            } else {
                socket.close();
                cout << rang::fg::red;
                throw std::invalid_argument("[Client] Server responded with wrong ACN for window size\nFor security measures connection is closed.");
                cout << rang::style::reset;
            }
        } else {
            cout << rang::fg::red;
            cout << "[Client] Connection was not closed as intended:\n" << ec.message();
            cout << rang::style::reset;
        }

    } else {
        cout << rang::fg::magenta << "\n\n Allowed characters are: \n\n" << rang::style::reset;
        Table ascii_table;
        ascii_table.add_row({"Character", "ASCII Value"});
        int cnt = 33;
        string tmp;
        while (cnt < 127) {
            tmp = char(cnt);
            ascii_table.add_row({tmp, to_string(cnt)});
            cnt++;
        }
        cout << ascii_table << "\n";
    }
}
