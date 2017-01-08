#include "d_server_socket.h"
#include "../system/system.h"
#include "d_socket.h"

d_server_socket::d_server_socket(uint16_t port)
        : socket(system::get_instance()->get_accepter(port)) {

    filed = get_filed(*socket);
}

d_server_socket::~d_server_socket() {
    socket->close();
    delete socket;
    socket = NULL;
}

pollable *d_server_socket::accept() {
    pollable::accept();

    unix_socket *tmp;
    ssize_t n = socket->read(&tmp, sizeof(void *));
    if (n < (int) sizeof(void *))
        throw (net_exception("accept"));

    return new d_socket(tmp);
}
