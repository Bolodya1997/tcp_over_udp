#ifndef TCP_OVER_UDP_D_SERVER_SOCKET_H
#define TCP_OVER_UDP_D_SERVER_SOCKET_H

#include <poll/pollable.h>
#include "../system/unix_socket.h"

class d_server_socket : public pollable {

    unix_socket *socket;

public:
    d_server_socket(uint16_t port);
    ~d_server_socket();

    virtual pollable *accept() override;

private:
    void __abstract_guard() override { }
};

#endif //TCP_OVER_UDP_D_SERVER_SOCKET_H
