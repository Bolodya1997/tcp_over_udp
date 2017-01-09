#ifndef TCP_OVER_UDP_PROTOCOL_H
#define TCP_OVER_UDP_PROTOCOL_H

#include <cstring>

struct head {
    unsigned int SYN : 1;
    unsigned int ACK : 1;
    unsigned int RST : 1;
    unsigned int FIN : 1;
    unsigned int window_size : 4;   //  up to 15 messages

    unsigned int seq_number;
    unsigned int ack_number;

    char data[0];
};

class protocol {
public:
    static const size_t HEAD_SIZE = sizeof(head);
    static const size_t MAX_DATA_SIZE = 1024 * 10;
    static const size_t MAX_MESSAGE_SIZE = HEAD_SIZE + MAX_DATA_SIZE;

    static const unsigned int MAX_WINDOW_SIZE = 15;

    static const int TIMEOUT = 100;
    static const int MAX_WAIT_ITERATION = 10;

    static head empty_head() {
        head res;
        bzero(&res, HEAD_SIZE);

        return res;
    }
};

#endif //TCP_OVER_UDP_PROTOCOL_H
