#ifndef TCP_OVER_UDP_UNIX_SOCKET_H
#define TCP_OVER_UDP_UNIX_SOCKET_H

#include <net/socket.h>
#include "system_exception.h"

class unix_socket : public net::socket {

    unix_socket *other;

    unix_socket(int filed, unix_socket *other) : socket(filed), other(other) { }

public:
    static std::pair<unix_socket *, unix_socket *> get_connected_pair() {
        int tmp[2];
        if (socketpair(AF_UNIX, SOCK_STREAM | SOCK_NONBLOCK, 0, tmp) < 0)
            throw (system_exception());

        unix_socket *_1 = new unix_socket(tmp[0], NULL);
        unix_socket *_2 = new unix_socket(tmp[1], _1);
        _1->other = _2;

        return { _1, _2 };
    }

    void close() override {
        net::socket::close();

        other = NULL;
    }

    bool is_other_closed() {
        return other == NULL;
    }
};

#endif //TCP_OVER_UDP_UNIX_SOCKET_H