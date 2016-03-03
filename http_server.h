//
// Created by rostislav on 01.03.16.
//

#ifndef WEB_SERVER_HTTP_SERVER_H
#define WEB_SERVER_HTTP_SERVER_H

#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <ev.h>
#include <queue>
#include <pthread.h>

using namespace std;

class http_server {
private:
    in_addr_t ip;
    uint port;

    // Request handler
    void handle_http();

public:
    // Create and run server event loop
    int server_sd;
    http_server(in_addr_t ip, uint port, string dir);
    void start();
    // Thread-functions for handling requests
    static void* handle_request(void* arg);
    ~http_server(){}
};


#endif //WEB_SERVER_HTTP_SERVER_H
