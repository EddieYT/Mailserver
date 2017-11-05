## Introduction
- A multithreaded web server with complete functionalities in sending, receiving and deleting mails, 
  fully compatible with Thunderbird mail application.

- Designed a dispatcher/worker system and created corresponding modules for POP3 and SMTP protocols.  
1. POP3 and SMTP share the __cmdline.h__ file.
2. POP3 uses the following modules: **thread_func_pop3.h**, **process_pop3.h**, **process_pop3mail.h**
3. SMTP uses the following modules: **thread_func.h**, **process_smtp.h**, **process_mail.h**

- Utilized Linux programming interface to support debug mode, mailbox path & port assigning options.

## How to run this program?
### SMTP
#### There are several command line options: ./smpt <your directory containing users' mailbox>
1. -v Debug mode: This mode will show debug messages for you.
2. -p <port #> Assign a port: You can assign a port for the server.
- HELO <domain>: which starts a connection;
- MAIL FROM: tells the server who the sender of the email is;
- RCPT TO: specifies the recipient;
- DATA: starts inputing the content of email
- QUIT: terminates the connection;
- RSET: aborts a mail transaction; and
- NOOP: does nothing.

### POP3
#### There are several command line options: ./pop3 <your directory containing users' mailbox>
1. -v Debug mode: This mode will show debug messages for you.
2. -p <port #> Assign a port: You can assign a port for the server.

- USER: tells the server which user is logging in;
- PASS: specifies the user's password;
- STAT: returns the number of messages and the size of the mailbox;
- UIDL: shows a list of messages, along with a unique ID for each message;
- RETR: retrieves a particular message;
- DELE: deletes a message;
- QUIT: terminates the connection;
- LIST: shows the size of a particular message, or all the messages;
- RSET: undeletes all the messages that have been deleted with DELE; and
- NOOP: does nothing.
