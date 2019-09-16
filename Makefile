
all: movedup md5sumd mslink

movedup: movedup.c
	gcc movedup.c -o movedup

md5sumd: md5sumd.c
	gcc md5sumd.c -lcrypto -o md5sumd

mslink: mslink.c
	gcc mslink.c -o mslink

