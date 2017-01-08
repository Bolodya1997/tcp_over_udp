#include "system_socket.h"
#include "d_tcp_realisation/connect_exception.h"

using namespace std;

//  ------   util   ------

static inline
uint64_t addr_to_key(sockaddr_in sock_addr) {
    uint64_t tmp = sock_addr.sin_addr.s_addr;
    return (tmp << 32) | sock_addr.sin_port;
}

static inline
sockaddr_in key_to_addr(uint64_t key) {
    sockaddr_in tmp = {
            .sin_family = AF_INET,
            .sin_port   = (in_port_t) key,
            .sin_addr   = { (in_addr_t) (key >> 32) },
            .sin_zero   = { 0 }
    };
    return tmp;
}

//  ------   system_socket   ------

system_socket::system_socket(observable *owner, poller *system_poller,
                             net::datagram_socket *socket)
        : owner(owner), system_poller(system_poller),
          socket(socket) {

    owner->add_observer(this);

    socket->set_owner(this);
    system_poller->add_untimed(socket->set_actions(POLL_RE));

    auto tmp = unix_socket::get_connected_pair();
    user = tmp.first;
    to_return = tmp.second;

    user->set_owner(this);
    system_poller->add_untimed(user->set_actions(POLL_WR));
}

system_socket::~system_socket() {
    if (user)
        user->close();

    owner->remove_observer(this);

    for (auto it = virt_sockets.begin(); it != virt_sockets.end(); it = virt_sockets.erase(it)) {
        socket->set_receiver(key_to_addr(it->first));
        delete it->second;
    }

    socket->close();
}

void system_socket::update() {
    for (auto it = virt_sockets.begin(); it != virt_sockets.end();) {
        socket->set_receiver(key_to_addr(it->first));
        try {
            it->second->update();
        } catch (...) {
            delete it->second;
            it = virt_sockets.erase(it);

            continue;
        }
        ++it;
    }
}

void system_socket::update(void *arg) {
    if (arg == socket) {
        socket_routine();
    } else {    //  arg == user
        accepter_routine();
    }
}

void system_socket::socket_routine() {
    ssize_t n = socket->read(buffer, protocol::MAX_MESSAGE_SIZE);
    if (n < 0)
        throw (system_exception());
    auto message = string(buffer, (unsigned long) n);

    auto addr = socket->get_sender();
    socket->set_receiver(addr);

    auto key = addr_to_key(addr);

    virtual_socket *virt_socket;
    auto it = virt_sockets.find(key);
    if (it == virt_sockets.end())
        virt_socket = add_connection(key);
    else
        virt_socket = it->second;

    try {
        virt_socket->update(message);
    } catch (connect_exception) {
        not_connected.remove(key);
        accepted.push_back(key);
        if (user->get_actions() == POLL_NO)
            accepter_routine();
    } catch (...) {
        remove_connection(key);
    }
}

virtual_socket *system_socket::add_connection(uint64_t key) {
    auto *tmp = new virtual_socket(this, system_poller, socket);
    virt_sockets.insert({ key, tmp });

    not_connected.push_back(key);

    return tmp;
}

void system_socket::remove_connection(uint64_t key) {
    not_connected.remove(key);
    accepted.remove(key);

    auto it = virt_sockets.find(key);
    if (it != virt_sockets.end()) {
        socket->set_receiver(key_to_addr(key));
        delete it->second;

        virt_sockets.erase(it);
    }

    if (virt_sockets.empty() && user == NULL)
        throw (system_exception());
}

void system_socket::accepter_routine() {
    if (user->is_disconnected()) {
        close_accepter();
        return;
    }

    if (accepted.empty()) {
        user->set_actions(POLL_NO);
        return;
    }
    user->set_actions(POLL_WR);

    auto virt_socket = virt_sockets.at(accepted.front());
    auto *tmp = virt_socket->get_user_end();
    if (user->write(&tmp, sizeof(void *)) < (int) sizeof(void *)) {
        close_accepter();
        return;
    }
    virt_socket->start();
    
    accepted.pop_front();
    return;
}

void system_socket::close_accepter() {
    user->close();
    user = NULL;

    while (!not_connected.empty()) {
        auto key = not_connected.front();
        remove_connection(key);
    }

    while (!accepted.empty()) {
        auto key = accepted.front();
        remove_connection(key);
    }
}
