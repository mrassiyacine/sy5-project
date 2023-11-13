#include "tsh.h"

char **CMD = NULL;
char *PATH_BUF = NULL;
char *REAL_PATH = NULL;
int IN_TAR = 1;
char *TAR_IN_NAME = NULL;
char *TAR_PATH = NULL;
char *BUF = NULL;

void initialize_globals() {
  PATH_BUF = malloc(sizeof(char)*PATH_MAX);
  REAL_PATH = getcwd(PATH_BUF, PATH_MAX);
  TAR_PATH = malloc(sizeof(char)*BUFSIZE);
  TAR_IN_NAME = malloc(sizeof(char)*PATH_MAX);
}

void arrayfree(char **array) {
  int i=0;
  while(array[i]!=NULL) { array[i]=NULL; free(array[i]);}
  array=NULL;
  free(array);
}

int is_tar(char *name) {
    int len = strlen(name);

    if (len < 4 || strcmp(name + len - 4, ".tar")!=0) return 1;
    else return 0;
}

int is_path(char *name) {
  if(strstr(name, "/")!=NULL || strcmp(name, "..")==0 || strcmp(name, ".")==0) return 0;
  else return 1;
}

int contains_tar(char *name) {
  if(strstr(name, ".tar")!=NULL) return 0;
  else return 1;
}

int is_path_tar(char *path) {
  if(strstr(path + 1, ".tar")!=NULL || strstr(path, ".tar/")!=NULL) return 0;
  else return 1;
}

int is_cmd_tar(char *cmd) {

  if(IN_TAR == 0 || strstr(cmd, ".tar")!=NULL) return 0;
  else return 1;
  
}

char **make_cmd(char *str) {
  char **res=malloc(sizeof(char *) * PATHSIZE);
  int cpt = 0;
  char *args = strtok(str, " ");
  
  while(args !=NULL) {
    res[cpt] = malloc(strlen(args) + 1);
      strcpy(res[cpt], args);
    args=strtok(NULL, " ");
    cpt++;
  }
  res[cpt] = NULL;
  return res;
}

char **make_path(char *str) {
  char **res=malloc(sizeof(char *) * PATHSIZE);
  int cpt = 0;
  char *args = strtok(str, "/");
  
  while(args !=NULL) {
    res[cpt] = malloc(strlen(args) + 1);
    strcpy(res[cpt], args);
    args=strtok(NULL, "/");
    cpt++;
  }
  res[cpt] = NULL;
  return res;
}

int strarraylen(char **strarray) {
  int res = 0;
  while(strarray[res] != NULL) res++;

  return res;
}

char *getLastPath(char *path) {
  char *path_cpy = malloc(sizeof(char) * strlen(path));
  strcpy(path_cpy, path);
  
  char **tmp = make_path(path_cpy);
  char *res = malloc(sizeof(char)*strlen(tmp[strarraylen(tmp)-1]));
  res = tmp[strarraylen(tmp)-1];
  arrayfree(tmp);
  free(path_cpy);
  return res;
}

char *getCpy(char *str) {
  char *str_cpy = malloc(sizeof(char) * strlen(str));
  strcpy(str_cpy, str);
  
  return str_cpy;
}

char *octToRights(char *octal) {
  char *res = malloc(sizeof(char)*9);;

  for(int i=4; i<7; i++) {
    if(octal[i] == 0) strcat(res, "---");
    else if(octal[i] == '1') strcat(res, "--x");
    else if(octal[i] == '2') strcat(res, "-w-");
    else if(octal[i] == '3') strcat(res, "-wx");
    else if(octal[i] == '4') strcat(res, "r--");
    else if(octal[i] == '5') strcat(res, "r-x");
    else if(octal[i] == '6') strcat(res, "rw-");
    else if(octal[i] == '7') strcat(res, "rwx");
  }
    return res;
}

char *UTCtime(char *time) {
  int tmp = atoi(time);
  time_t pTime = tmp;
  char *res = malloc(sizeof(char) * 20);
  
  write(1, ctime(&pTime), strlen(ctime(&pTime)));

  struct tm *t = gmtime(&pTime);

  if(t->tm_mon == 0) strcpy(res, "jan.");
  else if(t->tm_mon == 1) strcpy(res, "fev.");
  else if(t->tm_mon == 2) strcpy(res, "mar.");
  else if(t->tm_mon == 3) strcpy(res, "avr.");
  else if(t->tm_mon == 4) strcpy(res, "mai");
  else if(t->tm_mon == 5) strcpy(res, "juin");
  else if(t->tm_mon == 6) strcpy(res, "juil.");
  else if(t->tm_mon == 7) strcpy(res, "août.");
  else if(t->tm_mon == 8) strcpy(res, "sept.");
  else if(t->tm_mon == 9) strcpy(res, "oct.");
  else if(t->tm_mon == 10) strcpy(res, "nov.");
  else if(t->tm_mon == 11) strcpy(res, "dec.");

  strcat(res, "  ");

  char day[2];
  sprintf(day, "%d", t->tm_mday);
  strcat(res, day);

  strcat(res, "  ");

  char hour[2];
  sprintf(hour, "%d", t->tm_hour);
  strcat(res, hour);

  strcat(res, ":");

  char min[2];
  sprintf(min, "%d", t->tm_min);
  strcat(res, min);
  
  return res;
}

int get_links(char *tar, char *name) {
  int links = 1;
  char **tmp = NULL;
  int o = open(tar, O_RDONLY);
  
  while(1) {
    struct posix_header h;
    memset(&h, 0, BLOCKSIZE);
      
    ssize_t lu = read(o, &h, BLOCKSIZE);
    if(lu <= 0) break;
    
    size_t size;
    sscanf(h.size, "%lo", &size);

    if(strstr(h.name, name)!=NULL) {
      if(h.name[strlen(h.name)-1] == '/') {
	tmp = make_path(h.name);
	for(int i = 0; i<strarraylen(tmp); i++) {
	  if((strcmp(tmp[i], name)==0) && ((i == strarraylen(tmp)-2) || (i == strarraylen(tmp)-1))) { links++; break; }
	}
      }
    }
    lseek(o, ((size + BLOCKSIZE -1) >> BLOCKBITS )*512, SEEK_CUR);
  }
  if(tmp != NULL) arrayfree(tmp);
  close(o);
  return links;
}

char *is_redir_or_pipe(char *str) {
  for(int i = 0; i<strlen(str); i++) {
    if(str[i] == '|') return "|";
    else if(str[i] == '<') return "<";
    else if(str[i] == '>') return ">";
    else if(str[i] == '2') {
      if(str[i+1] == '>') return "2>";
    }
  }
  return NULL;
}

char **make_redir_or_pipe(char *cmd, char *sep) {
  char **res=malloc(sizeof(char *) * PATHSIZE);
  int cpt = 0;

  if(strcmp(sep, "2>")==0) {
    
    char *needle = strstr(cmd, " 2> ");
    
    for(size_t i = 0; i<sizeof(" 2> ") - 1; i++) 
      {
	*needle++ = '\0';
      }
      
      res[0] = malloc(strlen(cmd) + 1);
      strcpy(res[0], cmd);
      res[1] = malloc(strlen(needle) + 1);
      strcpy(res[1], needle);
      res[2] = NULL;
  }
 
  else {
    char *args = strtok(cmd, sep);
    while(args !=NULL) {
      res[cpt] = malloc(strlen(args) + 1);
      strcpy(res[cpt], args);
      args=strtok(NULL, " ");
      cpt++;
    }
    res[cpt] = NULL;
  }
  return res;
}
