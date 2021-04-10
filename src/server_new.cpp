#include <iostream>
#include <asio.hpp>
#include <chrono>
#include <thread>

using namespace std;

string receive_data(asio::ip::tcp::socket& socket) {
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

void process(asio::ip::tcp::socket socket) {
        cout << "[Server] Client connected\n";

        string number_of_sended_frames_tmp;
        string window_size_tmp;
        
        try {
            window_size_tmp = receive_data(socket);
            window_size_tmp.pop_back();
            send_data(socket, "[SERVER]WS_ACN");
        } catch (std::system_error const& ex) {
            cout << "[SERVER] Window size not valid\n"; //later logging here
        }
      
        //gets maximum of sent frames
        try {
            number_of_sended_frames_tmp = receive_data(socket);
            number_of_sended_frames_tmp.pop_back();
            send_data(socket, "[SERVER]F_ACN");
        } catch (std::system_error const& ex) {
            cout << "[SERVER] Number of receiving frames not valid\n"; //later logging here
        }

        vector<char> res_vec;

        int window_size = stoi(window_size_tmp);
        int number_of_sended_frames = stoi(number_of_sended_frames_tmp);

        int window_cnt = 0;
        int cnt = 0;
        int checksum_server = 0;
        auto start_time = std::chrono::high_resolution_clock::now();
        while (cnt < number_of_sended_frames) {
            
            try {
                string res = receive_data(socket);
                res.pop_back();
                ++window_cnt;

                if (stoi(res) == 0) {
                    break;
                }

                res_vec.push_back(static_cast<char>(stoi(res)));

                checksum_server += stoi(res);

                if (window_cnt == window_size) {
                    cout << "[Server] All frames of window size received. Sending checksum...\n";
                    send_data(socket, to_string(checksum_server));
                    window_cnt = 0;
                    checksum_server = 0;
                } else {
                    send_data(socket, res);
                    //std::this_thread::sleep_for(std::chrono::milliseconds(10));
                }
            } catch (std::system_error const& ex) {
                break;
            }
            cnt++;
            auto current_time = std::chrono::high_resolution_clock::now();
            if (std::chrono::duration_cast<std::chrono::seconds>(current_time - start_time).count() > 5) {
                send_data(socket, "[Server] Waiting time exceeded");
                break;
            }
        }


        if ((int)res_vec.size() == number_of_sended_frames) {
            int tmp_max = max_sum(res_vec, window_size, number_of_sended_frames);
            //cout << "test: " << tmp_max;
            send_data(socket, to_string(tmp_max));
        } else {
            send_data(socket, "[Server] Received package count does not match with received count of data packages to process.\nPlease look into log files.");
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(80));
        socket.close();
        cout << "[Server] Client disconnected\n";
}

int main() {
    //asio::shared_ptr<ip::tcp::socket> socket_ptr;

    asio::io_context context;
    asio::error_code er;
    asio::ip::tcp::endpoint ep(asio::ip::make_address("127.0.0.1", er), 9999);
    asio::ip::tcp::acceptor acc(context, ep);
    acc.listen();

    cout << "[Server] Started!\n";

    while (true) {
        asio::ip::tcp::socket socket(context);
        acc.accept(socket);

        if (!er) {
            thread t1{process, move(socket)};
            t1.detach();
        } else {
            cout << "[Server] Connection failed:\n" << er.message() << "\n";
        }
    }
}