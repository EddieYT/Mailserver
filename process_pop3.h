#include <regex>
#include <sys/file.h>
#include <map>
#include <set>
#include "process_pop3mail.h"
using namespace std;
using namespace regex_constants;

char err[] = "-ERR Syntax error, command unrecognised\r\n";
char bad_seq[] = "-ERR Bad command sequence\r\n";
char user_found[] = "+OK User found\r\n";
char user_nfound[] = "-ERR User not found\r\n";
char farewell[] = "+OK Goodbye, POP3 server signing off\r\n";
char pw_valid[] = "+OK password confirmed and maildrop is opened\r\n";
char pw_invalid[] = "-ERR invalid password\r\n";
char file_notopen[] = "-ERR file open fails\r\n";
char no_msg[] = "-ERR no such message in maildrop\r\n";
char noop[] = "+OK Nothing\r\n";
char update_err[] = "some deleted messages not removed\r\n";
char mail_gone[] = "-ERR message already deleted\r\n";
char mail_del[] = "+OK message deleted\r\n";
char retr_msg[] = "+OK message follows\r\n";
char end_msg[] = ".\r\n";

regex user_reg ("^user (.+)\r\n$", ECMAScript | icase );
regex pass_reg ("^pass (.+)\r\n$", ECMAScript | icase );
regex quit_reg ("^quit\r\n$", ECMAScript | icase );
regex stat_reg ("^stat\r\n$", ECMAScript | icase);
regex uidl_reg ("^uidl\r\n$|^uidl (\\d+)\r\n$", ECMAScript | icase);
regex retr_reg ("^retr (\\d+)\r\n$", ECMAScript | icase);
regex dele_reg ("^dele (\\d+)\r\n$", ECMAScript | icase);
regex noop_reg ("^noop\r\n$", ECMAScript | icase);
regex rset_reg ("^rset\r\n$", ECMAScript | icase);
regex list_reg ("^list\r\n$|^list (\\d+)\r\n$", ECMAScript | icase);

enum string_code {
  init,
  pass,
  trans,
  update,
};

void reset_data(thread_data* input) {
  input->status = init;
  input->login.clear();
}

void send_back(int fd, char arr[], thread_data* td) {
  if (td->vflag) {
    fprintf(stderr, "[%d] S: %s", fd, arr);
  }
  send(fd, arr, strlen(arr), 0);
}

/* This handler funciton will deal with incoming "RETR" command*/
void retr_handler(thread_data* td, smatch matches, int* cfd) {
  string strnum = matches[1];
  int num;
  char res[1024];
  switch(td->status) {
    case trans:
      num = stoi(strnum);
      if (num > td->mails.size() || num <= 0) {
        send_back(*cfd, no_msg, td);
      } else {
        if (td->del_set.find(get_uid(td->mails.at(num - 1))) != td->del_set.end()) {
          send_back(*cfd, mail_gone, td);
        } else {
          send_back(*cfd, retr_msg, td);
          strcpy(res, td->mails.at(num - 1).c_str());
          send_back(*cfd, res, td);
          send_back(*cfd, end_msg, td);
        }
      }
      break;
    default:
      send_back(*cfd, bad_seq, td);
  }
}

/* This handler funciton will deal with incoming "RSET" command*/
void rset_handler(thread_data* td, smatch matches, int* cfd) {
  int num_msg = td->del_set.size();
  string buff;
  char res[100];
  switch(td->status) {
    case trans:
      td->del_set.clear();
      buff = "+OK " + to_string(num_msg) + " messages unmarked.\r\n";
      strcpy(res, buff.c_str());
      send_back(*cfd, res, td);
    default:
      send_back(*cfd, bad_seq, td);
  }
}

/* This handler funciton will deal with incoming "LIST" command*/
void list_handler(thread_data* td, smatch matches, int* cfd) {
  string strnum = matches[1];
  string buff;
  int num;
  char res[1024];
  switch(td->status) {
    case trans:
      if (strnum == "") {
        buff = "+OK " + to_string(td->mails.size()) + " messages.\r\n";
        strcpy(res, buff.c_str());
        send_back(*cfd, res, td);
        for (int i = 0; i < td->mails.size(); i++) {
          buff = to_string(i+1) + " " + to_string(td->mails.at(i).length()) + "\r\n";
          strcpy(res, buff.c_str());
          send_back(*cfd, res, td);
        }
        send_back(*cfd, end_msg, td);
      } else {
        num = stoi(strnum);
        if (num > td->mails.size() || num <= 0) {
          send_back(*cfd, no_msg, td);
        } else {
          if (td->del_set.find(get_uid(td->mails.at(num - 1))) != td->del_set.end()) {
            send_back(*cfd, mail_gone, td);
          } else {
            buff = "+OK " + strnum + " " + to_string(td->mails.at(num - 1).length()) + "\r\n";
            strcpy(res, buff.c_str());
            send_back(*cfd,res, td);
          }
        }
      }
      break;
    default:
      send_back(*cfd, bad_seq, td);
  }
}

/*
This function will process the command and send corresponding messages
to the client.
If a "quit" command received, it returns true; otherwise, returns false.
 */
bool process_command(thread_data* td, string command) {
  int* cfd = td->cfd;
  if (td->vflag) fprintf(stderr, "[%d] C: %s\n", *cfd, command.substr(0, command.length() - 2).c_str());
  //Start matching the command's pattern
  smatch matches;
  if (regex_search(command, matches, user_reg)) {
    string input = matches[1];
    switch(td->status) {
      case init:
        if (has_user(input)) {
          send_back(*cfd, user_found, td);
          td->status = pass;
          td->login = input;
        } else {
          send_back(*cfd, user_nfound, td);
        }
        break;
      default:
        send_back(*cfd, bad_seq, td);
    }
  } else if (regex_search(command, matches, quit_reg)) {
    int count = 0;
    string buff;
    char res[100];
    switch(td->status) {
      case trans:
        freopen(NULL, "w", td->myfile);
        for (map<string, string>::iterator it=td->msg_mapping.begin(); it!=td->msg_mapping.end(); ++it) {
          if (td->del_set.find(it->first) == td->del_set.end()) {
            if (fputs(it->second.c_str(),td->myfile) == EOF) {
              send_back(*cfd, update_err, td);
              break;
            }
            count++;
          }
        }
        buff = "+OK dewey POP3 server signing off (" + to_string(count) + " messages left)\r\n";
        strcpy(res, buff.c_str());
        send_back(*cfd, res, td);
        fclose(td->myfile);
        return true;
      default:
        send_back(*cfd, farewell, td);
        fclose(td->myfile);
        return true;
    }
  } else if (regex_search(command, matches, pass_reg)) {
    string pw = matches[1];
    switch(td->status) {
      case pass:
        if (pw == "cis505") {
          if (open_maildrop(td)) {
            send_back(*cfd, pw_valid, td);
            parse_email(td);
            td->status = trans;
          } else {
            send_back(*cfd, file_notopen, td);
            td->status = init;
          }
        } else {
          td->status = init;
          send_back(*cfd, pw_invalid, td);
        }
        break;
      default:
        send_back(*cfd, bad_seq, td);
    }
  } else if(regex_search(command, matches, stat_reg)) {
    string stat_ok;
    char res[100];
    switch(td->status) {
      case trans:
        stat_ok = "+OK " + to_string(td->mails.size()) + " " + to_string(td->size) + "\r\n";
        strcpy(res, stat_ok.c_str());
        send_back(*cfd, res, td);
        break;
      default:
        send_back(*cfd, bad_seq, td);
    }
  } else if(regex_search(command, matches, uidl_reg)) {
    string strnum = matches[1];
    string buff;
    char res[100];
    int num;
    switch(td->status) {
      case trans:
        if (strnum == "") {
          for (int i = 0; i < td->mails.size(); i++) {
            buff = to_string(i+1) + " " + get_uid(td->mails.at(i)) + "\r\n";
            strcpy(res, buff.c_str());
            send_back(*cfd, res,td);
          }
        } else {
          num = stoi(strnum);
          if (num > td->mails.size() || num <= 0) {
            send_back(*cfd, no_msg, td);
          } else {
            buff = strnum + " " + get_uid(td->mails.at(num - 1)) + "\r\n";
            strcpy(res, buff.c_str());
            send_back(*cfd, res,td);
          }
        }
        break;
      default:
        send_back(*cfd, bad_seq, td);
    }
  } else if (regex_search(command, matches, dele_reg)) {
    string strnum = matches[1];
    int num;
    switch(td->status) {
      case trans:
        num = stoi(strnum);
        if (num > td->mails.size() || num <= 0) {
          send_back(*cfd, no_msg, td);
        } else {
          if (td->del_set.find(get_uid(td->mails.at(num - 1))) != td->del_set.end()) {
            send_back(*cfd, mail_gone, td);
          } else {
            td->del_set.insert(get_uid(td->mails.at(num - 1)));
            send_back(*cfd, mail_del, td);
          }
        }
        break;
      default:
        send_back(*cfd, bad_seq, td);
    }
  } else if (regex_search(command, matches, retr_reg)) {
    retr_handler(td, matches, cfd);
  } else if (regex_search(command, matches, list_reg)) {
    list_handler(td, matches, cfd);
  } else if (regex_search(command, matches, noop_reg)) {
    send_back(*cfd, noop, td);
  } else if (regex_search(command, matches, rset_reg)) {
    rset_handler(td, matches, cfd);
  } else {
    send_back(*cfd, err, td);
  }
  return false;
}
