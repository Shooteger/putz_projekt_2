//          Copyright Maurice Putz 2021.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE)

#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <fstream>

#include <asio.hpp>
#include <chrono>
#include <thread>
#include <nlohmann/json.hpp>
#include "spdlog/sinks/basic_file_sink.h"

using namespace std;
using json = nlohmann::json;

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

void process(asio::ip::tcp::socket socket, std::shared_ptr<spdlog::logger> logger) {

    cout << "[Server] Client connected\n";
    logger->info("[SERVER] New client connected");

    string number_of_sended_frames_tmp;
    string window_size_tmp;
    
    //getting size if sliding window must be >= 1
    try {
        window_size_tmp = receive_data(socket);
        window_size_tmp.pop_back();
        send_data(socket, "[SERVER]WS_ACN");
        logger->info("[SERVER] Window frame size ACN");
    } catch (std::system_error const& ex) {
        cout << "[SERVER] Window size not valid\n";
        logger->error("[SERVER] Window frame size could NOT be ACN");
    }
    
    //gets maximum of sent frames
    try {
        number_of_sended_frames_tmp = receive_data(socket);
        number_of_sended_frames_tmp.pop_back();
        send_data(socket, "[SERVER]F_ACN");
        logger->info("[SERVER] Maximum count of data to be received by server ACN");
    } catch (std::system_error const& ex) {
        cout << "[SERVER] Number of receiving frames not valid\n"; //later logging here
        logger->error("[SERVER] Maximum count of data to be received could NOT be ACN");
    }

    vector<char> res_vec;
    int window_size = stoi(window_size_tmp);
    int number_of_sended_frames = stoi(number_of_sended_frames_tmp);
    int window_cnt = 0;
    int cnt = 0;
    int checksum_server = 0;
    auto start_time = std::chrono::high_resolution_clock::now();    //start time for process

    while (cnt < number_of_sended_frames) {
        try {
            string res = receive_data(socket);
            res.pop_back();
            ++window_cnt;

            if (stoi(res) == 0) {
                break;
                logger->info("[SERVER] Package loss simulation ACN");
            }

            res_vec.push_back(static_cast<char>(stoi(res)));
            checksum_server += stoi(res);

            if (window_cnt == window_size) {
                cout << "[SERVER] All frames of window size received. Sending checksum...\n";
                send_data(socket, to_string(checksum_server));
                window_cnt = 0;
                checksum_server = 0;
            } else {
                send_data(socket, res);
                logger->info("[SERVER] Sending data frame ACN");
            }
        } catch (std::system_error const& ex) { //could rise after last data frame received -> ignore and go on
            break;
        }
        cnt++;

        //server ends connection to client, if whole process takes more time than 6 seconds for security reason
        auto current_time = std::chrono::high_resolution_clock::now();
        if (std::chrono::duration_cast<std::chrono::seconds>(current_time - start_time).count() > 6) {
            send_data(socket, "[SERVER] Waiting time for data exceeded");
            logger->error("[SERVER] Waiting time for data exceeded");
            socket.close();
            break;
        }
    }

    if ((int)res_vec.size() == number_of_sended_frames) {
        int tmp_max = max_sum(res_vec, window_size, number_of_sended_frames);
        send_data(socket, to_string(tmp_max));
        logger->info("[SERVER] Sending calculated max sum of windows to client");
    } else {
        send_data(socket, "[SERVER] Received package count does not match with received count of data packages to process.\nPlease look into log files.");
        string tmp_msg1 = "[SERVER] Received package count:" + to_string((int)res_vec.size());
        string tmp_msg2 = " does not match with received count of data packages: " + to_string(number_of_sended_frames);
        string tmp_msg3 = " to process. Maybe some packages got lost while transmitting them. Please try again.";
        string tmp_msg4 = tmp_msg1 + tmp_msg2 + tmp_msg3;
        logger->error(tmp_msg4.c_str());
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(90)); //wait for closing socket, to be sure client got all data
    socket.close();
    cout << "[SERVER] Client disconnected\n";
    logger->info("[SERVER] Socket to client got closed, no more data transmission possible");
}

int main() {
    asio::io_context context;
    asio::error_code er;
    asio::ip::tcp::endpoint ep(asio::ip::make_address("127.0.0.1", er), 9999);
    asio::ip::tcp::acceptor acc(context, ep);
    acc.listen();

    cout << "[SERVER] Started!\n";

    std::shared_ptr<spdlog::logger> logger;
    try {
        logger = spdlog::basic_logger_mt("basic_logger", "server_log.txt");
    } catch (const spdlog::spdlog_ex &ex) {
        cout << "Initializing logpath failed.\nTry to change the location of the server file and try again.\nCommand with \"./connectsim -h\"\n";
        throw std::invalid_argument("[SERVER] Server Could not create logfile");
    }

    while (true) {
        asio::ip::tcp::socket socket(context);
        acc.accept(socket);
        
        if (!er) {
            thread t1{process, move(socket), logger};
            t1.detach();
        } else {
            cout << "[SERVER] Connection failed: " << er.message() << "\n";
        }
    }
}