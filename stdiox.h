/*
Name: Matthew Angelakos
Pledge: I pledge my honor that I have abided by the Stevens Honor System.
*/
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h> 
#include <errno.h> 
#include <string.h> 
#include <dirent.h> 
#include <sys/stat.h> 
#include <limits.h>

int fprintfx(char *, char, void *);
int fscanfx(char *, char, void *);
int clean();
