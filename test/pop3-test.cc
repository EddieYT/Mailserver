#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>

#include "test.h"

int main(int argc, char *argv[])
{
  if (argc != 2)
    panic("Syntax: %s <port>", argv[0]);

  // Initialize the buffers

  struct connection conn1;
  initializeBuffers(&conn1, 5000);

  // Open a connection and exchange greeting messages

  connectToPort(&conn1, atoi(argv[1]));
  expectToRead(&conn1, "+OK*");
  expectNoMoreData(&conn1);

  // Log into the account

  writeString(&conn1, "USER linhphan\r\n");
  expectToRead(&conn1, "+OK*");
  expectNoMoreData(&conn1);

  writeString(&conn1, "PASS cis505\r\n");
  expectToRead(&conn1, "+OK*");
  expectNoMoreData(&conn1);

  // Check available messages

  writeString(&conn1, "STAT\r\n");
  expectToRead(&conn1, "+OK 1 264");
  expectNoMoreData(&conn1);

  // Try a nonexistent command

  writeString(&conn1, "BLAH\r\n");
  expectToRead(&conn1, "-ERR*");
  expectNoMoreData(&conn1);

  // Retrieve the one message the SMTP tester has sent

  writeString(&conn1, "RETR 1\r\n");
  expectToRead(&conn1, "+OK*");
  expectToRead(&conn1, "From: Benjamin Franklin <benjamin.franklin@localhost>");
  expectToRead(&conn1, "To: Linh Thi Xuan Phan <linhphan@localhost>");
  expectToRead(&conn1, "Date: Fri, 21 Oct 2016 18:29:11 -0400");
  expectToRead(&conn1, "Subject: Testing my new email account");
  expectToRead(&conn1, "");
  expectToRead(&conn1, "Linh,");
  expectToRead(&conn1, "");
  expectToRead(&conn1, "I just wanted to see whether my new email account works.");
  expectToRead(&conn1, "");
  expectToRead(&conn1, "        - Ben");
  expectToRead(&conn1, ".");
  expectNoMoreData(&conn1);

  // Delete the message

  writeString(&conn1, "DELE 1\r\n");
  expectToRead(&conn1, "+OK*");
  expectNoMoreData(&conn1);

  // Close the connection

  writeString(&conn1, "QUIT\r\n");
  expectToRead(&conn1, "+OK*");
  expectRemoteClose(&conn1);
  closeConnection(&conn1);

  freeBuffers(&conn1);
  return 0;
}