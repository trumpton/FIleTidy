
//
// movedup.c
//
//	This program takes an input list file, which contains a sorted list of files, complete with
//	MS5Sum, Size, Folder Data and File Date, and identifies which files are duplicates.
//	It can then move, rename or delete the duplicates, and optionally create links or shortcuts
//	pointing the duplicate to the master.
//
//	15 September 2010
//

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <stdio.h>
#include <libgen.h>
#include <errno.h>
#include <getopt.h>
#include <unistd.h>

// Enable debug to prevent actual creations / removes
// #define DEBUG (1==1)
#define DEBUG (1==0)

// Charactreristics of input
// a50ad48e2dee46a01b8ce424fce8cd07:000000512904 000000000000:001289039680 Books/BBC/November 2010.zip
// 000000000011111111112222222222333333333344444 4444455555555556666666667
// 012345678901234567890123456789012345678901234 6789012345678901234567890

#define ENDMD5SZ 45
#define ENDDATES 71
#define MAXPATH 32767
#define MAXLINE MAXPATH

// Boolean definition
#define bool int
#define true (1==1)
#define false (1==0)

int mkpath(const char *s, mode_t mode){
	char *q, *r = NULL, *path = NULL, *up = NULL;
	int rv;

	rv = -1;
	if (strcmp(s, ".") == 0 || strcmp(s, "/") == 0) return (0);
	if ((path = strdup(s)) == NULL) exit(1);
	if ((q = strdup(s)) == NULL) exit(1);
	if ((r = dirname(q)) == NULL) goto out;
	if ((up = strdup(r)) == NULL) exit(1);
	if ((mkpath(up, mode) == -1) && (errno != EEXIST)) goto out;
	if ((mkdir(path, mode) == -1) && (errno != EEXIST)) rv = -1;
	else rv = 0;
out:
	if (up != NULL) free(up);
	free(q);
	free(path);
	return (rv);
}

//
// relativepath - Generates a relative path to sourcefile
//
char *relativepath(char *source, char *target, char *relativetarget)
{
	relativetarget[0]='\0' ;

	// Compare paths and work out relative reference
	int matchingslash=0 ;
	for (int m=0; source[m]!='\0' && source[m]==target[m]; m++) {
		if (source[m]=='/') matchingslash=m+1 ;
	}

	// Invalid if both match completely, or one doesn't contain a filename
	if (source[matchingslash]=='\0' || target[matchingslash]=='\0') { return NULL ; }

	// Add ../../ etc. to output
	for (int i=matchingslash; source[i]!='\0'; i++) {
		if (source[i]=='/') strcat(relativetarget, "../") ;
	}

	// Concattenate path
	strcat(relativetarget, &target[matchingslash]) ;

	return relativetarget ;
}

//
// symlink
//
int createlink(char *target, char *linkfile)
{
	int status ;
	char rpath[MAXPATH] ;
	if (relativepath(linkfile, target, rpath)!=NULL) {
		// Create symbolic link
		return symlink(rpath, linkfile) ;
	} else {
		// Tried to link to self
		errno=EEXIST ;
		return -1 ;
	}
}


//
// shortcut
//
int shortcut(char *target, char *shortcutfile)
{
	char rpath[MAXPATH] ;
	if (relativepath(target, shortcutfile, rpath)!=NULL) {

		errno=EACCES ;
		return -1 ;

	} else {

		errno=EEXIST ;
		return -1 ;

	}
}

int main(int argc, char *argv[])
{

	////////////////////////////////////////////////////
	//
	// Process command line options
	//

	int c ;
	bool dodelete = false ;
	bool domove = false ;
	char *outputfolder=NULL ;
	bool dorename = false ;
	bool createsoftlink = false ;
	bool createshortcut = false ;
	char *lstfile=NULL ;	
		
	while ((c = getopt (argc, argv, "DM:Rhls")) != -1) {
		switch(c) {
			case 'D':
				dodelete=true ;
				break ;
			case 'M':
				domove=true ;
				outputfolder=optarg ;
				break ;
			case 'R':
				dorename=true ;
				break ;
			case 'l':
				createsoftlink=true ;
				break ;
			case 's':
				createshortcut=true ;
				break ;
			case 'h':
				printf("movedup [ -h ] [ -D | -M outputfolder | -R ] [ -l ] [ -s ] listfile\n") ;
				printf("  -D - Delete duplicates\n") ;
				printf("  -M - Move duplicates to outputfolder\n") ;
				printf("  -R - Rename duplicates\n") ;
				printf("  -h - Help\n") ;
				printf("  -l - Create soft link\n") ;
				printf("  -s - Create windows shortcut\n") ;
				return 1 ;
				break ;
			case '?':
				if (optopt=='M') {
					fprintf(stderr, "movedup: -M requires folder\n") ;
				} else {
					fprintf(stderr, "movedup: unrecognised option -%c\n", optopt) ;
				}
				return 1 ;
				break ;
		}
	}

	// Get list file name
	if (optind==argc) {
		fprintf(stderr, "movedup: listfile not specified.  movedup -h for help\n") ;
		return 1 ;
	} else {
		lstfile = argv[optind] ;
	}


	////////////////////////////////////////////////////
	//
	// Open Source File and Create Output Folder
	//

	FILE *fp ; 

	fp=fopen(lstfile,"r") ;
	if (fp==NULL) {
		printf("movedup: unable to open %s\n", lstfile) ;
		return 1 ;
	}

	if (domove) mkpath(outputfolder, S_IRWXU) ;

	////////////////////////////////////////////////////
	//
	// Parse LstFile
	//

	char masterpath[MAXPATH] ;
	char lastsum[ENDMD5SZ] ;

	lastsum[0]='\0' ;
	while (!feof(fp)) {

		char line[MAXLINE] ;

		if (fgets(line, sizeof(line)-1, fp)!=NULL) {

			char fpath1[MAXPATH], fpath2[MAXPATH] ;
			char *fpath, *ffolder, *fname ;

			for (int i=0; line[i]!='\0'; i++) {
				if (line[i]=='\r' || line[i]=='\n') line[i]='\0' ;
			}

			line[ENDMD5SZ]='\0' ;
			line[ENDDATES]='\0' ;

			fpath = &line[ENDDATES+1] ;
			strcpy(fpath1, fpath) ;
			fname = basename(fpath1) ;
			strcpy(fpath2, fpath) ;
			ffolder = dirname(fpath2) ;

			if (strcmp(lastsum, line)!=0) {

				// This entry is not a duplicate (save path)

				printf("%s\n", fpath) ;
				strcpy(masterpath, fpath) ;

			} else {

				// This entry is a duplicate to move/rename/delete

				bool success=false ;

				if (domove) {

					// Moving duplicates

					char target[MAXPATH] ;
					strcpy(target, outputfolder) ;
					strcat(target, "/") ;
					strcat(target, fpath) ;

					if (DEBUG || mkpath(target, S_IRWXU)<0) {

						fprintf(stderr, "movedup: Error creating: %s\n", target) ;

					} else {

						// Move the file
						strcat(target, "/") ;
						strcat(target, fname) ;
						if (DEBUG || rename(fpath, target)<0) {
							fprintf(stderr, "movedup: Error moving %s to %s (%s)\n", fpath, target, strerror(errno)) ;
						} else {
							success=true ;
						}
					}

				} else if (dorename) {

					// Renaming duplicates

					char target[MAXPATH] ;
					strcpy(target, ffolder) ;
					strcat(target, "/____") ;
					strcat(target, fname) ;
					if (DEBUG || rename(fpath, target)<0) {
						fprintf(stderr, "movedup: Error renaming %s to %s (%s)\n", fpath, target, strerror(errno)) ;
					} else {
						success=true ;
					}

				} else if (dodelete) {

					// Removing duplicates

					if (DEBUG || unlink(fpath)<0) {
						fprintf(stderr, "movedup: Error removing %s (%s)\n", fpath, strerror(errno)) ;
					} else {
						success=true ;
					}

				} else {

					// Just reporting (and possibly linking)

					success=true ;

				}

				if (success) {
					printf("%s *duplicateof* %s\n", fpath, masterpath) ;
				}


				////////////////////////////////////////////////////
				//
				// Now create links as requested
				//

 
					if (createsoftlink) {

						if (DEBUG || createlink(masterpath, fpath)<0) {
							fprintf(stderr, "movedup: Error creating soft link from %s to %s (%s)\n", 
								fpath, masterpath, strerror(errno)) ;
						}
					}

					if (createshortcut) {

						if (DEBUG || shortcut(masterpath, fpath)<0) {
							if (DEBUG || errno!=EEXIST) {
								fprintf(stderr, "movedup: Error creating shortcut from %s to %s (%s)\n", 
									fpath, masterpath, strerror(errno)) ;
							}
						}
					}

			}
		}

		strcpy(lastsum, line) ;			
	}

	fclose(fp) ;
	return 0 ;
}
