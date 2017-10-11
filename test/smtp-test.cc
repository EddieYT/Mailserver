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
  expectToRead(&conn1, "220 localhost *");
  expectNoMoreData(&conn1);

  writeString(&conn1, "HELO tester\r\n");
  expectToRead(&conn1, "250 localhost");
  expectNoMoreData(&conn1);

  // Specify the sender and the receipient (with one incorrect recipient)

  writeString(&conn1, "MAIL FROM:<benjamin.franklin@localhost>\r\n");
  expectToRead(&conn1, "250 OK");
  expectNoMoreData(&conn1);

  writeString(&conn1, "RCPT TO:<linhphan@localhost>\r\n");
  expectToRead(&conn1, "250 OK");
  expectNoMoreData(&conn1);

  writeString(&conn1, "RCPT TO:<nonexistent.mailbox@localhost>\r\n");
  expectToRead(&conn1, "550 *");
  expectNoMoreData(&conn1);

  // Send the actual data

  writeString(&conn1, "DATA\r\n");
  expectToRead(&conn1, "354 *");
  expectNoMoreData(&conn1);

  writeString(&conn1, "From: Benjamin Franklin <benjamin.franklin@localhost>\r\n");
  writeString(&conn1, "To: Linh Thi Xuan Phan <linhphan@localhost>\r\n");
  writeString(&conn1, "Date: Fri, 21 Oct 2016 18:29:11 -0400\r\n");
  writeString(&conn1, "Subject: Testing my new email account\r\n");
  writeString(&conn1, "\r\n");
  writeString(&conn1, "Linh,\r\n");
  writeString(&conn1, "\r\n");
  writeString(&conn1, "I just wanted to see whether my new email account works.\r\n");
  writeString(&conn1, "\r\n");
  writeString(&conn1, "        - Ben\r\n");
  expectNoMoreData(&conn1);
  writeString(&conn1, ".\r\n");
  expectToRead(&conn1, "250 OK");
  expectNoMoreData(&conn1);

  // Close the connection

  writeString(&conn1, "QUIT\r\n");
  expectToRead(&conn1, "221 *");
  expectRemoteClose(&conn1);
  closeConnection(&conn1);

  freeBuffers(&conn1);
  return 0;
}