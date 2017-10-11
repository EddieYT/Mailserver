#ifndef __test_h__
#define __test_h__

#define panic(a...) do { fprintf(stderr, a); fprintf(stderr, "\n"); exit(1); } while (0) 

// For each connection we keep a) its file descriptor, and b) a buffer that contains
// any data we have read from the connection but not yet processed. This is necessary
// because sometimes the server might send more bytes than we immediately expect.

struct connection {
  int fd;
  char *buf;
  int bytesInBuffer;
  int bufferSizeBytes;
};

void log(const char *prefix, const char *data, int len, const char *suffix);
void writeString(struct connection *conn, const char *data);
void expectNoMoreData(struct connection *conn);
void connectToPort(struct connection *conn, int portno);
void expectToRead(struct connection *conn, const char *data);
void expectRemoteClose(struct connection *conn);
void initializeBuffers(struct connection *conn, int bufferSizeBytes);
void closeConnection(struct connection *conn);
void freeBuffers(struct connection *conn);

#endif /* defined(__test_h__) */