#include "tsh.h"
#include "cmd_tar.h"

int cd(char *dir_or_tar) {
  DIR * dir1 = opendir(".");
  if(dir1 == NULL) exit(1);
  struct stat s;
  
  while(1) {
    struct dirent *d = readdir(dir1);
    if(d == NULL) break;

    /* 1ER CAS : ON NE SE TROUVE PAS DANS UN TARBALL */
    if(strcmp(d -> d_name, dir_or_tar)==0 && IN_TAR == 1) {
      
      /* 2ÈME CAS : ON VEUT ALLER DANS UN TARBALL */
      if(is_tar(d -> d_name)==0) {
	char add_to_path[PATHSIZE] = "/";
	strcat(add_to_path, d->d_name);
	strcat(REAL_PATH, add_to_path);
	
        strcpy(TAR_IN_NAME, d->d_name);
	IN_TAR = 0;
	closedir(dir1);
	return 0;
      }
      
      if(stat(d -> d_name, &s)==0) {
	if(S_ISDIR(s.st_mode)) {
	  
	  chdir(dir_or_tar);
	  REAL_PATH = getcwd(PATH_BUF, PATH_MAX);
	  closedir(dir1);
	  return 0;
	}
      }
    }
    
    /*5ÈME CAS : ON SE TROUVE DANS UN TAR ET ON REVIENT EN ARRIÈRE */
    else if(IN_TAR == 0 && strcmp(dir_or_tar, "..")==0) {
      size_t i = strlen(REAL_PATH)-1;
      while(1) {
	if(REAL_PATH[i] == '/') { REAL_PATH[i] = '\0'; break; }
	else i--;
      }

      if(is_path_tar(REAL_PATH)==1) { IN_TAR = 1; TAR_PATH[0] = '\0'; }

      if(IN_TAR==0) {
	size_t j = strlen(TAR_PATH)-2;
	while(1) {
	  if(TAR_PATH[j] == '/' || strlen(TAR_PATH)==0) break;
	  TAR_PATH[j] = '\0';
	  j--;
	}
      }

      
      closedir(dir1);
      return 0;
    }

    /* 4ÈME CAS : ON SE TROUVE DANS UN TARBALL ET ON VEUT SE DÉPLACER DANS LE RÉPERTOIRE COURANT */
    else if(IN_TAR == 0 && strcmp(dir_or_tar, ".")==0) {
      
      closedir(dir1);
      return 0;
      }

    /*3ÈME CAS : ON SE TROUVE DANS UN TAR ET ON NE REVIENT PAS EN ARRIÈRE */
    else if(IN_TAR == 0) {

      int o = open(TAR_IN_NAME, O_RDONLY);
      
      strcat(TAR_PATH, dir_or_tar);
      strcat(TAR_PATH, "/");
  
      while(1) {
	
	struct posix_header h;
	
	ssize_t lu = read(o, &h, BLOCKSIZE);
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
	  if(h.typeflag != '1') {
	    char add_to_path[PATHSIZE] = "/";
	    strcat(add_to_path, dir_or_tar);
	    strcat(REAL_PATH, add_to_path);
	    
	    closedir(dir1);
	    close(o);
	    
	    return 0;
	  }
	  else return -1;
	}
	else lseek(o, ((size + BLOCKSIZE -1) >> BLOCKBITS )*512, SEEK_CUR);
      }
    }
  }
  closedir(dir1);
  return -1;
}

char **dir_exists(char **path) {

  int k = 0;
  char **tab = malloc(sizeof(char *) * PATHSIZE);
  
  for(int i=0; i<strarraylen(path); i++) {
    
    // copie défensive du chemin : 3 cas
    if(strcmp(path[i], "..")==0) { // 1er cas : on va faire cd(".."). Pour faire le chemin inverse on doit sauvegarder le répertoire dans lequel on se trouve avant de faire cd("..")

      char *last_path = getLastPath(REAL_PATH);
      tab[k] = malloc(sizeof(char) * strlen(last_path));
      strcpy(tab[k], last_path);
      k++;
      cd("..");
      
      last_path = NULL;
      free(last_path);
    }
    else if(strcmp(path[i], ".")==0) { // 2ème cas : on sauvegarde "." dans le chemin inversé. Si on ne traitait pas ce cas on aurait un bug car i on avait "." dans le chemin il serait traité dans le 'else' qui suit et ajouterai ".." au chemin inversé
      tab[k] = malloc(sizeof(char) *1);
      strcpy(tab[k], ".");
      k++;
      cd(".");
    }
    else { // 3ème cas : il ne reste qu'à traiter le cas où on a un nom de répertoire; on stocke alors ".." dans le chemin inversé pour pouvoir revenir en arrière
      tab[k] = malloc(sizeof(char) *2);
      strcpy(tab[k], "..");
      k++;
      if(cd(path[i])==-1) {
	for(int j=k-2; j>=0; j--) cd(tab[j]);
	return NULL;
      }
    }
  }
  for(int l=k-1; l>=0; l--) cd(tab[l]);
  return tab;

}

char **get_rev_path() {
  char **rev_path = malloc(sizeof(char *) * PATHSIZE);
  int k = 0;

  while(strcmp(REAL_PATH, "/")!=0) {
    char *last_path = getLastPath(REAL_PATH);
    rev_path[k] = malloc(sizeof(char) * strlen(last_path));
    strcpy(rev_path[k], last_path);
    k++;
    cd("..");

    last_path = NULL;
    free(last_path);
  }
  return rev_path;
  
}
  
int cd_aux(char **path, int errorMsg) {

  /* Cette fonction va appeler 'cd()' mais va considérer le cas où l'utilisateur rentre en argument un chemin au lieu d'un simple nom de répertoire. Elle va aussi gérer le cas où l'utilisateur a passé en argument un chemin erroné. Ce qu'on fait : dans une boucle de la taille de l'argument (qui est make_path(cmd[1])), pour chaque élément de ce dernier on va faire 'cd()'. En parallèle, on va faire une copie défensive du chamin que l'on est en train de parcourir et s'il arrive à un moment dans le chemin que 'cd()' échoue, on reviendra sur nos pas */

  if(path[0] == NULL) {
    while(strcmp(REAL_PATH, "/")!=0) {
      cd("..");
    }
  }

  else if(strcmp(path[0], "~")==0) {
    char **rev_path = get_rev_path();
    for(int i=1; i<strarraylen(path); i++) {
      if(cd(path[i])==-1) {
	for(int j=strarraylen(rev_path)-1; j>=0; j--) cd(rev_path[j]);
	return -1;
      }
    }
  }

  else if(dir_exists(path)==NULL) {
    if(errorMsg == 0) write(2, "cd: Aucun fichier ou dossier de ce type\n", 40);
    return -1;
  }
  else {
    for(int i=0; i<strarraylen(path); i++) cd(path[i]);
  }
  return 0;
}

void pwd() {
  int w1 = write(1, REAL_PATH, strlen(REAL_PATH));
  if(w1 == -1) { perror("write"); exit(EXIT_FAILURE); }
  int w2 = write(1, "\n", 1);
  if(w2 == -1) { perror("write"); exit(EXIT_FAILURE); }
}
  
void execution(char **cmd) {
  if(strcmp(cmd[0], "cd")==0) { //si on n'est pas dans un tar et qu'on fait cd
    char **tmp = make_path(cmd[1]);
    cd_aux(tmp, 0);
    arrayfree(tmp);
  }
  else if(strcmp(cmd[0], "pwd")==0) pwd(); //si on n'est pas dans un tar et qu'on fait pwd
  else if(strcmp(cmd[0], "exit")==0) exit(EXIT_SUCCESS);
  else {
    int f, w;

    f = fork();
    switch(f) {
      
    case -1 :
      perror("fork");
      exit(EXIT_FAILURE);
    case 0 :
      if(execvp(cmd[0], cmd)==-1) perror(cmd[0]);
      exit(EXIT_SUCCESS);
    default :
      wait(&w);
    }
  }
}

void redirection_or_pipe(char **redir, char *sep) {
  char *cmd_cpy = malloc(sizeof(char) * strlen(redir[0]));
  strcpy(cmd_cpy, redir[0]);
  char **cmd = make_cmd(cmd_cpy); // on transforme en commande le premier élément qui est censé être une commande
  if(strcmp(sep, "|")==0) {
    char **cmd2 = make_cmd(redir[1]);
    int fd[2];
    if(pipe(fd)==-1) {
      perror("pipe");
      exit(1);
    }

    int f2;
    int f = fork();
    switch(f) {
      
    case -1 :
      perror("fork pipe");
      exit(1);
    case 0 :
      close(fd[0]);
	 dup2(fd[1], STDOUT_FILENO);
	 close(fd[1]);
	 if(execvp(cmd[0], cmd)==-1) perror(cmd[0]);
       
    default :
      f2 = fork();;
      switch(f2) {
      case -1 :
	perror("fork pipe 2");
	exit(1);
      case 0 :
	close(fd[1]);
	dup2(fd[0], STDIN_FILENO);
	close(fd[0]);
	if(execvp(cmd2[0], cmd2)==-1) perror(cmd2[0]);

       default :
	 wait(NULL);
      }
    }
    arrayfree(cmd);
    arrayfree(cmd2);
    return;
  }

  else {

    int STDOUT, STDIN, STDERR, d;
    
    int is_fic = (is_path(redir[1])==0) ? 1 : 0;

    char *file = NULL;
    char **rev = NULL;
    char **path = NULL;
    
    if(is_fic == 0) {
      file = malloc(sizeof(char) * strlen(redir[1]));
      strcpy(file, redir[1]);
    }
    else {
      path = make_path(redir[1]);
      file = malloc(sizeof(char) * strlen(path[strarraylen(path)-1]));
      strcpy(file, path[strarraylen(path)-1]);
      path[strarraylen(path)-1] = NULL;

      rev = dir_exists(path);
      cd_aux(path, 1);
    }
	
	
    if(IN_TAR == 1) {
      int o = open(file, O_RDWR | O_CREAT);
      if(o<0) {write(1, "Aucun fichier de ce type\n", 26); }
      
      if(strcmp(sep, "<")==0 || strcmp(sep, "<<")==0) {
        STDIN = dup(0);
        d = dup2(o, 0);
	
	if(is_cmd_tar(redir[0]) == 0) execution_tar(cmd);
	else execution(cmd);
	
	dup2(STDIN, d);
      }
      else if(strcmp(sep, ">")==0 || strcmp(sep, ">>")==0) {
        STDOUT = dup(1);
        d = dup2(o, 1);

	if(is_cmd_tar(redir[0]) == 0) { execution_tar(cmd);}
	else { execution(cmd); }
	
	dup2(STDOUT, d);
	
      }
      else if(strcmp(sep, "2>")==0 || strcmp(sep, "2>>")==0) {
        STDERR = dup(2);
        d = dup2(o, 2);

	if(is_cmd_tar(redir[0]) == 0) execution_tar(cmd);
	else execution(cmd);
	
	dup2(STDERR, d);
      }
    }

    else {
      int o = open(TAR_IN_NAME, O_RDWR);
      if(o<0) {write(1, "Aucun fichier de ce type\n", 26); }
      while(1) {
	struct posix_header h;
	memset(&h, 0, BLOCKSIZE);
	
	ssize_t lu = read(o, &h, BLOCKSIZE);
	if(lu <= 0) break;
        
	size_t size;
	sscanf(h.size, "%lo", &size);

	char tmp[PATHSIZE] = "";
	strcat(tmp, TAR_PATH);
	strcat(tmp, file);
	
	if(strcmp(h.name, tmp)==0) {
	  for(int i=strarraylen(path)-1; i>=0; i--) cd(rev[i]);
	  arrayfree(rev);
	  arrayfree(path);
	  
	  
	  if(strcmp(sep, "<")==0 || strcmp(sep, "<<")==0) {
	    STDIN = dup(0);
	    d = dup2(o, 0);
	  }
	  else if(strcmp(sep, ">")==0 || strcmp(sep, ">>")==0) {
	    STDOUT = dup(1);
	    d = dup2(o, 1);
	  }
	  else if(strcmp(sep, "2>")==0 || strcmp(sep, "2>>")==0) {
	    STDERR = dup(2);
	    d = dup2(o, 2);
	  }

	  if(is_cmd_tar(redir[0]) == 0) execution_tar(cmd);
	  else execution(cmd);

	  if(strcmp(sep, "<")==0 || strcmp(sep, "<<")==0) dup2(STDIN, d);
	  else if(strcmp(sep, ">")==0 || strcmp(sep, ">>")==0) dup2(STDOUT, d);
	  else if(strcmp(sep, "2>")==0 || strcmp(sep, "2>>")==0) dup2(STDERR, d);

	  close(o);
	  break;
	}
	else lseek(o, ((size + BLOCKSIZE -1) >> BLOCKBITS )*512, SEEK_CUR);
      }
    }
    cmd_cpy = NULL;
    free(cmd_cpy);
    file = NULL;
    free(file);
    arrayfree(cmd);
  }
}
