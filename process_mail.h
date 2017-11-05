#include <dirent.h>
#include <fstream>
#include <map>
using namespace std;
using namespace regex_constants;

extern char* directory;
extern vector<string> maillist;
extern map<string, string> users;

typedef struct thread_data {
  int vflag;
  int* cfd;
  int status;
  string sender;
  string content;
  vector<string> receivers;
} thread_data;

/*
Return true if the user exists in mbox, otherwise return false;
*/
bool has_user(string input) {
  map<string, string>::iterator it;
  it = users.find(input);
  if (it == users.end()) return false;
  return true;
}

/*
Send data and update target mbox.
*/
int send_mail(thread_data* td) {
  vector<string>& cur = td->receivers;
  ofstream myfile;
  for (int i = 0; i < cur.size(); i++) {
    string cur_mail = cur.at(i);
    int found = cur_mail.find("@localhost");
    cur_mail.replace(cur_mail.begin() + found, cur_mail.end(), ".mbox");
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
