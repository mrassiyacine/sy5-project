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
#include "tar.h"
#include "errno.h"
#include "time.h"
#define BUFSIZE 1000000

/* VARIABLES GLOBALES */

char *PATH_BUF = NULL;
char *REAL_PATH = NULL;
int in_tar = 1;
char *tar_in_name = NULL;
char *TAR_PATH = NULL;

/* FONCTIONS "AUXILIAIRES" (À METTRE DANS "aux.c") */

void initialize_globals() {
  PATH_BUF = malloc(sizeof(char)*PATH_MAX);
  REAL_PATH = getcwd(PATH_BUF, PATH_MAX);
  TAR_PATH = malloc(sizeof(char)*BUFSIZE);
}

int is_tar(char *name) {
  if(strstr(name + 1, ".tar")!=NULL) return 0;
  else return 1;
}

int is_path_tar(char *path) {
  if(strstr(path + 1, ".tar")!=NULL || strstr(path, ".tar/")!=NULL) return 0;
  else return 1;
}

char **make_cmd(char *str) {
  char **res=malloc(sizeof(char *) * 100);
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
  char **res=malloc(sizeof(char *) * 100);
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
  char **tmp = make_path(path);
  char *res = malloc(sizeof(char)*100);
  res = tmp[strarraylen(tmp)-1];
  return res;
}

/* COMMANDES PAS EXÉCUTABLES PAR EXEC */

int cd(char *dir_or_tar) {
  DIR * dir1 = opendir(".");
  if(dir1 == NULL) exit(1);
  struct stat s;
  int error = 0;
  
  while(1) {
    struct dirent *d = readdir(dir1);
    if(d == NULL) break;

    /* 1ER CAS : ON NE SE TROUVE PAS DANS UN TARBALL */
    if(strcmp(d -> d_name, dir_or_tar)==0 && in_tar == 1) {
      
      /* 2ÈME CAS : ON VEUT ALLER DANS UN TARBALL */
      if(is_tar(d -> d_name)==0) {
	char add_to_path[20] = "/"; //À CHANGER
	strcat(add_to_path, d->d_name);
	strcat(REAL_PATH, add_to_path);

	tar_in_name = d-> d_name;
	in_tar = 0;
	error = 1;
	closedir(dir1);
	return 0;;
      }
      
      if(stat(d -> d_name, &s)==0) {
	if(S_ISDIR(s.st_mode)) {
	  
	  chdir(dir_or_tar);
	  REAL_PATH = getcwd(PATH_BUF, PATH_MAX);
	  error = 1;
	  closedir(dir1);
	  return 0;
	}
      }
    }
    
    /*5ÈME CAS : ON SE TROUVE DANS UN TAR ET ON REVIENT EN ARRIÈRE */
    else if(in_tar == 0 && strcmp(dir_or_tar, "..")==0) {
      size_t i = strlen(REAL_PATH)-1;
      while(1) {
	if(REAL_PATH[i] == '/') { REAL_PATH[i] = '\0'; break; }
	else i--;
      }

      if(is_path_tar(REAL_PATH)==1) { in_tar = 1; TAR_PATH[0] = '\0'; }

      if(in_tar==0) {
	size_t j = strlen(TAR_PATH)-2;
	while(1) {
	  if(TAR_PATH[j] == '/' || strlen(TAR_PATH)==0) break;
	  TAR_PATH[j] = '\0';
	  j--;
	}
      }

      
      error = 1;
      closedir(dir1);
      return 0;
    }

    /* 4ÈME CAS : ON SE TROUVE DANS UN TARBALL ET ON VEUT SE DÉPLACER DANS LE RÉPERTOIRE COURANT */
    else if(in_tar == 0 && strcmp(dir_or_tar, ".")==0) {
      
      error = 1;
      closedir(dir1);
      return 0;
      }

    /*3ÈME CAS : ON SE TROUVE DANS UN TAR ET ON NE REVIENT PAS EN ARRIÈRE */
    else if(in_tar == 0) {
      
      int o = open(tar_in_name, O_RDONLY);
      
      strcat(TAR_PATH, dir_or_tar);
      strcat(TAR_PATH, "/");
  
      while(1) {
	struct posix_header h;

	ssize_t lu = read(o, &h, 512);
	if(lu <= 0) {
	  size_t k = strlen(TAR_PATH)-2;
	  while(1) {
	    if(TAR_PATH[k] == '/' || strlen(TAR_PATH)==0) break;
	    TAR_PATH[k] = '\0';
	    k--;
	  }
	  break;
	}

	size_t size;
	sscanf(h.size, "%lo", &size);
	
	if(strcmp(h.name, TAR_PATH)==0) {
	  char add_to_path[20] = "/"; //À CHANGER
	  strcat(add_to_path, dir_or_tar);
	  strcat(REAL_PATH, add_to_path);

	  error = 1;
	  closedir(dir1);
	  break;
	}
	else lseek(o, ((size + BLOCKSIZE -1) >> BLOCKBITS )*512, SEEK_CUR);
      }
      if(error == 1) {
	close(o);
	return 0;
      }
    }
  }
  if(error == 0) { write(1, "cd: Aucun fichier ou dossier de ce type\n", 40); closedir(dir1); return -1;}
  return -1;
}
  
void cd_aux(char **path) {
  int cpt = 0;
  int k = 0;
  char **tab = malloc(sizeof(char *)*100);
  
  for(int i=0; i<strarraylen(path); i++) {
    
    if(strcmp(path[i], "..")==0){
      char *real_path_cpy = malloc(sizeof(char)*BUFSIZE);
      strcpy(real_path_cpy, REAL_PATH);
      cpt--;
 
      char *last_path = getLastPath(real_path_cpy);
      tab[k] = malloc(sizeof(char) *100);
      strcpy(tab[k], last_path);
      k++;
    }
    
    if(cd(path[i])==-1) {
      for(int l=k-1; l>=0; l--) cd(tab[l]);
      for(int j=0; j<cpt; j++) cd("..");
      break;
    }
    cpt++;
  }
}

void pwd() {
  int w1 = write(1, REAL_PATH, strlen(REAL_PATH));
  if(w1 == -1) { perror("write"); exit(EXIT_FAILURE); }
  int w2 = write(1, "\n", 1);
  if(w2 == -1) { perror("write"); exit(EXIT_FAILURE); }
}

/* COMMANDES EXÉCUTABLES PAR EXEC */

void execution(char **cmd) {
  if(strcmp(cmd[0],"cd")==0) cd_aux(make_path(cmd[1])); //si on n'est pas dans un tar et quon fait cd
  else if(strcmp(cmd[0], "pwd")==0) pwd(); //si on n'est pas dans un tar et quon fait cd
  else {
    int f, w;

    f = fork();
    switch(f) {
      
    case -1:
      perror("fork");
      exit(EXIT_FAILURE);
    case 0:
      if(execvp(cmd[0], cmd)==-1) perror(cmd[0]);
      exit(EXIT_SUCCESS);
    default:
      wait(&w);
    }
  }
}

/* COMMANDES EN MODE TAR */

void cat_tar(char **files) {

  if(strarraylen(files) == 1) {
    execution(files);
  }
  else {
    int o = open(tar_in_name, O_RDONLY);
    int error = 0;
    
    for(int i=1; i<strarraylen(files); i++) {
      lseek(o, 0, SEEK_SET);
      while(1) {
	struct posix_header h;
	char res[BUFSIZE];
	
	ssize_t lu = read(o, &h, 512);
	if(lu <= 0) break;
	
	size_t size;
	sscanf(h.size, "%lo", &size);
	
	if(strcmp(h.name, files[i])==0) {
	  read(o, res, ((size +512 -1)/512)*512);
	  int w1 = write(1, res, ((size +512 -1)/512)*512);
	  if(w1 == -1) { perror("write"); exit(EXIT_FAILURE); }
	  
	  error = 1;
	  close(o);
	  break;
	}
	else lseek(o, ((size + BLOCKSIZE -1) >> BLOCKBITS )*512, SEEK_CUR);
      }
    }
    if(error == 0) {
      int w2 = write(1, "cat : Aucun fichier ou dossier de ce type\n", 44);
      if(w2 == -1) { perror("write"); exit(EXIT_FAILURE); }
      close(o);
    }
  }
}

void ls_tar(char **file_or_option) {
  int o = open(tar_in_name, O_RDONLY);
  if(o == -1) exit(1);
    
  if(strlen(TAR_PATH)==0 && strarraylen(file_or_option)==1) {
    while(1) {
      size_t size;
      struct posix_header h;
      memset(&h, 0, 512);
      
      int lu = read(o, &h, 512);
      if(lu < 1) break;
      
      sscanf(h.size, "%lo", &size);
      printf("%s ", h.name);
      
      lseek(o, ((size + BLOCKSIZE -1) >> BLOCKBITS )*512, SEEK_CUR);
    }
    printf("\n");
    close(o);
  }
}

void ls_l_tar(char **file_or_option) {

  int o = open(tar_in_name, O_RDONLY);
  if(o == -1) exit(1);
  if(strarraylen(file_or_option) == 2) {
    while(1) {
    size_t size;
    struct posix_header h;
    memset(&h, 0, 512);
    
    int lu = read(o, &h, 512);
    if(strcmp(h.magic, "\0\0\0\0\0\0")==0) break;
    if(lu < 1) break;
    
    sscanf(h.size, "%lo", &size);

    printf("%s ", h.mode); //À CHANGER (write)
    printf("%s ", h.uid); //À CHANGER (write)
    printf("%s ", h.gid); //À CHANGER (write)
    printf("%ld ", size); //À CHANGER (write)
    printf("%s ", h.name); //À CHANGER (write)
    printf("\n"); //À CHANGER (write)
	   
    
    lseek(o, ((size + BLOCKSIZE -1) >> BLOCKBITS )*512, SEEK_CUR);
    }
    printf("\n");
    close(o);
  }
}

void cp_tar(char *file1, char *file2) {
  int o = open(tar_in_name, O_RDWR);
  int error = 0;

  while(1) {
      struct posix_header h;
      memset(&h, 0, 512);
      char res[BUFSIZE];

      ssize_t lu = read(o, &h, 512);
      if(lu <= 0) break;

      size_t size = sscanf(h.size, "%lo", &size);

      if(strcmp(h.name, file1)==0) {
	read(o, res, ((size +512 -1)/512)*512);
	struct posix_header h2 = h;

	strncpy(h2.name, file2, 100); // changement du nom
	sprintf(h2.size, "%011o", 512);

	lseek(o, 0, SEEK_END);
	int w1 = write(o, &h2, 512);
	if(w1 == -1) { perror("write"); exit(EXIT_FAILURE); }
	int w2 = write(o, res, ((size +512 -1)/512)*512);
	if(w2 == -1) { perror("write"); exit(EXIT_FAILURE); }

	error = 1;
	close(o);
	break;
      }
      else lseek(o, ((size + BLOCKSIZE -1) >> BLOCKBITS )*512, SEEK_CUR);
  }
  if(error == 0) {
    int w3 = write(1, "cat: Aucun fichier ou dossier de ce type\n", 41);
    if(w3 == -1) { perror("write"); exit(EXIT_FAILURE); }
    close(o);  
  }
}

void execution_tar(char **cmd) {
  if(strcmp(cmd[0], "cat")==0) cat_tar(cmd);
  else if(strcmp(cmd[0], "ls")==0) ls_tar(cmd);
  else if(strcmp(cmd[0], "cp")==0) cp_tar(cmd[1], cmd[2]);
  else if(strcmp(cmd[0], "cd")==0) cd_aux(make_path(cmd[1]));
  else if(strcmp(cmd[0], "pwd")==0) pwd();
  else execution(cmd);
}
	

int main() {

  /*Le main contient une boucle infinie qui lit l'entrée standard et agit selon ce qui a été lu. Elle se temrine seulement quand il n'y a plus rien à lire (aka CTRL+D) */
  
  char **cmd = NULL; //pr la commande de l'appel à execution()
  ssize_t lu=0;

  initialize_globals(); //initialisation des variables globales
  
  while(1) {
    size_t t = strlen(REAL_PATH);
    write(1, REAL_PATH, t+1);
    write(1, " \x1b[35m*prompt* \x1b[0m ", 19);
    
    char *buf = NULL;
    buf = malloc(sizeof(char) * BUFSIZE); //buffer pour le read
    
    lu=read(0, buf, BUFSIZE);
    if(lu<=0) break;
    
    char *tmp2 = malloc(sizeof(char)*(int)lu); //pr y mettre ce qui a été lu
    for(int i=0; i<lu-1; i++) tmp2[i]=buf[i];

    cmd=make_cmd(tmp2); //on stocke la commande dans un tableau de str

    //puis on agit selon le cas :
    if(in_tar == 0) execution_tar(cmd); //si on est dans un tar
    else execution(cmd);

    tmp2 = NULL;
    free(tmp2);
    free(buf);
  }

  free(REAL_PATH);
  free(TAR_PATH);

  return 0;
}
