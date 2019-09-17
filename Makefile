
all: movedup md5sumd

install: movedup md5sumd duplicatefiletidy
	/bin/cp movedup md5sumd duplicatefiletidy ~/bin

movedup: movedup.c
	gcc movedup.c -o movedup

md5sumd: md5sumd.c
	gcc md5sumd.c -lcrypto -o md5sumd

