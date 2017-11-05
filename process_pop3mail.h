#include <dirent.h>
using namespace std;
using namespace regex_constants;

extern char* directory;
extern vector<string> maillist;
extern map<string, string> users;

void computeDigest(char *data, int dataLengthBytes, unsigned char *digestBuffer);

/*
Data Structure for each thread.
*/
typedef struct thread_data {
  int vflag;
  int* cfd;
  int status, length;
  string login;
  FILE* myfile;
  map<string, string> mail_mapping;
  map<string, string> msg_mapping;
  vector<string> mails;
  vector<string> msgs;
  set<string> del_set;
} thread_data;

/*
Get unique id for a specific email
*/
string get_uid(string cur_mail) {
  unsigned char buff[100];
  char data[2000];
  char md5[MD5_DIGEST_LENGTH * 2];
  strcpy(data, cur_mail.c_str());
  computeDigest(data, cur_mail.length(), buff);
  for (int i = 0; i < MD5_DIGEST_LENGTH; i++) {
    sprintf(&md5[i * 2], "%02X", buff[i]);
  }
  return string(md5);
}

/*
Parse the mailbox for a user and store all relevant info in thread_data
*/
void parse_email(thread_data* td) {
  string mailbox;
  char buffer[100];
  unsigned char buff[100];
  char data[2000];
  char md5[MD5_DIGEST_LENGTH * 2];
  while(fgets(buffer , 100 , td->myfile) != NULL) {
    mailbox += string(buffer);
  }
  int found = 0;
  int pre = -1;
  int start;
  while ((found = mailbox.find("From <", found)) != -1){
    if (pre >= 0) {
    start = mailbox.find("\r\n", pre) + 2;
    string cur_mail = mailbox.substr(pre, found);
    string cur_msg = mailbox.substr(start, found);
    td->mails.push_back(cur_mail);
    td->msgs.push_back(cur_msg);
    td->length += cur_msg.length();
    }
    pre = found;
    found++;
  }
  if (pre >= 0) {
    start = mailbox.find("\r\n", pre) + 2;
    string cur_mail = mailbox.substr(pre);
    string cur_msg = mailbox.substr(start);
    td->mails.push_back(cur_mail);
    td->msgs.push_back(cur_msg);
    td->length += cur_msg.length();
  }
  for (int i = 0; i < td->mails.size(); i++) {
    string cur_mail = td->mails.at(i);
    string cur_msg = td->msgs.at(i);
    strcpy(data, cur_mail.c_str());
    computeDigest(data, cur_mail.length(), buff);
    for (int i = 0; i < MD5_DIGEST_LENGTH; i++) {
      sprintf(&md5[i * 2], "%02X", buff[i]);
    }
    td->mail_mapping[string(md5)] = cur_mail;
    td->msg_mapping[string(md5)] = cur_msg;
  }
}

/*
Return true if the user exists; otherwise return false;
*/
bool has_user(string input) {
  map<string, string>::iterator it;
  it = users.find(input);
  if (it == users.end()) return false;
  return true;
}

/*
Return true if file open successes;otherwise return false;
*/
bool open_maildrop(thread_data* td) {
  string target("./");
  target.append(directory);
  target = target + "/" + users.at(td->login);
  td->myfile = fopen(target.c_str(), "r+");
  if (td->myfile == NULL) {
    cerr << "File not opened." << endl;
    return false;
  }
  return true;
}

/*
Get the whole mail list from the mbox directory
*/
void get_mailinglist(vector<string>& list, map<string, string>& users) {
  regex mbox (".+.mbox$", ECMAScript | icase );
  smatch matches;
  string buff("./");
  buff.append(directory);
  DIR *dir = opendir(buff.c_str());
  if(dir)
  {
      struct dirent *ent;
      while((ent = readdir(dir)) != NULL)
      {
          string cur_dir(ent->d_name);
          if(regex_search(cur_dir, matches, mbox)) {
            list.push_back(cur_dir);
            string cur_user = cur_dir.substr(0, cur_dir.find("."));
            users[cur_user] = cur_dir;
          }
      }
  }
}

/*
Retrieve a string with current datetime
*/
string get_datetime() {
  time_t rawtime;
  struct tm * timeinfo;
  char buffer [80];
  time (&rawtime);
  timeinfo = localtime (&rawtime);
  strftime (buffer,80,"%a %h %d %T %Y",timeinfo);
  string res(buffer);
  return res;
}

/*
Check if a specific mail is marked as "deleted"
*/
bool is_deleted(string mail, thread_data* td) {
  string uid = get_uid(mail);
  if(td->del_set.find(uid) == td->del_set.end()) return false;
  return true;
}
