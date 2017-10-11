#include <stdlib.h>
#include <stdio.h>
#include <openssl/md5.h>

void computeDigest(char *data, int dataLengthBytes, unsigned char *digestBuffer)
{
  /* The digest will be written to digestBuffer, which must be at least MD5_DIGEST_LENGTH bytes long */

  MD5_CTX c;
  MD5_Init(&c);
  MD5_Update(&c, data, dataLengthBytes);
  MD5_Final(digestBuffer, &c);
}

int main(int argc, char *argv[])
{

  /* Your code here */

  return 0;
}
