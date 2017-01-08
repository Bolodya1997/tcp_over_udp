#ifndef TCP_OVER_UDP_VIRTUAL_SOCKET_H
#define TCP_OVER_UDP_VIRTUAL_SOCKET_H

#include <templates/observer.h>
#include <net/datagram_socket.h>
#include <templates/observable.h>
#include <poll/poller.h>
#include "../unix_socket.h"
#include "../protocol.h"
#include "connector.h"
#include "messenger.h"

class virtual_socket : public observer {

    observable *const owner;
    poller *const system_poller;

    net::datagram_socket *const socket;
    unix_socket *user;
    unix_socket *to_return; //  user_end

    unsigned int window_size = 0;
    unsigned int seq_number = (unsigned int) rand();
    unsigned int ack_number;

    bool connected = false;
    unsigned int update_counter = 0;

    connector *_connector;
    messenger *_messenger;

    char buffer[protocol::MAX_MESSAGE_SIZE];

public:
    virtual_socket(observable *owner, poller *system_poller,
                   net::datagram_socket *socket);
    ~virtual_socket();

    unix_socket *get_user_end() {
        return to_return;
    }

    void start() {
        window_size = protocol::MAX_WINDOW_SIZE;
    }

    void update() override;
    void update(void *arg) override;
    void update(std::string &message);

private:
    void reset_connection();

    void socket_routine();

    void user_routine();
    void read_from_user();
    void write_to_user();
    void finish_user();
};

#endif //TCP_OVER_UDP_VIRTUAL_SOCKET_H
