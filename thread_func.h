#include "process_cmd.h"
using namespace std;

extern bool ctrlc_flag;

/*
This function will read data from the fd when a connection to client is established.
*/
void* read_connection(void* input) {
  thread_data* td = (thread_data*) input;
  int* cfd = td->cfd;
  char buffer[256];
  struct timeval tv;
  fd_set rfds;
  tv.tv_usec = 10;
  FD_ZERO(&rfds);
  // Send the greeting message.
  char greeting[] = "220 localhost service ready\r\n";
  send(*cfd, greeting, strlen(greeting), 0);
  if (td->vflag) fprintf(stderr, "[%d] S: %s", *cfd, greeting);
  string command;
  int select_val;
  int valread;
  while (!ctrlc_flag) {
    FD_SET(*cfd, &rfds);
    bool quit_flag = false;
    // Check if there's a fd to be read.
    if ((select_val = select(*cfd + 1, &rfds, NULL, NULL, &tv)) > 0) {
      valread = read(*cfd, buffer, 256);
      for (int i = 0; i < valread; i++) {
        if (buffer[i] == '\r') {
          continue;
        } else if (buffer[i] == '\n') {
          quit_flag = process_command(td, command);
          if (quit_flag) break;
          command = "";
        } else {
          command += string(1, buffer[i]);
        }
      }
      memset(&buffer, 0, sizeof(buffer));
    }
    if (quit_flag) break;
  }
  if (td->vflag) fprintf(stderr, "[%d] Connection closed\n", *cfd);
  char err_terminate[] = "-ERR Server shutting down\r\n";
  if (ctrlc_flag) send(*cfd, err_terminate, strlen(err_terminate), 0);
  close(*cfd);
  delete(cfd);
  pthread_exit(NULL);
}
