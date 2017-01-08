#include "connector.h"
#include "../system_exception.h"

using namespace std;

void connector::update() {
    auto _message = protocol::empty_head();
    _message.SYN = syn;
    _message.seq_number = seq_number;

    if (ack) {
        _message.ACK = ack;
        _message.ack_number = ack_number;
    }

    if (socket->write(&_message, protocol::HEAD_SIZE) < (int) protocol::HEAD_SIZE) {
        throw (system_exception());
    }
}

void connector::update(head *message) {
    if (!ack && !message->SYN)
        throw (system_exception());

    if (message->SYN) {
        ack = 1;
        ack_number = message->seq_number;
    }

    if (message->ACK) {
        if (message->ack_number == seq_number)
            connected = true;
        else
            throw (system_exception());
    }

}
