## Introduction
- A multithreaded web server with complete functionalities in sending, receiving and deleting mails, 
  fully compatible with Thunderbird mail application.

- Designed a dispatcher/worker system and created corresponding modules for POP3 and SMTP protocols.  
1. POP3 and SMTP share the __cmdline.h__ file.
2. POP3 uses the following modules: **thread_func_pop3.h**, **process_pop3.h**, **process_pop3mail.h**
3. SMTP uses the following modules: **thread_func.h**, **process_smtp.h**, **process_mail.h**

- Utilized Linux programming interface to support debug mode, mailbox path & port assigning options.
