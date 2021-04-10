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
#include "rang.hpp"

//ignore warning "-Wnon-virtual-dtor" from extern library "tabulate"
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wnon-virtual-dtor"
#include "tabulate.hpp"
#pragma GCC diagnostic pop

using namespace std;
using namespace tabulate;

//returns vector of ascii character
vector<char> create_random_ascii(string allowed_ascii_signs="") {
    vector<char> res;
    srand((int)time(0)); //starting point and time point of random seed
	int repeat = rand() % 127  + 2;   //how often random ascii sign should be repeated

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
    asio::write(socket, asio::buffer(message + "\n"));
}

int max_sum(vector<char> ascii_vec, int window_size, int size) {
    if (size >= window_size) {
        int max_sum = 0;
        for (int i = 0; i < window_size; ++i) {
            max_sum += (int)ascii_vec.at(i);
        }
        
        int window_sum = max_sum;
        for (int i = window_size; i < size; ++i) {
            window_sum += (int)ascii_vec.at(i) - (int)ascii_vec.at(i - window_size);
            max_sum = max(max_sum, window_sum);
        }
        
        return max_sum;
    } else {
        return -1;
    }
}

int main(int argc, char* argv[]) {
    string input_chars;
    string window_size = "1";       //hier aufpassen!!!! window size muss kleiner als anzahl zufällig gewählter zu übertragenden zeichen sein
    string logpath_str = "connectsim_log.txt";
    
    std::shared_ptr<spdlog::logger> logger;

    bool a = false;
    bool l = false;
    bool pl = false;
    bool dm = false;
    bool pr = false;

    //setting logger path
    
    CLI::App app {"Networking Simulator"};
    app.add_option("input_characters", input_chars,
         "Given characters will be random times send to server    Example: \"./connectsim asdf\"");
    app.add_option("-w,--windowsize", window_size,
         "Given number will be the size of the window of sliding window algorithm used for data transmission    Example: \"./connectsim 3\"")->check(CLI::PositiveNumber);
    app.add_option("-s,--set_logpath", logpath_str,
         "Given Path will set new Path for saving logfile    Example: \"./connectsim -s /home/user/Desktop/");
    app.add_flag("-p,--packageloss", pl , "Simulates Package loss while sending data to server and otherwise");
    app.add_flag("-d,--datamanipulation", dm , "Simulates manipulation of some data packages.");
    app.add_flag("-r,--packagerow", pr , "Simulates behaviour if sended data have wrong order.");
    app.add_flag("-a,--allowed", a , "Show allowed character for input");
    app.add_flag("-l,--logpath", l , "Returns path of logfile with details on console");

    //NOTE ADD WHICH ASCII CHARACTERS ARE ALLOWED! 33 until 129 in dec!
    cout << rang::fg::cyan;
    try {
        CLI11_PARSE(app, argc, argv);
    } catch(const CLI::ParseError &e) {
        cout << rang::fg::red;
        return app.exit(e);
    }
    cout << rang::style::reset;

    try {
        logger = spdlog::basic_logger_mt("basic_logger", logpath_str);
    } catch (const spdlog::spdlog_ex &ex) {
        cout << "Initializing logpath failed.\nTry to change the path where the file should be saved to and try again.\nCommand with \"./connectsim -h\"\n";
    }

    if (!a && !l) {
        const vector<char> ascii_vec = create_random_ascii(input_chars);
        size_t tmp_compare = stoi(window_size);

        if (ascii_vec.size() >= tmp_compare) {

            asio::error_code ec;
            asio::io_context context;
            asio::ip::tcp::socket socket(context);
            socket.connect(asio::ip::tcp::endpoint(asio::ip::address::from_string("127.0.0.1", ec), 9999));

            if (!ec) {
                logger->info("[Client] Client connected to server");
                string response;
                string tmp;
                
                send_data(socket, window_size);
                response = receive_data(socket);
                response.pop_back();    //here is stucked an hour, freaking \n remove!!!

                if (response == "[SERVER]WS_ACN") { //check if window size acn
                    
                    send_data(socket, to_string(ascii_vec.size()));
                    response = receive_data(socket);
                    response.pop_back();

                    if (response == "[SERVER]F_ACN") { //check if frames count acn
                        int w_cnt = 0; //window size counter

                        int checksum = 0;

                        if (pl || dm || pr) {
                           cout << "test\n";
                        }

                        int max_checksum = max_sum(ascii_vec, stoi(window_size), (int)ascii_vec.size());
                        
                        cout << (int)ascii_vec.size() << "\n";

                        for (size_t i=0; i < ascii_vec.size(); ++i) {
                            //cout << (int)ascii_vec.at(i) << "\n";

                            checksum += (int)ascii_vec.at(i);

                            send_data(socket, to_string(ascii_vec.at(i)));
                            ++w_cnt;

                            if (pl) {
                                ++i;
                                pl = false;
                            }

                            //should enter after sending maximum window size
                            if (w_cnt == stoi(window_size)) {

                                //cout << "Checksum: " << checksum << "\n";

                                response = receive_data(socket);
                                response.pop_back();

                                if (stoi(response) != checksum) {
                                    //throw std::invalid_argument("[Client] Server responded with wrong checksum.");
                                    //break;
                                    logger->info("[Client] Server responded with wrong checksum");
                                } else {
                                    cout << "[Client] Checksum response from Server correct\n";
                                }
                                
                                w_cnt = 0;
                                checksum = 0;
                            } else {
                                response = receive_data(socket);
                                response.pop_back();

                                if (response != to_string(ascii_vec.at(i))) {
                                    logger->info("[Client] Server responded with wrong ACN");
                                    //throw std::invalid_argument("[Client] Server responded with wrong ACN.");
                                    //cout << i << "\n";
                                    ++i;
                                }
                            }
                        }
                        send_data(socket, "0"); //sending 0 if -p paremeter is true, because server awaits full length of frames, which count is sended before
                                                //without this, there would be an endless loop, server waits for one last character which will never be send

                        std::this_thread::sleep_for(std::chrono::milliseconds(50));
                        response = receive_data(socket);
                        response.pop_back();

                        //cout << "response: " << response << "; max: " << max_checksum << "\n";

                        if (max_checksum != stoi(response)) {
                            //throw std::invalid_argument("[Client] Server responded with wrong maximum checksum at end.");
                            cout << "[Client] Server with wrong max sum: " << response << "\n";
                            //throw std::invalid_argument("[Client] Server responded with wrong maximum checksum at end.");
                        }
                        socket.close(ec);
                        cout << "[Client] From server disconnected!\n";

                    } else {
                        socket.close();
                        cout << rang::fg::red;
                        cout << "[Client] Server responded with wrong ACN for number of data frames\nFor security measures connection is beeing closed.\n";
                        logger->error("[Client] Server responded with wrong ACN for number of data frames");
                        cout << rang::style::reset;
                    }
                } else {
                    socket.close();
                    cout << rang::fg::red;
                    cout << "[Client] Server responded with wrong ACN for window size\nFor security measures connection is closed.\n";
                    logger->error("[Client] Server responded with wrong ACN for window size");
                    cout << rang::style::reset;
                }
            } else {
                cout << rang::fg::red;
                cout << "[Client] Could not connect to Server: \n" << ec.message();
                logger->error("[Client] Client could not connect to the server: {0}", ec.message());
                cout << rang::style::reset;
            }
        } else {
            cout << "[Client] Given window size is higher than count of random ASCII values which are to transfer.\n Window size must be lower or same count.";
            cout << "Please try again with same parameters and repeat it until it is working,\nor try a minor value of the window size with \"-w\"-option!\n";
        }
    } else if (a) {
        cout << rang::fg::magenta << "\n\nAllowed characters are:\n\n" << rang::style::reset;
        Table ascii_table;
        ascii_table.format().corner_color(Color::magenta).border_color(Color::magenta)
            .font_style({FontStyle::bold}).font_color(Color::cyan);
        ascii_table.add_row({"Character", "ASCII Value"});
        int cnt = 33;
        string tmp;
        while (cnt < 127) {
            tmp = char(cnt);
            ascii_table.add_row({tmp, to_string(cnt)});
            cnt++;
        }
        cout << ascii_table << "\n";
    } else if(l) {
        auto tmp_l_path = system("readlink -f connectsim_log.txt");
        cout << rang::fg::magenta << "\n" << tmp_l_path << "\n" << rang::style::reset;
    } else {
        try {
            logger->error("Something very strange happened");
        } catch(...) {
            cout << "Could never happen\n";
        }
    }
    
}
