//          Copyright Maurice Putz 2021.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file boost_license.txt)

//#pragma GCC diagnostic push
//#pragma GCC diagnostic ignored "-Wnon-virtual-dtor"
//#pragma GCC diagnostic pop
#include <iostream>
#include <stdio.h>
#include <stdlib.h>

#include <CLI11.hpp>
#define ASIO_STANDALONE
#include <asio.hpp>
#include "spdlog/sinks/basic_file_sink.h"
#include "spdlog/async.h"

#include "server.h"
#include "client.h"

using namespace std;

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

int main() {
    const string hostname = "127.0.0.1";
    const int port = 8477; 
    /*

    asio::error_code ec;

    asio::io_context context;

    asio::ip::tcp::endpoint endpoint(asio::ip::make_address("127.0.0.1", ec), 8888); //gehört zu server

    asio::ip::tcp::socket socket(context);
    */
   
    if (!ec) {
        cout << "Connected...\n";
        vector<char> test = create_random_ascii("mpFAwsds6");
    
        for (size_t i=0; i < test.size(); ++i) {
            cout << test.at(i) << "\n";
        }
    } else {
        cout << "Connection failed to address:\n" << ec.message() << "\n";
    }
    
}
