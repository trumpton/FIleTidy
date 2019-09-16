//
// gcc f.c -lcrypto -o f
//

#include <sys/types.h>
#include <sys/stat.h>
#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include <openssl/md5.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <time.h>

char * md5sum(char *file)
{
  static char md5res[MD5_DIGEST_LENGTH*2+1] ;
  int n;
  MD5_CTX c;
  char buf[512];
  ssize_t bytes;
  unsigned char out[MD5_DIGEST_LENGTH];

  // Clear output response
  md5res[0]='\0' ;
  for (n=0; n<MD5_DIGEST_LENGTH; n++) strcat(md5res, "xx") ;

  int fh ;
  fh = open(file, O_RDONLY) ;

  if (fh>=0) {

    // Calculate MD5Sum
    MD5_Init(&c);
    bytes=read(fh, buf, 512);
    while(bytes > 0)
    {
      MD5_Update(&c, buf, bytes);
      bytes=read(fh, buf, 512);
    }
    MD5_Final(out, &c);
    close(fh) ;

    // Generate output string
    for(n=0; n<MD5_DIGEST_LENGTH; n++) {
      sprintf((char *)md5res+(n*2),"%02x", (int)out[n]) ;
    }

  }

  return md5res ;

}


int main(int argc, char *argv[])
{
	if (! (argc==2 || (argc==3 && strcmp(argv[1],"-f")==0) ) ) {
		printf("md5sumd [ -f ] filename\n") ;
		printf("  -f - Include folder in timestamp\n") ;
		printf("outputs:md5sum:filesize timestamp filename\n") ;
		return 1 ;
	}

	// Get Filename, Foldername and parameters

	int incfolder = (1==0) ;

	if (argc==3 && strcmp(argv[1],"-f")==0) incfolder=(1==1) ;

	char *filename = argv[argc-1] ;

	char folder[65535] ;
	strcpy(folder, filename) ;
	int i ;
	for (i=strlen(folder); i>0 && folder[i]!='/'; i--) ;
	if (i==0) {
		strcpy(folder,".") ;
	} else {
		folder[i]='\0' ;
	}

	struct stat buf ;
	struct stat folderbuf ;

	if (stat(filename, &buf)!=0) {
		printf("md5sumd: Error accessing %s\n", argv[1]) ;
		return 1 ;
	}

	if (incfolder) {
		if (stat(folder, &folderbuf)!=0) {
			printf("md5sumd: Error accessing foler %s\n", folder) ;
			return 1 ;
		}
	}

	printf("%s:%012ld %012ld:%012ld %s\n",
		md5sum(filename),
		buf.st_size,
		(incfolder?folderbuf.st_mtime:(long int)0),
		buf.st_mtime,
		filename) ;
	
	return 0 ;
}
