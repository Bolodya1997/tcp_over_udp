#ifndef TCP_OVER_UDP_CONNECTOR_H
#define TCP_OVER_UDP_CONNECTOR_H

#include <net/datagram_socket.h>
#include "../protocol.h"

class connector {

    net::datagram_socket *const socket;

    unsigned syn = 1;
    unsigned ack = 0;

    bool &connected;

    unsigned &seq_number;
    unsigned &ack_number;

public:
    connector(net::datagram_socket *socket, bool &connected,
              unsigned &seq_number, unsigned &ack_number)
            : socket(socket), connected(connected),
              seq_number(seq_number), ack_number(ack_number) { }

    void update();
    void update(head *message);
};

#endif //TCP_OVER_UDP_CONNECTOR_H
