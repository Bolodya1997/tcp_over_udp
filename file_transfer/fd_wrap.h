#ifndef TCP_OVER_UDP_FD_WRAP_H
#define TCP_OVER_UDP_FD_WRAP_H

#include <poll/pollable.h>

class fd_wrap : public pollable {

public:
    fd_wrap(int filed) {
        this->filed = filed;
    }

    ssize_t read(void *buff, size_t n) override {
        return ::read(filed, buff, n);
    }

    ssize_t write(const void *buff, size_t n) override {
        return ::write(filed, buff, n);
    }

private:
    void __abstract_guard() override { }
};

#endif //TCP_OVER_UDP_FD_WRAP_H

