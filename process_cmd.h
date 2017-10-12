#include <regex>
#include "process_mail.h"
using namespace std;
using namespace regex_constants;

char err[] = "500 Syntax error, command unrecognised\r\n";
char err501[] = "501 Syntax error in parameters or arguments\r\n";
char err503[] = "503 Bad sequence of commands\r\n";
char err550[] = "550 mailbox unavailable\r\n";
char data354[] = "354 send the mail data, end with .\r\n";
char farewell[] = "221 localhost Service closing transmission channel\r\n";
char helomsg[] = "250 localhost\r\n";
char ok[] = "250 OK\r\n";

smatch matches;
regex helo_reg ("helo .*\r\n$", ECMAScript | icase );
regex mail_reg ("mail from:<(\\S+@localhost)>\r\n$", ECMAScript | icase );
regex rcpt_reg ("rcpt to:<(\\S+@localhost)>\r\n$", ECMAScript | icase );
regex data_reg ("data (.*\r\n)$", ECMAScript | icase );

enum string_code {
  quit,
  rset,
  noop,
  data,
  mailing,
  recving,
  reading,
  init,
  helo,
  error,
};

string_code hash_hit (string const& in_string){
  if(in_string == "QUIT") return quit;
  if(in_string == "RSET") return rset;
  if(in_string == "NOOP") return noop;
  if(in_string == "DATA") return data;
  if(in_string == "HELO") return helo;
  return error;
};

void reset_data(thread_data* input) {
  input->status = init;
  input->sender.clear();
  input->content.clear();
  input->receivers.clear();
}
/*
This function will process the command and send corresponding messages
to the client.
If a "quit" command received, it returns true; otherwise, returns false.
 */
bool process_command(thread_data* td, string command) {
  int* cfd = td->cfd;
  if (td->vflag) fprintf(stderr, "[%d] C: %s\n", *cfd, command.c_str());
  // Keep reading data if it's at the DATA state.
  if (td->status == reading) {
    if (command == ".\r\n") {
      send_mail(td);
      reset_data(td);
      send(*cfd, ok, strlen(ok), 0);
      return false;
    } else if(command.length() == 6) {
      string cm_type = command.substr(0,4);
      transform(cm_type.begin(), cm_type.end(), cm_type.begin(), ::toupper);
      if (cm_type == "RSET") {
        reset_data(td);
        td->status = init;
        send(*cfd, ok, strlen(ok), 0);
        return false;
      }
    }
    td->content += command;
    cout << td->content << endl;
    return false;
  }
  // Start processing the current command.
  if (command.length() < 6) {
    send(*cfd, err, strlen(err), 0);
    if (td->vflag) fprintf(stderr, "[%d] S: %s", *cfd, err);
    return false;
  }
  if (command.length() == 6) {
    string cm_type = command.substr(0,4);
    transform(cm_type.begin(), cm_type.end(), cm_type.begin(), ::toupper);
    switch(hash_hit(cm_type)) {
      case (quit):
        send(*cfd, farewell, strlen(farewell), 0);
        if (td->vflag) fprintf(stderr, "[%d] S: %s", *cfd, farewell);
        return true;
      case (rset):
        reset_data(td);
        td->status = init;
        send(*cfd, ok, strlen(ok), 0);
        break;
      case (noop):
        send(*cfd, ok, strlen(ok), 0);
        break;
      case (helo):
        send(*cfd, err501, strlen(err501), 0);
        break;
      case (data):
        switch(td->status) {
          case recving:
            td->status = reading;
            send(*cfd, data354, strlen(data354), 0);
            break;
          default:
            send(*cfd, err503, strlen(err503), 0);
        }
        break;
      default:
        send(*cfd, err, strlen(err), 0);
    }
  } else {
    if(regex_search(command, matches, helo_reg)) {
      switch(td->status) {
        case init:
          send(*cfd, helomsg, strlen(helomsg), 0);
          break;
        default:
          send(*cfd, err503, strlen(err503), 0);
      }
      cout << "Match found\n";
    } else if(regex_search(command, matches, mail_reg)) {
      string datetime = get_datetime();
      switch(td->status) {
        case init:
          send(*cfd, ok, strlen(ok), 0);
          td->status = mailing;
          td->sender = matches[1];
          td->content += "From <" + td->sender + "> " + datetime + "\r\n";
          cout << td->content << endl;
          break;
        default:
          send(*cfd, err503, strlen(err503), 0);
      }
    } else if(regex_search(command, matches, rcpt_reg)) {
      switch(td->status) {
        case mailing:
          td->status = recving;
          if (!in_maillist(td, matches[1])) {
            send(*cfd, err550, strlen(err550), 0);
            break;
          }
          td->receivers.push_back(matches[1]);
          send(*cfd, ok, strlen(ok), 0);
          break;
        case recving:
          if (!in_maillist(td, matches[1])) {
            send(*cfd, err550, strlen(err550), 0);
            break;
          }
          td->receivers.push_back(matches[1]);
          send(*cfd, ok, strlen(ok), 0);
          break;
        default:
          send(*cfd, err503, strlen(err503), 0);
      }
      cout << "Match found\n";
    } else if(regex_search(command, matches, data_reg)) {
      switch(td->status) {
        case recving:
          td->status = reading;
          td->content += matches[1];
          send(*cfd, data354, strlen(data354), 0);
          break;
        default:
          send(*cfd, err503, strlen(err503), 0);
      }
      cout << "Match found\n";
    } else {
      cout << "Match not found\n";
      send(*cfd, err, strlen(err), 0);
      if (td->vflag) fprintf(stderr, "[%d] S: %s", *cfd, err);
    }
  }
  return false;
}
