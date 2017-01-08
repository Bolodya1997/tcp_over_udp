#ifndef TCP_OVER_UDP_SYSTEM_SOCKET_H
#define TCP_OVER_UDP_SYSTEM_SOCKET_H

#include <templates/observer.h>
#include <net/datagram_socket.h>
#include <templates/observable.h>
#include <poll/poller.h>
#include <map>
#include <queue>
#include "unix_socket.h"
#include "d_tcp_realisation/virtual_socket.h"
#include "protocol.h"

class system_socket : public observer,
                      public observable {

    observable *const owner;
    poller *const system_poller;

    net::datagram_socket *const socket;
    unix_socket *user;
    unix_socket *to_return; //  user_end

    std::map<uint64_t, virtual_socket *> virt_sockets;
    std::list<uint64_t> not_connected;
    std::list<uint64_t> accepted;

    char buffer[protocol::MAX_MESSAGE_SIZE];

public:
    system_socket(observable *owner, poller *system_poller,
                  net::datagram_socket *socket);
    ~system_socket();

    unix_socket *get_user_end() {
        return to_return;
    }

    void update() override;
    void update(void *arg) override;

private:
    void socket_routine();
    virtual_socket *add_connection(uint64_t key);
    void remove_connection(uint64_t key);

    void accepter_routine();
    void close_accepter();

    void __abstract_guard() override { }
};

#endif //TCP_OVER_UDP_SYSTEM_SOCKET_H
