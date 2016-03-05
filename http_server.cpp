//
// Created by rostislav on 01.03.16.
//

#include "http_server.h"

#include <http_parser.h>

#include <fstream>
#include <string.h>

pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

pthread_attr_t attr;
int s;
void* result;

queue<uint> clients;
struct ev_loop *loop;

string url;

string dir;

string resp_ok = "HTTP/1.0 200 OK\r\n"
                 "Content-Length: ";

string resp_notfound = "HTTP/1.0 404 NOT FOUND\r\n"
                    "Content-Length: 0\r\n"
                    "Content-Type: text/html; charset=utf-8\r\n"
                    "Connection: close\r\n\r\n";
                        /*"<html>\n"
                        "<head>\n<title>Not Found</title>\n</head>\r\n"
                        "<body>\n<p>404 Request file not found.</p>\n</body>\n"
                        "</html>\r\n";*/

//#define handler_error(en, msg) \
//	do { errno = en; perror(msg); exit(EXIT_FAILURE); } while(0)

int on_url(http_parser* _, const char* at, size_t length) {
    (void)_;
    //printf("Url: %.*s\n", (int)length, at);
    char* temp_url = (char*)malloc(length*sizeof(char));
    sscanf(at, "%s", temp_url);
    string url = temp_url;
    int pos = url.find('?');
    /*ofstream debug("/tmp/debug.txt", ios_base::out);
    debug << temp_url << endl;*/

    if(pos >= 0){
        url.resize(pos);
    }

    /*debug << endl << "[realloc url]: " << url << endl;
    debug.close();*/
    return 0;
}

void request_h(string req) {
    http_parser_settings settings;
    memset(&settings, 0, sizeof(settings));
    settings.on_url = on_url;

    http_parser parser;
    http_parser_init(&parser, HTTP_REQUEST);
    http_parser_execute(&parser, &settings, req.c_str(), req.length());
}

void response_h(char* buffer){
    string file_name = dir + url;
    ifstream file(file_name, ios_base::in | ios::binary);
    ofstream log("/tmp/log_web.txt", ios_base::out | ios_base::app);
    if(file){
        string line;
        string text_file;

        while(getline(file, line)){
            text_file += line;
        }

        string resp_length = to_string(text_file.length());

        strcat(buffer, resp_ok.c_str());
        strcat(buffer, resp_length.c_str());
        strcat(buffer, "\r\nContent-Type: text/html; charset=utf-8");
        strcat(buffer, "\r\nConnection: close\r\n\r\n");
        strcat(buffer, text_file.c_str());
        strcat(buffer, "\r\n\r\n");
        log << "[OK]" << buffer <<
                "------------------------------------------" << endl;
    }else{
        strcat(buffer, resp_notfound.c_str());
        log << "[404]" << buffer <<
                "------------------------------------------" << endl;
    }
    file.close();
    log.close();
}

// BEGIN Event loop callbacks
static void read_cb(struct ev_loop *loop, struct ev_io *watcher, int revents) {
    char buffer[1024];

    ssize_t r = recv(watcher->fd, buffer, 1024, MSG_NOSIGNAL);
    if(r < 0) {
        memset(&buffer, 0, sizeof(buffer));
        strcat(buffer, resp_notfound.c_str());
        send(watcher->fd, buffer, strlen(buffer), MSG_NOSIGNAL);
        shutdown(watcher->fd, 0);
        return;
    } else if(r == 0) {
        ev_io_stop(loop, watcher);
        free(watcher);
        return;
    } else {
        //puts(buffer);
        request_h(buffer);
        memset(&buffer, 0, sizeof(buffer));
        response_h(buffer);
        //puts(buffer);
        send(watcher->fd, buffer, strlen(buffer), MSG_NOSIGNAL);
        shutdown(watcher->fd, 0);
        return;
    }
}

void accept_cb(struct ev_loop *loop, struct ev_io *watcher, int revents) {
    sockaddr_in client_sa;
    socklen_t client_sa_len = sizeof(client_sa);
    int client_sd = accept(watcher->fd, (struct sockaddr*)&client_sa, &client_sa_len);

    //cout << "client sd: " << client_sd << endl;

    // Add client to queue
    pthread_mutex_lock(&mutex);
    clients.push(client_sd);
    pthread_mutex_unlock(&mutex);
    // unlock

    pthread_t thread_id = client_sd;
    s = pthread_create(&thread_id, &attr, &http_server::handle_request, &result);
    if(s != 0) exit(1);

    s = pthread_attr_destroy(&attr);
    if(s != 0) exit(1);

//    string output;
//    output.append("client connected from ");
//    output.append(inet_ntoa(client_sa.sin_addr));
//    output.append(":");
//    output.append(to_string(client_sa.sin_port));
//    output.append("\nwith socket descriptor:");
//    output.append(to_string(client_sd));
//    puts(output.c_str());

    s = pthread_join(thread_id, &result);
    if(s != 0) exit(1);
}
// END Event loop callbacks

http_server::http_server(in_addr_t ip, uint port, string directory) : ip(ip), port(port) {
    dir = directory;
    loop = ev_default_loop(0);

    server_sd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    addr.sin_addr.s_addr = ip;
    if(bind(server_sd, (struct sockaddr *)&addr, sizeof(addr))){
        exit(1);
    }

    if(listen(server_sd, SOMAXCONN)){
        exit(1);
    }
}

void http_server::start() {
    s = pthread_attr_init(&attr);
    if(s != 0) exit(1);

    struct ev_io w_accept;
    ev_io_init(&w_accept, accept_cb, server_sd, EV_READ);
    ev_io_start(loop, &w_accept);

    while(1) ev_loop(loop, 0);
}

void* http_server::handle_request(void *arg) {
    pthread_mutex_lock(&mutex);
    int client_sd = clients.back();
    //cout << "[child] client sd: " << client_sd << endl;
    clients.pop();
    pthread_mutex_unlock(&mutex);

    struct ev_io *w_client = (struct ev_io*) malloc(sizeof(struct ev_io));
    ev_io_init(w_client, read_cb, client_sd, EV_READ);
    ev_io_start(loop, w_client);
}
