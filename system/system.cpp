#include <net/datagram_socket.h>
#include "system.h"
#include "system_socket.h"

using namespace std;
using namespace chrono;

class system *const system::instance = new system();

void system::main_loop() {
    while (true) {
        system_poller.poll();

        auto &ready = system_poller.get_ready();
        for (auto it = ready.begin(); it != ready.end(); it++) {
            observer *owner = (*it)->get_owner();
            try {
                owner->update(*it);
            } catch (...) {
                delete owner;
            }
        }

        millis now = duration_cast<millis>(system_clock::now().time_since_epoch());
        if ((now - last_update).count() > protocol::TIMEOUT)
            update_all();
        last_update = now;
    }
}

unix_socket *system::get_accepter(uint16_t port) {
    auto *socket = new net::datagram_socket(port);
    system_socket *sys_socket;

    try {
        sys_socket = new system_socket(this, &system_poller, socket);
    } catch (std::exception &e) {
        delete socket;
        throw e;
    }

    return sys_socket->get_user_end();
}

unix_socket *system::get_socket(std::string hostname, uint16_t port) {
    auto *socket = new net::datagram_socket();
    virtual_socket *virt_socket;

    try {
        virt_socket = new virtual_socket(this, &system_poller, socket);
        virt_socket->start();
    } catch (std::exception &e) {
        delete socket;
        throw e;
    }

    socket->set_owner(virt_socket);
    socket->set_receiver(hostname, port);
    system_poller.add_untimed(socket->set_actions(POLL_RE));

    return virt_socket->get_user_end();
}

void system::wait() {
    while (!this->is_empty());
}
