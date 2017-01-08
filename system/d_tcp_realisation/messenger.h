#ifndef TCP_OVER_UDP_MESSENGER_H
#define TCP_OVER_UDP_MESSENGER_H

#include <net/datagram_socket.h>
#include <vector>
#include <list>
#include "../protocol.h"
#include "../unix_socket.h"

class messenger {

    net::datagram_socket *const socket;

    std::vector<std::string> recv_window;
    std::list<std::string> send_window;

//  ------   user   ------
    unix_socket *const user;

    bool read_ready = false;
    bool write_ready = false;

//  ------   my state   ------
    unsigned int &max_window_size;

    unsigned int recv_window_size;
    unsigned int &seq_number;
    unsigned int &ack_number;

    bool finished;

//  ------   other's state   ------
    unsigned int send_window_size = 0;

//  ------   receive iteration   ------
    unsigned int __recv_offset = 0;
    unsigned int user_pos = 0;

//  ------   send iteration   ------
    unsigned int __seq_saved;

//  ------   improvements   ------
    bool skip_flag = false;
    unsigned int received = 0;
    unsigned int sent = 0;

    char buffer[protocol::MAX_DATA_SIZE];

public:
    messenger(net::datagram_socket *socket, unix_socket *user,
              unsigned int &window_size,
              unsigned int &seq_number, unsigned int &ack_number);

    void update();
    void update(head *message, unsigned long data_length);

    void user_read();
    void user_write();
    void user_finish();

private:
    void receive_routine();
    void send_routine();

    void receive_message(head *message, unsigned long data_length);
};

#endif //TCP_OVER_UDP_MESSENGER_H
