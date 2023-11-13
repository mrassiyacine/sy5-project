#ifndef test2
#define test2

int cd(char *dir_or_tar);

char **reverse_path(char **path);

char **dir_exists(char **path);

int cd_aux(char **path, int errorMsg);

void pwd();

void execution(char **cmd);

void redirection_or_pipe(char **redir, char *sep);

#endif
