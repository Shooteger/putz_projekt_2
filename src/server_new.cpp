#include <iostream>
#include <asio.hpp>

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
      
        cout << window_size_tmp << "\n";
        //gets maximum of sent frames
        try {
            number_of_sended_frames_tmp = receive_data(socket);
            number_of_sended_frames_tmp.pop_back();
            send_data(socket, "[SERVER]F_ACN");
        } catch (std::system_error const& ex) {
            cout << "[SERVER] Number of receiving frames not valid\n"; //later logging here
        }

        int window_size = stoi(window_size_tmp);
        int number_of_sended_frames = stoi(number_of_sended_frames_tmp);

        int window_cnt = 0;
        int cnt = 0;
        //int tmp_cnt = 0;
        while (cnt < number_of_sended_frames) {
            //cout << "server loop:" << tmp_cnt << "\n";
            //tmp_cnt++;

            try {
                string res = receive_data(socket);
                res.pop_back();
                ++window_cnt;

                if (res == "exit") {
                    cout << "[Server] Client disconneted\n";
                    break;
                }
                //cout << "vor if ws\n";
                if (window_cnt == window_size) {
                    cout << "[Server] All frames received\n";
                    send_data(socket, res);
                    window_cnt = 0;
                }

            } catch (std::system_error const& ex) {
                //should be raised after last char receiving
                break;
            }
            cnt++;
        }
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