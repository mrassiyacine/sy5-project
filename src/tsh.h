#ifndef test
#define test

#include "tar.h"

#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <dirent.h>
#include <sys/stat.h>
#include <linux/limits.h>
#include <fcntl.h>
#include "errno.h"
#include "time.h"
#include "signal.h"
#define BUFSIZE 1000
#define FILESIZE 100000
#define PATHSIZE 100

extern char **CMD;
extern char *PATH_BUF;
extern char *REAL_PATH;
extern int IN_TAR;
extern char *TAR_IN_NAME;
extern char *TAR_PATH;
extern char *BUF;


void initialize_globals();

void arrayfree(char **array);

int is_tar(char *name);

int is_path(char *name);

int contains_tar(char *name);

int is_path_tar(char *path);

int is_cmd_tar(char *cmd);

char **make_cmd(char *str);

char **make_path(char *str);

int strarraylen(char **strarray);

char *getLastPath(char *path);

char *getCpy(char *str);

char *octToRights(char *octal);

char *UTCtime(char *time);

int get_links(char *tar, char *name);

char *is_redir_or_pipe(char *name);

char **make_redir_or_pipe(char *cmd, char *sep);

#endif
