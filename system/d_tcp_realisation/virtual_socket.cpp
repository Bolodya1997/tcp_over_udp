#include "virtual_socket.h"
#include "connect_exception.h"

using namespace std;

virtual_socket::virtual_socket(observable *owner, poller *system_poller,
                               net::datagram_socket *socket)
        : owner(owner), system_poller(system_poller),
          socket(socket) {

    owner->add_observer(this);

    auto tmp = unix_socket::get_connected_pair();
    user = tmp.first;
    to_return = tmp.second;

    _connector = new connector(socket, connected, seq_number, ack_number);
    _messenger = new messenger(socket, user, window_size, seq_number, ack_number);

    user->set_owner(this);
    system_poller->add_untimed(user->set_actions(POLL_NO));
}

virtual_socket::~virtual_socket() {
    user->close();

    owner->remove_observer(this);

    reset_connection();

    if (socket->get_owner() == this)
        socket->close();

    delete _connector;
    delete _messenger;
}

inline
void virtual_socket::reset_connection() {
    auto message = protocol::empty_head();
    message.RST = 1;

    socket->write(&message, protocol::HEAD_SIZE);
}

void virtual_socket::update() {
    try {
        if (++update_counter > protocol::MAX_WAIT_ITERATION)
            throw (system_exception());

        if (!connected)
            _connector->update();
        else
            _messenger->update();
    } catch (exception &e) {
        if (socket->get_owner() != this)
            throw e;
    }
}

void virtual_socket::update(void *arg) {
    if (arg == socket)
        socket_routine();
    else    //  arg = user
        user_routine();
}

void virtual_socket::socket_routine() {
    ssize_t n = socket->read(buffer, protocol::MAX_MESSAGE_SIZE);
    if (n < 0)
        throw (system_exception());
    auto message = string(buffer, (unsigned long) n);

    update(message);
}

void virtual_socket::update(std::string &message) {
    update_counter = 0;

    if (message.length() < protocol::HEAD_SIZE)
        throw (system_exception());

    auto _message = (head *) message.data();
    if (_message->RST)
        throw (system_exception());

    bool __old_connected = connected;
    if (!connected)
        _connector->update(_message);

    if (connected)
        _messenger->update(_message, message.length() - protocol::HEAD_SIZE);

    if (__old_connected != connected) {
        user->set_actions(POLL_RE | POLL_WR);

        if (socket->get_owner() != this)
            throw (connect_exception());
        else
            ssize_t n = user->write(to_return, sizeof(void *));
    }
}

inline
void virtual_socket::user_routine() {
    if (user->is_readable())
        read_from_user();

    if (user->is_writable())
        write_to_user();

    if (user->is_disconnected())
        finish_user();
}

inline
void virtual_socket::read_from_user() {
    _messenger->user_read();
}

inline
void virtual_socket::write_to_user() {
    _messenger->user_write();
}

inline
void virtual_socket::finish_user() {
    _messenger->user_finish();
}
