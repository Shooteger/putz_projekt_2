//          Copyright Maurice Putz 2021.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE)

#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <fstream>

#include <CLI11.hpp>
#define ASIO_STANDALONE
#include <asio.hpp>
#include "spdlog/sinks/basic_file_sink.h"
#include "rang.hpp"
#include <nlohmann/json.hpp>

//ignore warning "-Wnon-virtual-dtor" from extern library "tabulate"
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wnon-virtual-dtor"
#include "tabulate.hpp"
#pragma GCC diagnostic pop

using namespace std;
using namespace tabulate;
using json = nlohmann::json;

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

//reveives data of given socket and returns a string
string receive_data(asio::ip::tcp::socket& socket) {
  asio::streambuf sbf;
  asio::read_until(socket, sbf, "\n");
  string data = asio::buffer_cast<const char*>(sbf.data());
  return data;
}

//sends data to given socket
void send_data(asio::ip::tcp::socket& socket, const string message) {
    asio::write(socket, asio::buffer(message + "\n"));
}

//calculates max size if every window and returns max size of given vector elements
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
    string window_size = "1";   //attention with this!!!! window size must be less than count of elements of vector with chars to be send
    string logpath_new = "";

    //open json file and load into logpath for logger
    std::ifstream i("settings.json");
    json json_obj;
    i >> json_obj;
    string logpath_str = json_obj["logpath"];
    std::shared_ptr<spdlog::logger> logger;

    //CLI parameter
    bool a = false;
    bool l = false;
    bool pl = false;
    bool dm = false;
    bool pr = false;

    bool pl_send = false;
    bool pr_after = false;
    
    CLI::App app {"ConnectSim"};
    app.add_option("input_characters", input_chars,
         "Given characters will be random times send to server    Example: \"./connectsim asdf\"");
    app.add_option("-w,--windowsize", window_size,
         "Given number will be the size of the window of sliding window algorithm used for data transmission    Example: \"./connectsim -w 3\"")->check(CLI::PositiveNumber);
    app.add_option("-s,--set_logpath", logpath_new,
         "Change name of logfile or local path for client    Example: \"./connectsim -s logging/log_cs.txt");
    app.add_flag("-p,--packageloss", pl , "Simulates Package loss while sending data to server and otherwise");
    app.add_flag("-d,--datamanipulation", dm , "Simulates manipulation of some data packages.");
    app.add_flag("-r,--packagerow", pr , "Simulates behaviour if sended data have wrong order.");
    app.add_flag("-a,--allowed", a , "Show allowed character for input");
    app.add_flag("-l,--logpath", l , "Returns path of logfile with details on console");

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
    
    if (!a && !l && logpath_new == "") {
        const vector<char> ascii_vec = create_random_ascii(input_chars);
        size_t tmp_compare = stoi(window_size);

        if (ascii_vec.size() >= tmp_compare) {

            asio::error_code ec;
            asio::io_context context;
            asio::ip::tcp::socket socket(context);
            socket.connect(asio::ip::tcp::endpoint(asio::ip::address::from_string("127.0.0.1", ec), 9999));

            if (!ec) {
                logger->info("[CLIENT] CLIENT connected to server");
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
                        int max_checksum = max_sum(ascii_vec, stoi(window_size), (int)ascii_vec.size());

                        int change_row = 1;  //initialise with one, because otherwise if would be triggered 
                        int random_count = rand() % (int)ascii_vec.size() + 1;

                        for (size_t i=0; i < ascii_vec.size(); ++i) {

                            checksum += (int)ascii_vec.at(i);

                            if (change_row == (int)i && pr_after == true) {
                                send_data(socket, to_string(ascii_vec.at((int)i-random_count)));
                            }

                            if (dm) {
                                if (int(i) <= 2) {
                                    char tmp = static_cast<char>((rand() % (126-33)) + 33);
                                    send_data(socket, to_string(tmp));
                                    logger->info("[CLIENT] Sending manipulated data");
                                } else {
                                    dm = false;
                                }
                            } else if (pr) {
                                random_count -= (int)i;
                                change_row = i+random_count;
                                send_data(socket, to_string(ascii_vec.at(change_row)));
                                pr = false;
                                pr_after = true;
                                logger->info("[CLIENT] Sending data in wrong order");
                            } else {
                                send_data(socket, to_string(ascii_vec.at(i)));
                            }

                            ++w_cnt;

                            if (pl) {
                                ++i;
                                pl = false;
                                pl_send = true;
                                logger->info("[CLIENT] Package loss simulation start");
                            }

                            //should enter after sending maximum window size
                            if (w_cnt == stoi(window_size)) {

                                response = receive_data(socket);
                                response.pop_back();

                                if (stoi(response) != checksum) {
                                    //throw std::invalid_argument("[CLIENT] Server responded with wrong checksum.");
                                    //break;
                                    logger->info("[CLIENT] Server responded with wrong checksum");
                                } else {
                                    cout << "[CLIENT] Checksum response from Server correct\n";
                                    logger->info("[CLIENT] Checksum response from Server correct");
                                }
                                
                                w_cnt = 0;
                                checksum = 0;
                            } else {
                                response = receive_data(socket);
                                response.pop_back();

                                if (response != to_string(ascii_vec.at(i))) {
                                    logger->info("[CLIENT] Server responded with wrong ACN");
                                }
                            }
                        }
                        
                        if (pl_send)
                            send_data(socket, "0"); //sending 0 if -p paremeter is true, because server awaits full length of frames, which count is sended before
                                                    //without this, there would be an endless loop, server waits for one last character which will never be send
                        response = receive_data(socket);
                        response.pop_back();

                        try {
                            if (max_checksum != stoi(response)) {
                                cout << "[CLIENT] Server responded with wrong max sum: " << response << "\n";
                            } else {
                                cout << "[CLIENT] Server responded with right max sum: " << response << "\n";
                            }
                        } catch (std::invalid_argument const& ex) {
                            cout << response << "\n";
                        }
                        
                        socket.close(ec);
                        cout << "[CLIENT] From server disconnected\n";
                        logger->info("[CLIENT] From server disconnected");

                    } else {
                        socket.close();
                        cout << rang::fg::red;
                        cout << "[CLIENT] Server responded with wrong ACN for number of data frames\nFor security measures connection is beeing closed.\n";
                        logger->error("[CLIENT] Server responded with wrong ACN for number of data frames");
                        cout << rang::style::reset;
                    }
                } else {
                    socket.close();
                    cout << rang::fg::red;
                    cout << "[CLIENT] Server responded with wrong ACN for window size\nFor security measures connection is closed.\n";
                    logger->error("[CLIENT] Server responded with wrong ACN for window size");
                    cout << rang::style::reset;
                }
            } else {
                cout << rang::fg::red;
                cout << "[CLIENT] Could not connect to Server: \n" << ec.message();
                logger->error("[CLIENT] CLIENT could not connect to the server: {0}", ec.message());
                cout << rang::style::reset;
            }
        } else {
            cout << "[CLIENT] Given window size is higher than count of random ASCII values which are to transfer.\n Window size must be lower or same count.";
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
        string command = "readlink -f " + logpath_str;
        auto tmp_l_path = system(command.c_str());
        cout << rang::fg::magenta << "\n" << tmp_l_path << "\n" << rang::style::reset;
    } else if (logpath_new != "") {
        try {
            json_obj["logpath"] = logpath_new;
            std::ofstream o("settings.json");
            o << std::setw(4) << json_obj << endl;

            cout << rang::fg::green << "\n" << "Path of Logging file successfully changed!" << "\n" << rang::style::reset;
        } catch(...) {
            cout << rang::fg::red << "\n" << "Path of Logging file could NOT be changed!" << "\n" << rang::style::reset;
        }
    } else {
        try {
            logger->error("Something very strange happened");
        } catch(...) {
            cout << "Could never happen\n";
        }
    }
    
}
