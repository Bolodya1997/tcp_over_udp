#include "d_socket.h"
#include "../system/system.h"

d_socket::d_socket(unix_socket *socket) : socket(socket) {
    filed = get_filed(*socket);
}

d_socket::d_socket(std::string hostname, uint16_t port)
        : d_socket(system::get_instance()->get_socket(hostname, port)) {
}

d_socket::~d_socket() {
    socket->close();
    delete socket;
    socket = NULL;
}

pollable *d_socket::accept() {
    pollable::accept();

    void *tmp;
    if (socket->read(&tmp, sizeof(void *)) < (int) sizeof(void *))
        throw (net_exception("connect"));

    return this;
}

ssize_t d_socket::read(void *buff, size_t n) {
    return socket->read(buff, n);
}

ssize_t d_socket::write(const void *buff, size_t n) {
    return socket->write(buff, n);
}
