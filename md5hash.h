#ifndef _MD5HASH_H

#define _MD5HASH_H 1

#define MD5HASH_SIZE 16
#define MD5HASH_LEN (2 * MD5HASH_SIZE)

void md5hash(const void *buf, unsigned int len, char *asciihash);

#endif

