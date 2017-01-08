#ifndef TCP_OVER_UDP_SYSTEM_H
#define TCP_OVER_UDP_SYSTEM_H

#include <map>
#include <set>
#include <poll/poller.h>
#include <templates/observable.h>
#include <thread/thread.h>
#include "unix_socket.h"
#include "protocol.h"

class system : public observable {
//  ------   singletone start   ------
private:
    static system *const instance;
    system() : system_poller(protocol::TIMEOUT) {
        thread([this]() -> void { main_loop(); }).start();
    }
    ~system() { }

public:
    static system *get_instance() {
        return instance;
    }
//  ------   singletone end   ------

private:
    poller system_poller;

    millis last_update = std::chrono::duration_cast<millis>
            (std::chrono::system_clock::now().time_since_epoch());

public:
    void main_loop();

    unix_socket *get_accepter(uint16_t port);
    unix_socket *get_socket(std::string hostname, uint16_t port);

    void wait();

private:
    void __abstract_guard() override { };
};

#endif //TCP_OVER_UDP_SYSTEM_H
