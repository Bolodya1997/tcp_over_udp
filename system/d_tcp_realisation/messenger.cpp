#include <iostream>
#include <logging.h>
#include "messenger.h"

using namespace std;

messenger::messenger(net::datagram_socket *socket, unix_socket *user,
                     unsigned int &window_size,
                     unsigned int &seq_number, unsigned int &ack_number)
        : socket(socket), user(user),
          max_window_size(window_size),
          seq_number(seq_number), ack_number(ack_number) {

    recv_window.resize(protocol::MAX_WINDOW_SIZE);

    recv_window_size = max_window_size;

    __seq_saved = seq_number;
}

void messenger::update() {
    receive_routine();
    send_routine();
}

void messenger::receive_routine() {
    received = 0;

    unsigned int pos;
    for (pos = __recv_offset; pos < protocol::MAX_WINDOW_SIZE; pos++) {
        if (recv_window[pos].empty())
            break;
    }
    ack_number += pos - __recv_offset;
    __recv_offset = pos;

    if (user_pos > 0) {
        recv_window.erase(recv_window.begin(), recv_window.begin() + user_pos);
        __recv_offset -= user_pos;

        user_pos = 0;
        recv_window.resize(protocol::MAX_WINDOW_SIZE);
    }

    recv_window_size = max_window_size - __recv_offset;

    if (write_ready)
        user_write();
}

void messenger::send_routine() {
    sent = seq_number - __seq_saved;
    for (int i = 0; i < sent; i++)
        send_window.pop_front();

    if (read_ready)
        user_read();

    auto message = protocol::empty_head();
    message.ACK = 1;
    message.seq_number = seq_number;
    message.ack_number = ack_number;
    message.window_size = recv_window_size;

    __seq_saved = seq_number;

    skip_flag = !skip_flag;
    if (skip_flag || send_window_size == 0 || send_window.size() == 0) {
        if (finished && send_window.size() == 0)
            message.RST = 1;
        socket->write(&message, protocol::HEAD_SIZE);
        return;
    }

    int pos = 0;
    for (auto it = send_window.begin(); it != send_window.end(); it++) {
        if (++pos > send_window_size)
            return;

        ++message.seq_number;
        auto head_wrap = string((char *) &message, protocol::HEAD_SIZE);

        auto tmp = head_wrap + *it;
        socket->write(tmp.data(), tmp.length());
    }
}

void messenger::update(head *message, unsigned long data_length) {
    if (message->ACK) {
        seq_number = max(seq_number, message->ack_number);
        sent = seq_number - __seq_saved;
    }

    if (data_length != 0)
        receive_message(message, data_length);

    send_window_size = message->window_size;

    if (received != 0 && (received >= recv_window_size) ||
            sent != 0 && (sent == send_window_size || sent == send_window.size()))
        update();
}

void messenger::user_finish() {  //  TODO: think about FIN usage
    user->set_actions(user->get_actions() & ~POLL_WR);
    write_ready = false;

    finished = true;
}

void messenger::receive_message(head *message, unsigned long data_length) {
    unsigned int dist = message->seq_number - ack_number;
    if (1 <= dist && dist <= recv_window_size) {
        recv_window[__recv_offset + dist - 1] = string(message->data, data_length);
        ++received;
    }
}

void messenger::user_read() {
    if (send_window.size() >= send_window_size) {
        user->set_actions(user->get_actions() & ~POLL_RE);
        read_ready = true;
        return;
    }

    ssize_t n = user->read(buffer, protocol::MAX_DATA_SIZE);
    if (n < 1) {
        user->set_actions(user->get_actions() & ~POLL_RE);
        read_ready = false;

        user_finish();
        return;
    } else {
        send_window.push_back(string(buffer, (unsigned long) n));
    }

    if (read_ready) {
        user->set_actions(user->get_actions() | POLL_RE);
        read_ready = false;
    } else {
        update();
    }
}

void messenger::user_write() {
    if (user_pos >= __recv_offset) {
        user->set_actions(user->get_actions() & ~POLL_WR);
        write_ready = true;
        return;
    }

    for (; user_pos < __recv_offset; user_pos++) {
        auto &cur = recv_window[user_pos];

        ssize_t n = user->write(cur.data(), cur.length());
        if (n < 1) {
            if (errno == EAGAIN)
                break;
            user_finish();
            return;
        } else if (n < cur.length()) {
            cur = cur.substr((unsigned long) n);
            break;
        }
    }

    if (write_ready) {
        user->set_actions(user->get_actions() | POLL_WR);
        write_ready = false;
    } else {
        update();
    }
}
