#include <dirent.h>
#include <fstream>
using namespace std;
using namespace regex_constants;

extern char* directory;

typedef struct thread_data {
  int vflag;
  int* cfd;
  int status;
  string sender;
  string content;
  vector<string> receivers;
  vector<string> maillist;
} thread_data;

int send_mail(thread_data* td) {
  vector<string>& cur = td->receivers;
  ofstream myfile;
  for (int i = 0; i < cur.size(); i++) {
    string cur_mail = cur.at(i);
    int found = cur_mail.find("@localhost");
    cur_mail.replace(cur_mail.begin() + found, cur_mail.end(), ".mbox");
    cout << cur_mail << endl;
    // Open file and append content to the end
    string target("./");
    target.append(directory);
    target = target + "/" + cur_mail;
    myfile.open(target, ofstream::out | ofstream::app);
    if (!myfile.is_open()) {
      cerr << "File not opened." << endl;
    }
    myfile << td->content;
    myfile.close();
  }
  return 1;
}

void get_mailinglist(thread_data* td) {
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
          string cur(ent->d_name);
          if(regex_search(cur, matches, mbox)) {
            td->maillist.push_back(cur);
            cout << cur << endl;
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

bool in_maillist(thread_data* td, string receiver) {
  int found = receiver.find("@localhost");
  string target = receiver.substr(0, found) + ".mbox";
  vector<string>& list = td->maillist;
  bool res = false;
  for (int i = 0; i < list.size(); i++) {
    if (list.at(i) == target) {
      res = true;
      break;
    }
  }
  return res;
}
