#ifndef TCP_OVER_UDP_D_SOCKET_H
#define TCP_OVER_UDP_D_SOCKET_H

#include <poll/pollable.h>
#include "../system/unix_socket.h"

class d_socket : public pollable {

    unix_socket *socket;

    d_socket(unix_socket *socket);

public:
    d_socket(std::string hostname, uint16_t port);
    ~d_socket();

    pollable *accept() override;    //  connect

    ssize_t read(void *buff, size_t n) override;
    ssize_t write(const void *buff, size_t n) override;

private:
    void __abstract_guard() override { }

    friend class d_server_socket;
};

#endif //TCP_OVER_UDP_D_SOCKET_H
