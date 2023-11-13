#ifndef test3
#define test3

int cat_tar(char *file);

void cat_tar_aux(char **cmd);

int cp_tar(char *file1, char *file2, char *dest, int errorMsg);

int cp_tar_aux(char **cmd, int errorMsg);

void names();

void ls_tar(int option);

void ls_tar_aux(char **cmd);

int is_directory(char *name);

int rm_tar(char *file, int errorMsg);

void rm_tar_aux(char **cmd, int errorMsg2);

int rmdir_tar(char *file, int option);

void rmdir_tar_aux(char **cmd, int option);

int mkdir_tar(char *name);

void mkdir_tar_aux(char **names);

int cp_tar_option(char **dirs);

void mv_tar(char **cmd);

void execution_tar(char **cmd);

#endif
