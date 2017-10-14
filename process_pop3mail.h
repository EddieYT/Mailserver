#include <dirent.h>
using namespace std;
using namespace regex_constants;

extern char* directory;
extern vector<string> maillist;
extern map<string, string> users;

void computeDigest(char *data, int dataLengthBytes, unsigned char *digestBuffer);

typedef struct thread_data {
  int vflag;
  int* cfd;
  int status, size;
  string login;
  FILE* myfile;
  map<string, string> msg_mapping;
  vector<string> mails;
  set<string> del_set;
} thread_data;

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
  int pre = 0;
  while ((found = mailbox.find("From <", found+1)) != -1) {
    string cur_mail = mailbox.substr(pre, found);
    td->mails.push_back(cur_mail);
    pre = found;
  }
  if (pre != 0) {
    td->mails.push_back(mailbox.substr(pre));
    td->size = mailbox.length();
  }
  for (int i = 0; i < td->mails.size(); i++) {
    string cur_mail = td->mails.at(i);
    strcpy(data, cur_mail.c_str());
    computeDigest(data, cur_mail.length(), buff);
    for (int i = 0; i < MD5_DIGEST_LENGTH; i++) {
      sprintf(&md5[i * 2], "%02X", buff[i]);
    }
    td->msg_mapping[string(md5)] = cur_mail;
  }
}

bool has_user(string input) {
  map<string, string>::iterator it;
  it = users.find(input);
  if (it == users.end()) return false;
  return true;
}

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
