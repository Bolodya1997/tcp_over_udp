#include <poll/poller.h>
#include <sys/wait.h>
#include <iostream>
#include <net/datagram_socket.h>
#include <thread/thread.h>
#include "d_tcp/d_server_socket.h"
#include "d_tcp/d_socket.h"
#include "system/system.h"

using namespace std;

char c_buff[1024];
string s_buff;

void parent_routine() {
    auto *server = new d_server_socket(2000);

    auto __poller = new poller(-1);
    __poller->add_untimed(server->set_actions(POLL_AC));
    __poller->poll();
    auto *socket = server->accept();
    server->close();

    __poller->add_untimed(socket->set_actions(POLL_RE));
    while (true) {
        __poller->poll();
        ssize_t n = socket->read(c_buff, 1024);
        if (n < 1) {
            cerr << "------   parent out   ------" << endl;
            return;
        }

        cerr << string(c_buff, (unsigned long) n);
    }
}

void child_routine() {
    auto socket = new d_socket("localhost", 2000);

    auto __poller = new poller(-1);
    __poller->add_untimed(socket->set_actions(POLL_AC));
    __poller->poll();
    socket->accept();

    socket->set_actions(POLL_WR);
    while (true) {
        s_buff = "012345678901234567890123456789";
        cin >> s_buff;

        __poller->poll();
        ssize_t n = socket->write(s_buff.data(), s_buff.length());
        if (n < 1) {
            cerr << "------   child out   ------" << endl;
            return;
        }

        delete socket;
        return;
    }
}

int main() {

    thread([]() -> void { parent_routine(); }).start();

    child_routine();

    system::get_instance()->wait();
    return 0;
}