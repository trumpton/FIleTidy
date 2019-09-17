
all: movedup md5sumd

movedup: movedup.c
	gcc movedup.c -o movedup

md5sumd: md5sumd.c
	gcc md5sumd.c -lcrypto -o md5sumd

