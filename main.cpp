#include "http_server.h"

#include <sys/types.h>
#include <sys/stat.h>

void parse_cmd(int argc, char* argv[], in_addr_t &ip, uint &port, string &dir);

int main(int argc, char* argv[]) {
    in_addr_t ip;
    uint port;
    string dir;

    parse_cmd(argc, argv, ip, port, dir);


    http_server hs(ip, port, dir);
    hs.start();

    pid_t process_id = 0;
    pid_t sid = 0;

    process_id = fork();
    if (process_id > 0)
    {
        //printf("process_id of child process %d \n", process_id);
        exit(1);
    }

    umask(0);

    sid = setsid();
    if(sid < 0)
    {
        exit(1);
    }
    chdir("/");
    close(STDIN_FILENO);
    close(STDOUT_FILENO);
    close(STDERR_FILENO);

    return 0;
}

void parse_cmd(int argc, char* argv[], in_addr_t &ip, uint &port, string &dir){
    int opt;
    while((opt = getopt(argc, argv, "h:p:d:")) != -1 ){
        switch (opt){
            case 'h':
                ip = inet_addr(optarg);
                break;
            case 'p':
                port = atoi(optarg);
                break;
            case 'd':
                dir = optarg;
                break;
            default:
                cerr << "error getopt" << endl;
                return;
        }
    }
}