#pragma once

#include <switch.h>
#include <string>

namespace netloader {

    typedef struct {
        bool activated;
        bool launch_app;
        bool transferring;
        bool sock_connected;
        size_t filelen, filetotal;
        std::string errormsg;
    } State;

    void task(void* arg);

    void getState(State *state);
    void signalExit();

    Result setNext();
}
