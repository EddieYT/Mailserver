#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <ctype.h>
#include <fcntl.h>
#include <string.h>
#include <errno.h>
#include <sys/socket.h>
#include <arpa/inet.h>

#include "test.h"

// This function prints data we sent to the server or received from it. The main 
// difference to a simple printf() is that this function replaces non-printable
// characters (say, a zero byte) with <0xAB>, where 0xAB is the ASCII code of 
// the non-printable character. (It also replaces a CR with <CR> and a LF with <LF>.)
// Without this, it would be hard to see, for instance, if the server sends back
// an extra zero byte

void log(const char *prefix, const char *data, int len, const char *suffix)
{
  printf("%s", prefix);
  for (int i=0; i<len; i++) {
    if (data[i] == '\n')
      printf("<LF>");
    else if (data[i] == '\r') 
      printf("<CR>");
    else if (isprint(data[i])) 
      printf("%c", data[i]);
    else 
      printf("<0x%02X>", (unsigned int)(unsigned char)data[i]);
  }
  printf("%s", suffix);
}

// This function writes a string to a connection. If a LF is required,
// it must be part of the 'data' argument. (The idea is that we might
// sometimes want to send 'partial' lines to see how the server handles these.)

void writeString(struct connection *conn, const char *data)
{
  int len = strlen(data);
  log("C: ", data, len, "\n");

  int wptr = 0;
  while (wptr < len) {
    int w = write(conn->fd, &data[wptr], len-wptr);
    if (w<0)
      panic("Cannot write to conncetion (%s)", strerror(errno));
    if (w==0)
      panic("Connection closed unexpectedly");
    wptr += w;
  }
}

// This function verifies that the server has sent us more data at this point.
// It does this by temporarily putting the socket into nonblocking mode and then
// attempting a read, which (if there is no data) should return EAGAIN. 
// Note that some of the server's data might still be 'in flight', so it is best
// to call this only after a certain delay.

void expectNoMoreData(struct connection *conn)
{
  int flags = fcntl(conn->fd, F_GETFL, 0);
  fcntl(conn->fd, F_SETFL, flags | O_NONBLOCK);
  int r = read(conn->fd, &conn->buf[conn->bytesInBuffer], conn->bufferSizeBytes - conn->bytesInBuffer);

  if ((r<0) && (errno != EAGAIN))
    panic("Read from connection failed (%s)", strerror(errno));

  if (r>0)
    conn->bytesInBuffer += r;

  if (conn->bytesInBuffer > 0) {
    log("S: ", conn->buf, conn->bytesInBuffer, " [unexpected; server should not have sent anything!]\n");
    conn->bytesInBuffer = 0;
  }

  fcntl(conn->fd, F_SETFL, flags);
}

// Attempts to connect to a port on the local machine.

void connectToPort(struct connection *conn, int portno)
{
  conn->fd = socket(PF_INET, SOCK_STREAM, 0);
  if (conn->fd < 0) 
    panic("Cannot open socket (%s)", strerror(errno));

  struct sockaddr_in servaddr;
  bzero(&servaddr, sizeof(servaddr));
  servaddr.sin_family=AF_INET;
  servaddr.sin_port=htons(portno);
  inet_pton(AF_INET, "127.0.0.1", &(servaddr.sin_addr));

  if (connect(conn->fd, (struct sockaddr*)&servaddr, sizeof(servaddr))<0)
    panic("Cannot connect to localhost:10000 (%s)", strerror(errno));

  conn->bytesInBuffer = 0;
}

// Reads a line of text from the server (until it sees a LF) and then compares
// the line to the argument. The argument should not end with a LF; the function
// strips off any LF or CRLF from the incoming data before doing the comparison. 
// (This is to avoid assumptions about whether the server terminates its lines
// with a CRLF or with a LF.)

void expectToRead(struct connection *conn, const char *data)
{
  // Keep reading until we see a LF

  int lfpos = -1;
  while (true) {
    for (int i=0; i<conn->bytesInBuffer; i++) {
      if (conn->buf[i] == '\n') {
        lfpos = i;
        break;
      }
    }

    if (lfpos >= 0)
      break;

    if (conn->bytesInBuffer >= conn->bufferSizeBytes)
      panic("Read %d bytes, but no CRLF found", conn->bufferSizeBytes);

    int bytesRead = read(conn->fd, &conn->buf[conn->bytesInBuffer], conn->bufferSizeBytes - conn->bytesInBuffer);
    if (bytesRead < 0)
      panic("Read failed (%s)", strerror(errno));
    if (bytesRead == 0)
      panic("Connection closed unexpectedly");

    conn->bytesInBuffer += bytesRead;
  }

  log("S: ", conn->buf, lfpos+1, "");

  // Get rid of the LF (or, if it is preceded by a CR, of both the CR and the LF)

  bool crMissing = false;
  if ((lfpos==0) || (conn->buf[lfpos-1] != '\r')) {
    crMissing = true;
    conn->buf[lfpos] = 0;
  } else {
    conn->buf[lfpos-1] = 0;
  }

  // Check whether the server's actual response matches the expected response
  // Note: The expected response might end in a wildcard (*) in which case
  // the rest of the server's line is ignored.

  int argptr = 0, bufptr = 0;
  bool match = true;
  while (match && data[argptr]) {
    if (data[argptr] == '*') 
      break;
    if (data[argptr++] != conn->buf[bufptr++])
      match = false;
  }

  if (!data[argptr] && conn->buf[bufptr])
    match = false;

  // Annotate the output to indicate whether the response matched the expectation.

  if (match) {
    if (crMissing)
      printf(" [Terminated by LF, not by CRLF]\n");
    else 
      printf(" [OK]\n");
  } else {
    log(" [Expected: '", data, strlen(data), "']\n");
  }

  // 'Eat' the line we just parsed. However, keep in mind that there might still be
  // more bytes in the buffer (e.g., another line, or a part of one), so we have to
  // copy the rest of the buffer up.

  for (int i=lfpos+1; i<conn->bytesInBuffer; i++)
    conn->buf[i-(lfpos+1)] = conn->buf[i];
  conn->bytesInBuffer -= (lfpos+1);
}

// This function verifies that the remote end has closed the connection.

void expectRemoteClose(struct connection *conn)
{
  int r = read(conn->fd, &conn->buf[conn->bytesInBuffer], conn->bufferSizeBytes - conn->bytesInBuffer);
  if (r<0)
    panic("Read failed (%s)", strerror(errno));
  if (r>0) {
    log("S: ", conn->buf, r + conn->bytesInBuffer, " [unexpected; server should have closed the connection]\n");
    conn->bytesInBuffer = 0;
  }
}

// This function initializes the read buffer

void initializeBuffers(struct connection *conn, int bufferSizeBytes)
{
  conn->fd = -1;
  conn->bufferSizeBytes = bufferSizeBytes;
  conn->bytesInBuffer = 0;
  conn->buf = (char*)malloc(bufferSizeBytes);
  if (!conn->buf)
  	panic("Cannot allocate %d bytes for buffer", bufferSizeBytes);
}

// This function closes our local end of a connection

void closeConnection(struct connection *conn)
{
  close(conn->fd);
}

// This function frees the allocated read buffer

void freeBuffers(struct connection *conn)
{
  free(conn->buf);
  conn->buf = NULL;	
}

// The main function, which basically drives the test cases by calling the above
// functions. TODO: Add your own test cases here!

