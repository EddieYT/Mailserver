#include <stdlib.h>
#include <stdio.h>
#include <iostream>
#include <unistd.h>
#include <stdexcept>
#include <cstring>
#include <string>
#include <vector>
#include <sys/time.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <pthread.h>
#include <algorithm>
#include <signal.h>
#include "thread_func.h"
using namespace std;

extern char *optarg;
extern int optind, opterr, optopt;
bool ctrlc_flag = false;
vector<pthread_t*> threads;
vector<thread_data*> tds;
vector<string> maillist;
map<string, string> users;
int socket_fd;
char* directory;

/*
This function will handle Ctrl + C and terminate all threads.
*/
void sig_handler(int sig)
{
  if (sig == SIGINT) {
    ctrlc_flag = true;
    // All threads join here
    for (int i = 0; i < threads.size(); i++) {
      pthread_join(*(threads.at(i)), NULL);
      delete(threads.at(i));
      delete(tds.at(i));
    }
    close(socket_fd);
    exit(0);
  }
}

int main(int argc, char *argv[])
{
  // Process the command line arguments
  int pflag = 0, aflag = 0, vflag = 0, opt;
  int port = 10000;
  while ((opt = getopt(argc, argv, "p:av")) != -1) {
    switch(opt) {
      case 'p':
        pflag = 1;
        try {
          port = stoi(optarg);
        } catch (invalid_argument &ia) {
          cerr << "Invalid argument: please enter an integer." << endl;
          return -1;
        }
        break;
      case 'a':
        aflag = 1;
        break;
      case 'v':
        vflag = 1;
        break;
      default:
        abort();
    }
  }
  if (aflag) {
    cerr << "Author: Ying Tsai / yingt" << endl;
    return -1;
  }
  // Create a socket as an endpoint
  socket_fd = socket(AF_INET, SOCK_STREAM, 0);
  if (socket_fd < 0) cerr << "Creating Socket Fails." << endl;
  struct sockaddr_in addr;
  //Clear address structure
  memset(&addr, 0, sizeof(struct sockaddr_in));

  // Set up the host addr
  addr.sin_family = AF_INET;
  addr.sin_addr.s_addr = INADDR_ANY;
  addr.sin_port = htons(port);
  if (bind(socket_fd, (struct sockaddr*) &addr, sizeof(addr)) < 0) {
    cerr << "Error on binding." << endl;
    return -1;
  }
  // Server starts listening on the assigned port.
  listen(socket_fd, 100);
  while (true) {
    if (signal(SIGINT, sig_handler) == SIG_ERR) {
      cerr << "Can't catch ctrl + c.\n";
    }
    struct sockaddr_in cli_addr;
    memset(&cli_addr, 0, sizeof(struct sockaddr_in));
    socklen_t cli_len = sizeof(cli_addr);
    int* cfd = new int;
    *cfd = accept(socket_fd, (struct sockaddr*) &cli_addr, &cli_len);
    if (*cfd < 0) {
      cerr << "Error on accept." << endl;
      return -1;
    }
    if (vflag) fprintf(stderr, "[%d] New connection\n", *cfd);
    // Create a new thread to deal with the incoming connection.
    pthread_t* cur_thread = new pthread_t;
    thread_data* td = new thread_data;
    td->vflag = vflag;
    td->cfd = cfd;
    pthread_create(cur_thread, NULL, read_connection, (void*) td);
    threads.push_back(cur_thread);
    tds.push_back(td);
  }
  close(socket_fd);
  return 0;
}
