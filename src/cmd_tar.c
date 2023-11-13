#include "tsh.h"
#include "cmd.h"

int is_directory(char *name) {

  if(is_tar(name)==0) { //on vérifie si le nom n'est pas déjà un autre tarball (considéré comme un répertoire dans notre shell)
    DIR *dir1 = opendir(".");
    if(dir1 == NULL) exit(1);
    
    while(1) {
      struct dirent *d = readdir(dir1);
      if(d == NULL) break;
      
      if(strcmp(d -> d_name, name)==0) return 0;
    }
  }
  else {
    char **tmp=make_path(name);
    if(dir_exists(tmp)!=NULL) { arrayfree(tmp); return 0; }
    else { arrayfree(tmp); return 1; }
  }
  return -1;
}

int is_already_a_file(char *name) {

  if(IN_TAR==1) { //on vérifie si le nom n'est pas déjà un autre tarball (considéré comme un répertoire dans notre shell)
    DIR *dir1 = opendir(".");
    if(dir1 == NULL) exit(1);
    
    while(1) {
      struct dirent *d = readdir(dir1);
      if(d == NULL) break;
      
      if(strcmp(d -> d_name, name)==0) return 0;
    }
  }
  else {
    if(is_tar(name)==0) return 0;
    char tmp[PATHSIZE] = "";
    strcat(tmp, TAR_PATH);
    strcat(tmp, name);
    int o = open(TAR_IN_NAME, O_RDONLY);
    
    while(1) {
      struct posix_header h;
      memset(&h, 0, BLOCKSIZE);
      
      ssize_t lu = read(o, &h, BLOCKSIZE);
      if(lu <= 0) break;

      size_t size;
      sscanf(h.size, "%lo", &size);
      
      if(strcmp(h.name, tmp)==0) return 0;
      
      lseek(o, ((size + BLOCKSIZE -1) >> BLOCKBITS )*512, SEEK_CUR);
    }
  }
  return 1;
}



int cat_tar(char *file) {

  char *current_dir = getLastPath(REAL_PATH);
  
  int o = open(TAR_IN_NAME, O_RDONLY);
  
  while(1) {
    struct posix_header h;
    memset(&h, 0, BLOCKSIZE);
    char res[FILESIZE];
    
    ssize_t lu = read(o, &h, BLOCKSIZE);
    if(lu <= 0) break;
    
    size_t size;
    sscanf(h.size, "%lo", &size);

    char *cpy_name = getCpy(h.name);
    char **name = make_path(cpy_name); //on transforme le nom en tableau de chaînes de caractères
    
    if(is_tar(current_dir) == 0) {
       if(strcmp(h.name, file)==0) {
	 if(h.typeflag != '5') {
	   read(o, res, ((size +BLOCKSIZE -1)/BLOCKSIZE)*BLOCKSIZE);
	   int w1 = write(1, res, ((size +BLOCKSIZE -1)/BLOCKSIZE)*BLOCKSIZE);
	   if(w1 == -1) { perror("write"); exit(EXIT_FAILURE); }
	 
	   close(o);
	   arrayfree(name);
	   return 0;
	 }
       }
       else lseek(o, ((size + BLOCKSIZE -1) >> BLOCKBITS )*512, SEEK_CUR);
    }
    else {
      if(strstr(h.name, current_dir)!=NULL) {
	for(int i=0; i<strarraylen(name); i++) {
	  if(strarraylen(name)>1 && i != strarraylen(name) - 1) {
	    if(strcmp(name[i], current_dir)==0 && strcmp(name[i+1], file)==0) {
	      if(h.typeflag != '5') {
		read(o, res, ((size +BLOCKSIZE -1)/BLOCKSIZE)*BLOCKSIZE);
		int w1 = write(1, res, ((size +BLOCKSIZE -1)/BLOCKSIZE)*BLOCKSIZE);
		if(w1 == -1) { perror("write cat"); exit(EXIT_FAILURE); }
		
		close(o);
		arrayfree(name);
		return 0;
	      }
	      else {
		write(2, "cat: impossible de supprimer '", 30);
		write(2, name[i+1], strlen(name[i+1]));
		write(2, "': est un dossier\n", 19);
		return -1;
	      }
	    }
	  }
	}
      }
      lseek(o, ((size + BLOCKSIZE -1) >> BLOCKBITS )*512, SEEK_CUR); arrayfree(name);
    }
  }
  return -1;
}

void cat_tar_aux(char **cmd) {

  char **path = NULL;
  char *file = NULL;
  
  if(!(strarraylen(cmd) > 1)) { char *tmp[] = {"cat", NULL}; execution(tmp); } // s'il n'y a pas d'argument, on fait juste cat
  else {
    
    for(int i = 1; i<strarraylen(cmd); i++) {
      char *cpy = getCpy(cmd[i]);

      if(cpy[strlen(cpy)-1]=='/') cpy[strlen(cpy)-1] = '\0';
      
      if(strstr(cpy, "/")==NULL) {
	int y = cat_tar(cpy);
	if(y == -1) { write(2, "cat: ", 5); write(2, cmd[i], strlen(cmd[i])); write(2, ": Aucun fichier de ce type\n", 28); }
      }
      else {
	
	path = make_path(cpy);
	file = malloc(sizeof(char) * strlen(path[strarraylen(path)-1]));
	strcpy(file, path[strarraylen(path)-1]);
	path[strarraylen(path)-1] = NULL;
	
	char **rev = dir_exists(path);
	int x = cd_aux(path, 1);
	
	if(x != -1) { // si cd réussit
	  if(IN_TAR == 0) { // si on se trouve dans un tar après le cd
	    int y = cat_tar(file);
	    if(y == -1) { write(2, "cat: ", 5); write(2, cmd[i], strlen(cmd[i])); write(2, ": Aucun fichier de ce type\n", 28); }
	    
	    for(int i=strarraylen(path)-1; i>=0; i--) cd(rev[i]); //puis on revient là où on était

	  }
	  else { // si on ne se trouve pas dans un tar après le cd
	    char *tmp[] = {"cat", file, NULL};
	    execution(tmp); // on fait un exec de 'ls'
	    for(int i=strarraylen(path)-1; i>=0; i--) cd(rev[i]); // puis on revient là où on était
	  }
	}
	else {
	  write(2, "cat: ", 5);
	  write(2, cmd[i], strlen(cmd[i]));
	  write(2, ": Aucun fichier ou dossier de ce type\n", 39);
	}
	if(rev != NULL) arrayfree(rev);
	arrayfree(path);
      }
    }
  }
}


int cp_tar(char *file1, char *file2, char *dest, int errorMsg) {
  
  char res[FILESIZE];
  char *tmp = getCpy(file1);
  char *f = getLastPath(tmp);
  
  if(dest != NULL) {
    // COPIE DU CONTENU (ET DE L'ENTÊTE DANS LE CAS OÙ ON EST DANS UN TAR) : pour la copie on doit vérifier si le nom rentré par l'utilisateur est un chemin ou simple nom de fichier. On doit ensuite considérer deux sous-cas : le fichier qu'on copie est dans un tar ou non.
    int o = open(TAR_IN_NAME, O_RDONLY);
    struct posix_header h2;
    memset(&h2, 0, BLOCKSIZE);

    read(o, &h2, BLOCKSIZE);
    lseek(o, 0, SEEK_SET);
    
    if(strstr(file1, "/")==NULL) { // dans le cas où le nom du fichier à copier n'est pas un chemin, on est sûr que l'on est dans un tar (edit : c'est faux)

      // COPIE DE L'ENTÊTE ET DU CONTENU
      if(IN_TAR == 0) {
	char name_tmp[PATHSIZE] = "";
	strcat(name_tmp, TAR_PATH);
	strcat(name_tmp, file1);
	
	while(1) {
	  struct posix_header h;
	  memset(&h, 0, BLOCKSIZE);
	  
	  ssize_t lu = read(o, &h, BLOCKSIZE);
	  if(lu <= 0) return -1;
	  
	  size_t size;
	  sscanf(h.size, "%lo", &size);
	  
	  if(strcmp(h.name, name_tmp)==0) {
	    h2 = h; //copie de l'entête
	    read(o, res, ((size + BLOCKSIZE -1) >> BLOCKBITS )*512); //copie du contenu
	    
	    char new_name[PATHSIZE] = "";
	    
	    strcat(new_name, dest);
	    strcat(new_name, "/");
	    strcat(new_name, file1); //changement du nom
	    strcpy(h2.name, new_name);
	    
	    close(o);
	    break;
	  }
	  else lseek(o, ((size + BLOCKSIZE -1) >> BLOCKBITS )*512, SEEK_CUR);
	}
      }
      else { //les fichiers à copier ne sont pas dans un tar
        int o2 = open(file1, O_RDONLY);
	if(o2 == -1) {
	  if(errorMsg == 0) write(2, "3cp: Aucun fichier ou dossier de ce type\n", 42);
	  return -1; }
	read(o2, res, FILESIZE);
	memset(&h2.name, 0, PATHSIZE);
	strcpy(h2.name, file1);
	h2.typeflag = '0';
	memset(&h2.size, 0, 12);
	sprintf(h2.size, "%011lo", strlen(res));
	close(o2);
      }
    }
    else { // file1 est un chemin, on doit alors traiter les deux cas : on se trouve dans un tar après le cd sur le chemin, ou non
      char *file_cpy = getCpy(file1);
      char **path_file = make_path(file_cpy);
      f = NULL;
      f = malloc(sizeof(char)*strlen(path_file[strarraylen(path_file)-1]));
      strcpy(f, path_file[strarraylen(path_file)-1]);
      path_file[strarraylen(path_file)-1] = NULL;
	
      char **rev = dir_exists(path_file);
      cd_aux(path_file, 1);

      if(IN_TAR == 0) { //copie dans le cas où on est dans un tar

	char name_tmp[PATHSIZE] = "";
	strcat(name_tmp, TAR_PATH);
	strcat(name_tmp, f);
      
	while(1) {
	  struct posix_header h;
	  memset(&h, 0, BLOCKSIZE);
	  
	  ssize_t lu = read(o, &h, BLOCKSIZE);
	  if(lu <= 0) break;
	  
	  size_t size;
	  sscanf(h.size, "%lo", &size);
	
	  if(strcmp(h.name, name_tmp)==0) {
	    char new_name[] = "";
	    strcat(new_name, dest);
	    strcat(new_name, "/");
	    strcat(new_name, f);
	    
	    h2 = h; //copie de l'entête
	    read(o, res, ((size + BLOCKSIZE -1) >> BLOCKBITS )*512); //copie du contenu

	    memset(&h2.name, 0, PATHSIZE);
	    strcpy(h2.name, new_name); //changement du nom
	    
	    for(int j=strarraylen(path_file)-1; j>=0; j--) cd(rev[j]);
	    close(o);
	  }
	else lseek(o, ((size + BLOCKSIZE -1) >> BLOCKBITS )*512, SEEK_CUR);
	}
      }

      else { //copie dans le cas où on est pas dans un tar
        int o2 = open(f, O_RDONLY);
	if(o2 == -1) {
	  if(errorMsg == 0) write(2, "4cp: Aucun fichier ou dossier de ce type\n", 42);
	  return -1; }
	read(o2, res, FILESIZE);
	memset(&h2.name, 0, PATHSIZE);
	strcpy(h2.name, f);
	h2.typeflag = '0';
	memset(&h2.size, 0, 12);
	sprintf(h2.size, "%011lo", strlen(res));
	close(o2);

	for(int j=strarraylen(path_file)-1; j>=0; j--) cd(rev[j]);
      }
    }
    
    // "COLLAGE" DANS DEST
    char *dest_cpy = getCpy(dest);
    char **path_dest = make_path(dest_cpy);
    
    char **rev = dir_exists(path_dest);
    cd_aux(path_dest, 1);

    if(is_already_a_file(file1)==0) {
      if(errorMsg == 0) {
	write(2, "cp: impossible de copier le fichier «", 39);
	write(2, file1, strlen(file1));
	write(2, "»: Le fichier existe\n", 23);
      }
      for(int j=strarraylen(path_dest)-1; j>=0; j--) cd(rev[j]);
      return -1;
    }
    
    if(IN_TAR == 0) {
      int o3 = open(TAR_IN_NAME, O_RDWR);
      struct posix_header h;
      memset(&h, 0, BLOCKSIZE);
      
      while(1) {
	ssize_t lu2 = read(o3, &h, BLOCKSIZE);
	if(lu2 <= 0) break;
	
	size_t size_2;
	sscanf(h.size, "%lo", &size_2);
	
	if(h.typeflag == '\0') {
	  lseek(o3, -512, SEEK_CUR);
	  int w1 = write(o3, &h2, BLOCKSIZE);
	  if(w1 == -1) { perror("write"); exit(EXIT_FAILURE); }
	  int w2 = write(o3, res, strlen(res));
	  if(w2 == -1) { perror("write"); exit(EXIT_FAILURE); }
	  int nb_blocks_to_add = (strlen(res) / BLOCKSIZE) + 2;
	  for(int i=0; i<=nb_blocks_to_add; i++) write(o3, &h, BLOCKSIZE);
	  
     	  close(o3);
	  for(int j=strarraylen(path_dest)-1; j>=0; j--) cd(rev[j]);
	  arrayfree(rev);
	  return 0;
	}
	else lseek(o3, ((size_2 + BLOCKSIZE -1) >> BLOCKBITS )*512, SEEK_CUR);
      }
    }
    else {// si on est pas dans tar après le cd dest
      int o4 = open(f, O_RDWR|O_CREAT, 0666);
      write(o4, res, strlen(res));
      close(o4);
      for(int j=strarraylen(path_dest)-1; j>=0; j--) cd(rev[j]);
      arrayfree(rev);
      return 0;
    }
  }
  
  else { //si dest==NULL

    if(strstr(file1, "/")==NULL) {
      
      int o5 = open(TAR_IN_NAME, O_RDWR);
      
      while(1) {
	struct posix_header h;
	memset(&h, 0, BLOCKSIZE);
	char res[BUFSIZE];
	
	ssize_t lu = read(o5, &h, BLOCKSIZE);
	if(lu <= 0) break;
	
	size_t size;
	sscanf(h.size, "%lo", &size);
        
	char name_tmp[PATHSIZE] = "";
	
	strcat(name_tmp, TAR_PATH);
	strcat(name_tmp, file1);
	
	if(strcmp(h.name, name_tmp)==0) {
	  struct posix_header h2;
	  memset(&h2, 0, BLOCKSIZE);
	  
	  h2 = h; //copie de l'entête
	  read(o5, res, ((size + BLOCKSIZE -1) >> BLOCKBITS )*512); //copie du contenu
	  
	  char new_name[PATHSIZE] = "";

	  if(is_path(file2)==1) {
	    
	    strcat(new_name, TAR_PATH);
	    strcat(new_name, file2);

	    memset(&h2.name, 0, PATHSIZE);
	    strcpy(h2.name, new_name); //changement du nom
	    
	    while(1) {
	      ssize_t lu_2 = read(o5, &h, BLOCKSIZE);
	      if(lu_2 <= 0) break;
	      
	      size_t size_2;
	      sscanf(h.size, "%lo", &size_2);
	      
	      if(h.typeflag == '\0') {
		
		if(is_already_a_file(file2)==0) {
		  if(errorMsg == 0) {
		    write(2, "cp: impossible de copier le fichier «", 39);
		    write(2, new_name, strlen(new_name));
		    write(2, "»: Le fichier existe\n", 23);
		  }
		  return -1;
		}
		
		lseek(o5, -512, SEEK_CUR);
		int w1 = write(o5, &h2, BLOCKSIZE);
		if(w1 == -1) { perror("write"); exit(EXIT_FAILURE); }
		int w2 = write(o5, res, strlen(res));
		if(w2 == -1) { perror("write"); exit(EXIT_FAILURE); }
		int nb_blocks_to_add = (strlen(res) / BLOCKSIZE) + 2;
		for(int i=0; i<=nb_blocks_to_add; i++) write(o5, &h, BLOCKSIZE);

		close(o5);
		return 0;
	      }
	      else lseek(o5, ((size_2 + BLOCKSIZE -1) >> BLOCKBITS )*512, SEEK_CUR);
	    }
	    close(o5);
	  }
	  else {
	    
	    char *file_cpy_2 = getCpy(file2);
	    char **path_file_2 = make_path(file_cpy_2);
	    char *f2 = malloc(sizeof(char)*strlen(path_file_2[strarraylen(path_file_2)-1]));
	    strcpy(f2, path_file_2[strarraylen(path_file_2)-1]);
	    path_file_2[strarraylen(path_file_2)-1] = NULL;
	    
	    char **rev = dir_exists(path_file_2);
	    cd_aux(path_file_2, 1);
	    
	    strcat(new_name, TAR_PATH);
	    
	    char *tmp = getCpy(file2);
	    char *tmp2 = getLastPath(tmp);
	    strcat(new_name, tmp2);

	    memset(&h2.name, 0, PATHSIZE);
	    strcpy(h2.name, new_name); //changement du nom
	    
	    if(IN_TAR == 0) {
	      while(1) {
		ssize_t lu_3 = read(o5, &h, BLOCKSIZE);
		if(lu_3 <= 0) break;
		
		size_t size_3;
		sscanf(h.size, "%lo", &size_3);
		
		if(h.typeflag == '\0') {
		  
		  if(is_already_a_file(f2)==0) {
		    if(errorMsg == 0) {
		      write(2, "1cp: impossible de copier le fichier «", 39);
		      write(2, f2, strlen(f2));
		      write(2, "»: Le fichier existe\n", 23);
		    }
		    for(int j=strarraylen(path_file_2)-1; j>=0; j--) cd(rev[j]);
		    return -1;
		  }
		  
		  lseek(o5, -512, SEEK_CUR);
		  int w1 = write(o5, &h2, BLOCKSIZE);
		  if(w1 == -1) { perror("write"); exit(EXIT_FAILURE); }
		  int w2 = write(o5, res, strlen(res));
		  if(w2 == -1) { perror("write"); exit(EXIT_FAILURE); }
		  int nb_blocks_to_add = (strlen(res) / BLOCKSIZE) + 2;
		  for(int i=0; i<=nb_blocks_to_add; i++) write(o5, &h, BLOCKSIZE);
		  
		  close(o5);
		  for(int j=strarraylen(path_file_2)-1; j>=0; j--) cd(rev[j]);
		  arrayfree(rev);
		  return 0;
		}
		else lseek(o5, ((size_3 + BLOCKSIZE -1) >> BLOCKBITS )*512, SEEK_CUR);
	      }
	      close(o5);
	    }
	    else {
	      int o9 = open(f2, O_WRONLY | O_CREAT, 0666);
	      
	      write(o9, res, strlen(res));
	      close(o9);
	      
	      for(int j=strarraylen(path_file_2)-1; j>=0; j--) cd(rev[j]);
	      arrayfree(rev);
	      return 0;
	    }
	  }
	}
	else lseek(o5, ((size + BLOCKSIZE -1) >> BLOCKBITS )*512, SEEK_CUR);
      }
    }
    else {// file1 est un chemin, on doit alors traiter les deux cas : on se trouve dans un tar après le cd sur le chemin, ou non
      
      int o6 = open(TAR_IN_NAME, O_RDWR);
      char res2[FILESIZE];

      char *file_cpy = getCpy(file1);
      char **path_file = make_path(file_cpy);
      char *f3 = malloc(sizeof(char)*strlen(path_file[strarraylen(path_file)-1]));
      strcpy(f3, path_file[strarraylen(path_file)-1]);
      path_file[strarraylen(path_file)-1] = NULL;
      
      char **rev = dir_exists(path_file);
      cd_aux(path_file, 1);
      
      if(IN_TAR == 0) { //copie dans le cas où on est dans un tar
	
	char name_tmp[PATHSIZE] = "";
	strcat(name_tmp, TAR_PATH);
	strcat(name_tmp, f3);
	
	while(1) {
	  struct posix_header h;
	  memset(&h, 0, BLOCKSIZE);
	  
	  ssize_t lu = read(o6, &h, BLOCKSIZE);
	  if(lu <= 0) break;
	  
	  size_t size;
	  sscanf(h.size, "%lo", &size);
	  
	  if(strcmp(h.name, name_tmp)==0) {
	    struct posix_header h2;
	    memset(&h2, 0, BLOCKSIZE);
	    
	    h2 = h; //copie de l'entête
	    read(o6, res2, ((size + BLOCKSIZE -1) >> BLOCKBITS )*512); //copie du contenu

	    for(int j=strarraylen(path_file)-1; j>=0; j--) cd(rev[j]);

	    char new_name[PATHSIZE] = "";
	    
	    lseek(o6, 0, SEEK_SET);

	    if(is_path(file2)==1) {
	      strcat(new_name, TAR_PATH);
	      strcat(new_name, file2);
	      strcpy(h2.name, new_name); //changement du nom
	      
	      while(1) {
		ssize_t lu_2 = read(o6, &h, BLOCKSIZE);
		if(lu_2 <= 0) break;
		
		size_t size_2;
		sscanf(h.size, "%lo", &size_2);
		
		if(h.typeflag == '\0') {
		  
		  if(is_already_a_file(file2)==0) {
		    if(errorMsg == 0) {
		      write(2, "2cp: impossible de copier le fichier «", 39);
		      write(2, new_name, strlen(new_name));
		      write(2, "»: Le fichier existe\n", 23);
		    }
		    return -1;
		  }
		  
		  lseek(o6, -512, SEEK_CUR);
		  int w1 = write(o6, &h2, BLOCKSIZE);
		  if(w1 == -1) { perror("write"); exit(EXIT_FAILURE); }
		  int w2 = write(o6, res2, strlen(res2));
		  if(w2 == -1) { perror("write"); exit(EXIT_FAILURE); }
		  int nb_blocks_to_add = (strlen(res2) / BLOCKSIZE) + 2;
		  for(int i=0; i<=nb_blocks_to_add; i++) write(o6, &h, BLOCKSIZE);
		  
		  close(o6);
		  return 0;
		}
		else lseek(o6, ((size_2 + BLOCKSIZE -1) >> BLOCKBITS )*512, SEEK_CUR);
	      }
	      close(o6);
	    }

	    else {

	      
	      char *file_cpy_2 = getCpy(file2);
	      char **path_file_2 = make_path(file_cpy_2);
	      char *f4 = malloc(sizeof(char)*strlen(path_file_2[strarraylen(path_file_2)-1]));
	      strcpy(f4, path_file_2[strarraylen(path_file_2)-1]);
	      path_file_2[strarraylen(path_file_2)-1] = NULL;
	      
	      char **rev = dir_exists(path_file_2);
	      cd_aux(path_file_2, 1);

	      strcat(new_name, TAR_PATH);
	      
	      char *tmp = getCpy(file2);
	      char *tmp2 = getLastPath(tmp);
	      strcat(new_name, tmp2);
	      strcpy(h2.name, new_name);
	      
	      if(IN_TAR == 0) {
		while(1) {
		  ssize_t lu_3 = read(o6, &h, BLOCKSIZE);
		  if(lu_3 <= 0) break;
		  
		  size_t size_3;
		  sscanf(h.size, "%lo", &size_3);
		  
		  if(h.typeflag == '\0') {
		    
		    if(is_already_a_file(f4)==0) {
		    if(errorMsg == 0) {
		      write(2, "3cp: impossible de copier le fichier «", 39);
		      write(2, f4, strlen(f4));
		      write(2, "»: Le fichier existe\n", 23);
		    }
		    for(int j=strarraylen(path_file_2)-1; j>=0; j--) cd(rev[j]);
		    return -1;
		  }
		    
		    lseek(o6, -512, SEEK_CUR);
		    int w1 = write(o6, &h2, BLOCKSIZE);
		    if(w1 == -1) { perror("write"); exit(EXIT_FAILURE); }
		    int w2 = write(o6, res2, strlen(res2));
		    if(w2 == -1) { perror("write"); exit(EXIT_FAILURE); }
		    int nb_blocks_to_add = (strlen(res2) / BLOCKSIZE) + 2;
		    for(int i=0; i<=nb_blocks_to_add; i++) write(o6, &h, BLOCKSIZE);
		    
		    close(o6);
		    for(int j=strarraylen(path_file_2)-1; j>=0; j--) cd(rev[j]);
		    return 0;
		  }
		  else lseek(o6, ((size_3 + BLOCKSIZE -1) >> BLOCKBITS )*512, SEEK_CUR);
		}
		close(o6);
	      }
	      else {
		int o9 = open(f4, O_WRONLY | O_CREAT, 0666);

		write(o9, res2, strlen(res2));

		for(int j=strarraylen(path_file_2)-1; j>=0; j--) cd(rev[j]);
		close(o9);
		return 0;
		
		
	      }
	    }
	    
	  }
	  else lseek(o6, ((size + BLOCKSIZE -1) >> BLOCKBITS )*512, SEEK_CUR);
	}
      }
      
      else { //copie dans le cas où on est pas dans un tar
	char res3[FILESIZE];
	int o7 = open(f, O_RDONLY);
	read(o7, res3, FILESIZE);
	close(o7);
	
	for(int j=strarraylen(path_file)-1; j>=0; j--) cd(rev[j]);
	
	int o8 = open(TAR_IN_NAME, O_RDWR);

	struct posix_header h;
	memset(&h, 0, BLOCKSIZE);
	
	struct posix_header h2;
	memset(&h2, 0, BLOCKSIZE);
	
        read(o8, &h2, BLOCKSIZE);

	lseek(o8, 0, SEEK_SET);
	
	char new_name[PATHSIZE] = "";
	sprintf(h2.size, "%011lo", strlen(res3));
	h2.typeflag = '0';

	if(is_path(file2)==1) {
	  strcat(new_name, TAR_PATH);
	  strcat(new_name, file2);
	  strcpy(h2.name, new_name); //changement du nom
	  
	  while(1) {
	    ssize_t lu_2 = read(o8, &h, BLOCKSIZE);
	    if(lu_2 <= 0) break;
	    
	    size_t size_2;
	    sscanf(h.size, "%lo", &size_2);
	    
	    if(h.typeflag == '\0') {
	      
	      if(is_already_a_file(file2)==0) {
		if(errorMsg == 0) {
		  write(2, "4cp: impossible de copier le fichier «", 39);
		  write(2, new_name, strlen(new_name));
		  write(2, "»: Le fichier existe\n", 23);
		}
		return -1;
	      }
	      
	      lseek(o8, -512, SEEK_CUR);
	      int w1 = write(o8, &h2, BLOCKSIZE);
	      if(w1 == -1) { perror("write"); exit(EXIT_FAILURE); }
	      int w2 = write(o8, res3, strlen(res3));
	      if(w2 == -1) { perror("write"); exit(EXIT_FAILURE); }
	      int nb_blocks_to_add = (strlen(res3) / BLOCKSIZE) + 2;
	      for(int i=0; i<=nb_blocks_to_add; i++) write(o8, &h, BLOCKSIZE);

	      close(o8);
	      return 0;
	    }
	    else lseek(o8, ((size_2 + BLOCKSIZE -1) >> BLOCKBITS )*512, SEEK_CUR);
	  }
	  close(o8);
	}
	else {
	  
	  char *file_cpy_2 = getCpy(file2);
	  char **path_file_2 = make_path(file_cpy_2);
	  char *f5 = malloc(sizeof(char)*strlen(path_file_2[strarraylen(path_file_2)-1]));
	  strcpy(f5, path_file_2[strarraylen(path_file_2)-1]);
	  path_file_2[strarraylen(path_file_2)-1] = NULL;
	  
	  char **rev = dir_exists(path_file_2);
	  cd_aux(path_file_2, 1);

	  strcat(new_name, TAR_PATH);
	  char *tmp = getCpy(file2);
	  char *tmp2 = getLastPath(tmp);
	  strcat(new_name, tmp2);
	  strcpy(h2.name, new_name);
	  
	  if(IN_TAR == 0) {
	    while(1) {
	      ssize_t lu_3 = read(o8, &h, BLOCKSIZE);
	      if(lu_3 <= 0) break;
	      
	      size_t size_3;
	      sscanf(h.size, "%lo", &size_3);
	      
	      if(h.typeflag == '\0') {
		
	        if(is_already_a_file(f5)==0) {
		    if(errorMsg == 0) {
		      write(2, "5cp: impossible de copier le fichier «", 39);
		      write(2, f5, strlen(f5));
		      write(2, "»: Le fichier existe\n", 23);
		    }
		    for(int j=strarraylen(path_file_2)-1; j>=0; j--) cd(rev[j]);
		    return -1;
		  }
		
		lseek(o8, -512, SEEK_CUR);
		int w1 = write(o8, &h2, BLOCKSIZE);
		if(w1 == -1) { perror("write"); exit(EXIT_FAILURE); }
		int w2 = write(o8, res3, strlen(res3));
		if(w2 == -1) { perror("write"); exit(EXIT_FAILURE); }
		int nb_blocks_to_add = (strlen(res3) / BLOCKSIZE) + 2;
		for(int i=0; i<=nb_blocks_to_add; i++) write(o8, &h, BLOCKSIZE);
		
		close(o8);
		for(int j=strarraylen(path_file_2)-1; j>=0; j--) cd(rev[j]);
		arrayfree(rev);
		return 0;
	      }
	      else lseek(o8, ((size_3 + BLOCKSIZE -1) >> BLOCKBITS )*512, SEEK_CUR);
	    }
	    close(o8);
	  }
	  else {
	    int o9 = open(f5, O_WRONLY | O_CREAT, 0666);
	    
	    write(o9, res3, strlen(res3));

	    close(o9);
	    
	    for(int j=strarraylen(path_file_2)-1; j>=0; j--) cd(rev[j]);
	    arrayfree(rev);
	    return 0;
	  }
	}
      }
    }
  }
  if(errorMsg == 0) write(2, "cp: Aucun fichier ou dossier de ce type\n", 41);
  return -1;
}

int cp_tar_aux(char **cmd, int errorMsg2) {

  int return_value = 0;
  
  char *cpy_dest = getCpy(cmd[strarraylen(cmd)-1]);
  char **path_dest = make_path(cpy_dest);
  
  if(strarraylen(cmd) == 1) {
    if(errorMsg2 == 0) write(2, "cp: opérande manquant\n", 23);
    return -1;
  }
  
  else if(dir_exists(path_dest) != NULL) {
    for(int i=1; i<strarraylen(cmd)-1; i++) {
      int y;
      if(errorMsg2 == 0) y = cp_tar(cmd[i], NULL, cmd[strarraylen(cmd)-1], 0);
      else y = cp_tar(cmd[i], NULL, cmd[strarraylen(cmd)-1], 1);
      if(y == -1) return_value = -1;
    }
  }
  else {
    if(strarraylen(cmd) > 3) {
      if(errorMsg2 == 0) {
	write(2, "cp: la cible '", 14);
	write(2, cmd[strarraylen(cmd)-1], strlen(cmd[strarraylen(cmd)-1]));
	write(2, "' n'est pas un répertoire", 25);
      }
      return -1;
    }
    else {
      int y;
      if(errorMsg2 == 0) y = cp_tar(cmd[1], cmd[2], NULL, 0);
      else y = cp_tar(cmd[1], cmd[2], NULL, 1);
      if(y == -1) return_value = -1;
    }	  
  }
  return return_value;
}
  
void ls_tar(int option) {
  
  char *last_path = getLastPath(REAL_PATH);
  
  char *listed = "";
  listed = malloc(sizeof(char) * PATHSIZE);
  
  int o = open(TAR_IN_NAME, O_RDONLY);
  if(o == -1) exit(1);
  
  while(1) { //dans cette boucle on va parcourir les entêtes du tar et afficher ce que l'on veut
    size_t size;
    struct posix_header h;
    memset(&h, 0, BLOCKSIZE);
    
    int lu = read(o, &h, BLOCKSIZE);
    if(lu < 1) break;

    sscanf(h.size, "%lo", &size);
    
    if(h.typeflag != '\0') { //si le fichier n'est pas un bloc vide

      char *cpy_name = getCpy(h.name);
      char **c = make_path(cpy_name); //on transforme le nom en tableau de chaînes de caractères
      
      if(is_tar(last_path)==0) {
	if(strstr(listed, c[0])==NULL) {
	  if(option == 0) {
	    char size_char[10];
	    sprintf(size_char, "%ld", size);
	    
	    char *rights = octToRights(h.mode);
	    if(h.typeflag == '5') write(1, "d", 1);
	    else write(1, "-", 1);
	    write(1, rights, strlen(rights));
	    write(1, " ", 1);
	    free(rights);

	    int t = get_links(TAR_IN_NAME, c[0]);
	    char links[3];
	    sprintf(links, "%d", t);
	    write(1, links, strlen(links));
	    write(1, " ", 1);
	    
	    write(1, h.uname, strlen(h.uname));
	    write(1, " ", 1);
	    
	    write(1, h.gname, strlen(h.gname));
	    write(1, " ", 1);
	    
	    write(1, size_char, strlen(size_char));
	    write(1, " ", 1);
	    
	    char *time = UTCtime(h.mtime);
	    write(1, time, strlen(time));
	    write(1, " ", 1);
	    time = NULL;
	    free(time);
	  }
	  
	  if(h.typeflag == '5') write(1, "\x1b[34m", 6);
	  write(1, c[0], strlen(c[0])); //on affiche la première chaîne de ce tableau
	  if(h.typeflag == '5') write(1, "\x1b[0m", 5);
	  write(1, " ", 1); //un espace ou deux pour la visibilité
	  if(option == 0) write(1, "\n", 2);
	  
	  strcat(listed, c[0]); //on stocke le nom du fic qu'on vient d'afficher dans listed
	  strcat(listed, " ");
	  }
      }
      else {
	if(strstr(h.name, last_path)!=NULL) {
	  for(int i=0; i<strarraylen(c); i++) {
	    if(strarraylen(c)>1) {
	      if(strcmp(c[i], last_path)==0 && i != strarraylen(c) - 1) {
		if(strstr(listed, c[i+1])==NULL && strcmp(c[i+1], last_path)!=0) {
		  if(option == 0) {
		    char size_char[10];
		    sprintf(size_char, "%ld", size);
		    
		    char *rights = octToRights(h.mode);
		    if(h.typeflag == '5') write(1, "d", 1);
		    else write(1, "-", 1);
		    write(1, rights, strlen(rights));
		    write(1, " ", 1);
		    free(rights);
		    
		    int t = get_links(TAR_IN_NAME, c[i+1]);
		    char links[3];
		    sprintf(links, "%d", t);
		    write(1, links, strlen(links));
		    write(1, " ", 1);
		    
		    write(1, h.uname, strlen(h.uname));
		    write(1, " ", 1);
		    
		    write(1, h.gname, strlen(h.gname));
		    write(1, " ", 1);
		    
		    write(1, size_char, strlen(size_char));
		    write(1, " ", 1);
		    
		    char *time = UTCtime(h.mtime);
		    write(1, time, strlen(time));
		    write(1, " ", 1);
		    time = NULL;
		    free(time);
		  }
		  if(h.typeflag == '5') write(1, "\x1b[34m", 6);
		  write(1, c[i+1], strlen(c[i+1]));
		  if(h.typeflag == '5') write(1, "\x1b[0m", 5);
		  write(1, " ", 1);
		  if(option == 0) write(1, "\n", 2);
		}
		strcat(listed, c[i+1]); //on stocke le nom du fic qu'on vient d'afficher dans listed
		strcat(listed, " ");
	      }
	    }
	  }
	}
      }
      lseek(o, ((size + BLOCKSIZE -1) >> BLOCKBITS )*512, SEEK_CUR);
      cpy_name = NULL;
      free(cpy_name);
      arrayfree(c);
    }
  }
  write(1, "\n", 1);
  close(o);
}

void ls_tar_aux(char **cmd) {
    
  if(strarraylen(cmd) == 1) ls_tar(1); // s'il n'y a pas d'argument, on fait juste ls là où on est

  else if(strarraylen(cmd) == 2 && strcmp(cmd[1], "-l")==0) ls_tar(0);

  else {

    for(int i = (strcmp(cmd[1], "-l")==0) ? 2 : 1; i<strarraylen(cmd); i++) {
      
      char *cpy = getCpy(cmd[i]);
      char **path = make_path(cpy);
      
      
      char **rev = dir_exists(path);
      int x = cd_aux(path, 1);
      
      if(x != -1) { // si cd réussit
	if(IN_TAR == 0) { // si on se trouve dans un tar après le cd
	  if( (strarraylen(cmd) > 2 && strcmp(cmd[1], "-l")!=0) || ((strarraylen(cmd) > 3 && strcmp(cmd[1], "-l")==0))) {
	    write(1, cmd[i], strlen(cmd[i]));
	    write(1, " :\n", 4);
	  }
	  if((strcmp(cmd[1], "-l")==0)) ls_tar(0);
	  else ls_tar(1);
	  if(strarraylen(cmd) > 2) write(1, "\n", 2);
	  for(int j=strarraylen(path)-1; j>=0; j--) cd(rev[j]); //puis on revient là où on était
	}
	else { // si on ne se trouve pas dans un tar après le cd
	  if((strcmp(cmd[1], "-l")==0)) {
	    char *tmp[] = {"ls", "-l", NULL};
	    execution(tmp); // on fait un exec de 'ls -l'
	  }
	  else {
	    char *tmp[] = {"ls", NULL};
	    execution(tmp); // on fait un exec de 'ls'
	  }
	  for(int j=strarraylen(path)-1; j>=0; j--) cd(rev[j]); // puis on revient là où on était
	}
      }
      else {// si cd échoue, on va afficher une erreur
	write(2, "ls: impossible d'accéder à '", 30);
	write(2, cmd[i], strlen(cmd[i]));
	write(2, "': Aucun dossier de ce type\n", 29);
      }
      if(rev != NULL) arrayfree(rev);
      arrayfree(path);
    }
  }
}

int rm_tar(char *file, int errorMsg) {
  if(is_directory(file)==0 && errorMsg == 0) {
    write(2, "rm: impossible de supprimer '", 29);
    write(2, file, strlen(file));
    write(2, "': est un dossier\n", 19);
    return -1;
  }

  char *last_path = getLastPath(REAL_PATH);
  
  int o = open(TAR_IN_NAME, O_RDWR);

  off_t file_size = lseek(o, 0, SEEK_END);
  lseek(o, 0, SEEK_SET);

  while(1) {
    struct posix_header h;
    
    ssize_t lu = read(o, &h, BLOCKSIZE);
    if(lu <= 0) break;
    
    size_t size;
    sscanf(h.size, "%lo", &size);

    char *cpy_name = getCpy(h.name);
    char **c = make_path(cpy_name); //on transforme le nom en tableau de chaînes de caractères
    
    if(is_tar(last_path) == 0) {
      if(strcmp(h.name, file)==0) {

	char buffer1[FILESIZE];
	char buffer2[FILESIZE];
        
	off_t pos = lseek(o, 0, SEEK_CUR) - BLOCKSIZE; // on sauvegarde la position juste avant le fichier
	lseek(o, 0, SEEK_SET); // on revient au début du fichier
	ssize_t lu = read(o, buffer1, pos); // on lit jusqu'à pos
	lseek(o, BLOCKSIZE, SEEK_CUR); //on revient après l'entête du fichier à supprimer
	
	off_t pos2 = lseek(o, ((size + BLOCKSIZE -1) >> BLOCKBITS)*512, SEEK_CUR); // on se place après le fichier à supprimer et on enregistre cette position

	ssize_t lu_2 = read(o, buffer2, file_size - pos2); // on lit tout ce qu'il y'a après le fichier à supprimer
	close(o);

	int o2 = open(TAR_IN_NAME, O_RDWR | O_TRUNC); //on réouvre le fichier en écrasant le contenu
	// puis on copie les deux buffers qui représentent le contenu du tar, sans le fichier qu'on a supprimé
	write(o2, buffer1, lu);
	lseek(o2, 0, SEEK_END);
	write(o2, buffer2, lu_2);
	
	
	close(o2);
	free(c);
	return 0;
      }
      else lseek(o, ((size + BLOCKSIZE -1) >> BLOCKBITS )*512, SEEK_CUR);
    }
    else {
      if(strstr(h.name, last_path)!=NULL) {
	for(int i=0; i<strarraylen(c); i++) {
	  if(strarraylen(c)>1 && i != strarraylen(c) - 1) {
	    if(strcmp(c[i], last_path)==0 && strcmp(c[i+1], file)==0) {
	      char buffer1[FILESIZE];
	      char buffer2[FILESIZE];
	     
	      off_t pos = lseek(o, 0, SEEK_CUR) - BLOCKSIZE;

	      lseek(o, 0, SEEK_SET);
	      ssize_t lu = read(o, buffer1, pos);
	      lseek(o, BLOCKSIZE, SEEK_CUR);
	      
	      off_t pos2 = lseek(o, ((size + BLOCKSIZE -1) >> BLOCKBITS)*512, SEEK_CUR);
	      ssize_t lu_2 = read(o, buffer2, file_size - pos2);
	      close(o);
	      
	      int o2 = open(TAR_IN_NAME, O_RDWR | O_TRUNC);
	      write(o2, buffer1, lu);
	      lseek(o2, 0, SEEK_END);
	      write(o2, buffer2, lu_2);
	      
	      close(o2);
	      free(c);
	      return 0;
	    }
	  }
	}
      }
      lseek(o, ((size + BLOCKSIZE -1) >> BLOCKBITS )*512, SEEK_CUR); free(c); c=NULL;
    }
  }
  if(errorMsg == 0) {
    write(2, "rm: impossible de supprimer '", 29);
    write(2, file, strlen(file));
    write(2, "': Aucun fichier ou dossier de ce type\n", 40);
  }
  return -1;
}

void rm_tar_aux(char **cmd, int errorMsg2) {

  char **path = NULL;
  char *file = NULL;

  if(strarraylen(cmd) == 1) {
    if(errorMsg2 == 0) write(2, "rm: opérande manquant\n", 23);
}

  else if(strarraylen(cmd) == 2 && strcmp(cmd[1], "-r")==0) {
    if(errorMsg2 == 0) write(2, "rm: opérande manquant\n", 23);
  }
  
  else {
    for(int i = 1; i<strarraylen(cmd); i++) {
      
      if(strstr(cmd[i], "/")==NULL) {
	(errorMsg2 == 0) ? rm_tar(cmd[i], 0) : rm_tar(cmd[i], 1);
      }
      else {
	char *cpy = getCpy(cmd[i]);
	
	path = make_path(cpy);
	file = malloc(sizeof(char) * strlen(path[strarraylen(path)-1]));
	strcpy(file, path[strarraylen(path)-1]);
	path[strarraylen(path)-1] = NULL;
	
	char **rev = dir_exists(path);
	int x = cd_aux(path, 1);
	
	if(x != -1) { // si cd réussit
	  if(IN_TAR == 0) { // si on se trouve dans un tar après le cd
	    (errorMsg2 == 0) ? rm_tar(file, 0) : rm_tar(file, 1);
	    
	    for(int i=strarraylen(path)-1; i>=0; i--) cd(rev[i]); //puis on revient là où on était
	  }
	  else { // si on ne se trouve pas dans un tar après le cd
	    char *tmp[] = {"rm", file, NULL};
	    execution(tmp); // on fait un exec de 'rm' sur 'file'
	    for(int i=strarraylen(path)-1; i>=0; i--) cd(rev[i]); // puis on revient là où on était
	  }
	}
	else {
	  if(errorMsg2 == 0) {
	    write(2, "rm: impossible de supprimer '", 29);
	    write(2, cmd[i], strlen(cmd[i]));
	    write(2, "': Aucun fichier ou dossier de ce type\n", 40);
	  }
	}
	if(rev != NULL) arrayfree(rev);
	arrayfree(path);
      }
    }
  }
}

int mkdir_tar(char *name) {

  char *cpy = getCpy(name);
  
  if(is_directory(cpy)==0) { //si le répertoire existe déjà on retourne une erreur
    write(2, "mkdir: impossible de créer le répertoire «", 45);
    write(2, cpy, strlen(name));
    write(2, "»: Le fichier existe\n", 23);
    return -1;
  }
  
  if(IN_TAR == 0) {
    write(1, "name :", 6);
    write(1, name, strlen(name));
    write(1, "\n", 2);
    
    int o = open(TAR_IN_NAME, O_RDWR);
      
    struct posix_header h;
    memset(&h, 0, BLOCKSIZE);
      
    ssize_t lu = read(o, &h, BLOCKSIZE);
    if(lu <= 0) exit(EXIT_FAILURE);
    struct posix_header h2;
    memset(&h2, 0, BLOCKSIZE);
      
    h2 = h; //copie de l'entête
    
    char new_name[PATHSIZE] = "";
    strcat(new_name, name);
    strcat(new_name, "/");
      
      memset(&h2.name, 0, PATHSIZE);
      strcpy(h2.name, new_name); //changement du nom
      
      h2.typeflag = '5'; //changement du type
      
      sprintf(h2.size, "%011o", 0);
      
      lseek(o, 0, SEEK_SET);
      
      while(1) {
	ssize_t lu_2 = read(o, &h, BLOCKSIZE);
	if(lu_2 <= 0) break;
	
	size_t size_2;
	sscanf(h.size, "%lo", &size_2);
	
	if(h.typeflag == '\0') {
	  lseek(o, -512, SEEK_CUR);
	  int w1 = write(o, &h2, BLOCKSIZE);
	  if(w1 == -1) { perror("write"); exit(EXIT_FAILURE); }
	  
	  write(o, &h, BLOCKSIZE); //on remet un bloc vide à la fin du fichier car il y'en a un en moins (car on a écrit dessus l'entête du nouveau répertoire)
      	  close(o);
	  return 0;
	}
	else lseek(o, ((size_2 + BLOCKSIZE -1) >> BLOCKBITS )*512, SEEK_CUR);
      }
    }
    
    else { //cas où on crée un .tar vide
  
      int o = open(name, O_RDWR | O_CREAT, 0666);
      
      close(o);
      return 0;
    }
    return -1;
}

void mkdir_tar_aux(char **names) {

  char **path = NULL;
  char *dir = NULL;
  
  if(!(strarraylen(names) > 1)) write(2, "mkdir: opérande manquant\n", 26);
  
  else {

    for(int i=1; i<strarraylen(names); i++) {

      char *cpy = getCpy(names[i]);

      if(cpy[strlen(cpy)-1]=='/') cpy[strlen(cpy)-1] = '\0';
      
      if(strstr(cpy, "/")==NULL) mkdir_tar(cpy);
      
      else {
	path = make_path(cpy);
	dir = malloc(sizeof(char) * strlen(path[strarraylen(path)-1]));
	strcpy(dir, path[strarraylen(path)-1]);
	path[strarraylen(path)-1] = NULL;
	
	char **rev = dir_exists(path);
	int x = cd_aux(path, 1);
	
	if(x != -1) { // si cd réussit
	  if(IN_TAR == 0) { // si on se trouve dans un tar après le cd
	    mkdir_tar(dir);
	    
	    for(int i=strarraylen(path)-1; i>=0; i--) cd(rev[i]); //puis on revient là où on était
	    break;
	  }
	  else { // si on ne se trouve pas dans un tar après le cd
	    char *tmp[] = {"mkdir", dir, NULL};
	    execution(tmp); // on fait un exec de 'mkdir'
	    for(int i=strarraylen(path)-1; i>=0; i--) cd(rev[i]); // puis on revient là où on était
	    break;
	  }
	}
	else write(2, "mkdir : Aucun dossier de ce type\n", 34);
	if(rev != NULL) arrayfree(rev);
	arrayfree(path);
      }
    }
  }
}

int cp_tar_option(char **dirs) {
  if(strarraylen(dirs) == 3) write(2, "cp: opérande de fichier manquant\n", 34);
  
   else {

     for(int i = 2; i<strarraylen(dirs); i++) {
       char *cpy = getCpy(dirs[i]);
       
       if(cpy[strlen(cpy)-1]=='/') cpy[strlen(cpy)-1] = '\0';
       
       if(strarraylen(dirs) > 4) {
	 
	 if((is_directory(cpy)==0) && (is_directory(dirs[strarraylen(dirs)-1])==0) ) {
	   char *dir_name = getLastPath(cpy);
	   char dir_to_create[PATHSIZE] = "";
	   
	   strcpy(dir_to_create, dirs[strarraylen(dirs)-1]);
	   strcat(dir_to_create+strlen(dir_to_create), "/");
	   strcat(dir_to_create+strlen(dir_to_create), dir_name);
	   
	   mkdir_tar(dir_to_create);
	   
	   if(IN_TAR == 0) {
	     strcat(dir_name, "/");
	     
	     int o = open(TAR_IN_NAME, O_RDWR);
	     
	     while(1) {
	       struct posix_header h;
	       memset(&h, 0, BLOCKSIZE);
	       
	       ssize_t lu = read(o, &h, BLOCKSIZE);
	       if(lu <= 0) break;
	       
	       size_t size;
	       sscanf(h.size, "%lo", &size);
	       
	       if(strstr(h.name, dir_name)!=NULL && strcmp(h.name, dir_name)!=0) {
		 write(1, "yes\n", 5);
		 cp_tar(h.name, NULL, dir_to_create, 0);
		 
	       }
	       
	       lseek(o, ((size + BLOCKSIZE -1) >> BLOCKBITS )*512, SEEK_CUR);
	     }
	   }
	   else {
	   }
	 }
       }
     }
   }
  return -1;
}

int rmdir_tar(char *file, int option) {


  if(is_directory(file) == 1) { // on vérifie si file est un dossier
    if(option == 0) {
      int y = rm_tar(file, 0);
      if(y == 0) return 0;
      else {
	write(2, "rm: impossible de supprimer '", 29);
	write(2, file, strlen(file));
	write(2, "': Aucun fichier ou dossier de ce type\n", 40);
      return -1;
      }
    }
  }
  else {
    if( (option == 0) && (IN_TAR == 1) && (is_tar(file)==0) ) { //si on supprime un .tar
      char *tmp[] = {"rm", file};
      execution(tmp);
      return 0;
    }
  }
  int missing_file = 0;
  
  int x = 0;

  char *last_path = getLastPath(REAL_PATH);

  while(1) {
    int o = open(TAR_IN_NAME, O_RDONLY);

    int bool = 1;
    int cpt = 0;

    while(1) { // dans ce morceau de code on vérifie s'il y'a encore des fichiers à supprimer
      struct posix_header h;
      memset(&h, 0, BLOCKSIZE);
      
      ssize_t lu = read(o, &h, BLOCKSIZE);
      if(lu <= 0) { break; }

      size_t size;
      sscanf(h.size, "%lo", &size);

      char *cpy_name = getCpy(h.name);
      char **c = make_path(cpy_name);

      for(int i =0; i<strarraylen(c); i++) {
	if(strcmp(c[i], file) == 0) { bool = 0; cpt++; }
      }
      lseek(o, ((size + BLOCKSIZE -1) >> BLOCKBITS )*512, SEEK_CUR); c=NULL; free(c);
    }
    
    if(option == 1) { if(cpt>1) { write(1, "rmdir: impossible de supprimer, le dossier n'est pas vide\n", 59); return -1;} }

    if(bool == 1) {
      if(missing_file == 0) {
	close(o);
	break;
      }
      else {
	close(o);
	return 0;
      }
    }
    
    off_t file_size = lseek(o, 0, SEEK_END);
    lseek(o, 0, SEEK_SET);

    while(1) {
    
      struct posix_header h;
      
      ssize_t lu = read(o, &h, BLOCKSIZE);
      if(lu <= 0) {
	if(missing_file == 0) {
	  close(o);
	  break;
	}
	else {
	  close(o);
	  return 0;
	}
      }
      
      size_t size;
      sscanf(h.size, "%lo", &size);
      
      char *cpy_name = getCpy(h.name);
      char **c = make_path(cpy_name); //on transforme le nom en tableau de chaînes de caractères
      
      if(is_tar(last_path) == 0) {
	if(strcmp(c[0], file)==0) {
	  
	  char buffer1[50000];
	  char buffer2[50000];
	  
	  off_t pos = lseek(o, 0, SEEK_CUR) - BLOCKSIZE;
	  
	  lseek(o, 0, SEEK_SET);
	  ssize_t lu = read(o, buffer1, pos);
	  lseek(o, BLOCKSIZE, SEEK_CUR);
	  
	  off_t pos2 = lseek(o, ((size + BLOCKSIZE -1) >> BLOCKBITS)*512, SEEK_CUR);
	  ssize_t lu_2 = read(o, buffer2, file_size - pos2);
	  close(o);
	  
	  int o2 = open(TAR_IN_NAME, O_RDWR | O_TRUNC);
	  write(o2, buffer1, lu);
	  lseek(o2, 0, SEEK_END);
	  write(o2, buffer2, lu_2);
	  
	  close(o2);
	  
	  missing_file = 1;
	  break;
	}
	else {lseek(o, ((size + BLOCKSIZE -1) >> BLOCKBITS )*512, SEEK_CUR); c=NULL; free(c);}
      }
      else {
	if(strstr(h.name, last_path)!=NULL) {
	  for(int i=0; i<strarraylen(c); i++) {
	    if(strarraylen(c)>1 && i != strarraylen(c) - 1) {
	      if(strcmp(c[i], last_path)==0 && strcmp(c[i+1], file)==0) {
		char buffer1[50000];
		char buffer2[50000];
		
		off_t pos = lseek(o, 0, SEEK_CUR) - BLOCKSIZE;
		
		lseek(o, 0, SEEK_SET);
		ssize_t lu = read(o, buffer1, pos);
		lseek(o, BLOCKSIZE, SEEK_CUR);
		
		off_t pos2 = lseek(o, ((size + BLOCKSIZE -1) >> BLOCKBITS)*512, SEEK_CUR);
		ssize_t lu_2 = read(o, buffer2, file_size - pos2);
		close(o);
		
		int o2 = open(TAR_IN_NAME, O_RDWR | O_TRUNC);
		write(o2, buffer1, lu);
		lseek(o2, 0, SEEK_END);
		write(o2, buffer2, lu_2);
		
		close(o2);
		
		missing_file = 1;
		x = 1;
	      }
	    }
	  }
	  if(x == 0) { lseek(o, ((size + BLOCKSIZE -1) >> BLOCKBITS )*512, SEEK_CUR); c=NULL; free(c);} else { x = 0; break; }
	}
	else { lseek(o, ((size + BLOCKSIZE -1) >> BLOCKBITS )*512, SEEK_CUR); c=NULL; free(c);}
      }
    }
  }
  write(2, "rmdir: impossible de supprimer '", 32);
  write(2, file, strlen(file));
  write(2, "': Aucun fichier ou dossier de ce type\n", 40);
  return -1;
}


void rmdir_tar_aux(char **cmd, int option) {

  char **path = NULL;
  char *file = NULL;

  if(option == 1 && strarraylen(cmd) == 1) write(2, "rmdir: opérande manquant\n", 26);
  
  else if(option == 0 && strarraylen(cmd) <= 2) write(2, "rm: opérande manquant\n", 23);
  
  else {
    for(int i = (option == 0) ? 2 : 1; i<strarraylen(cmd); i++) {
      char *cpy = getCpy(cmd[i]);

      if(cpy[strlen(cpy)-1]=='/') cpy[strlen(cpy)-1] = '\0';
      
      if(strstr(cpy, "/")==NULL) {
	if(option == 0) rmdir_tar(cpy, 0);
	else rmdir_tar(cpy, 1);
      }
      else {
	
	path = make_path(cpy);
	file = malloc(sizeof(char) * strlen(path[strarraylen(path)-1]));
	strcpy(file, path[strarraylen(path)-1]);
	path[strarraylen(path)-1] = NULL;
	
	char **rev = dir_exists(path);
	int x = cd_aux(path, 1);
	
	if(x != -1) { // si cd réussit
	  if(IN_TAR == 0) { // si on se trouve dans un tar après le cd
	    if(option == 0) rmdir_tar(file, 0);
	    else rmdir_tar(file, 1);
	    
	    for(int i=strarraylen(path)-1; i>=0; i--) cd(rev[i]); //puis on revient là où on était
	  }
	  else { // si on ne se trouve pas dans un tar après le cd
	    if(option == 0) {
	      char *tmp[] = {"rm", "-r", file, NULL};
	      execution(tmp);
	    }
	    else {
	      char *tmp[] = {"rmdir", file, NULL};
	      execution(tmp);
	    }
	    for(int i=strarraylen(path)-1; i>=0; i--) cd(rev[i]); // puis on revient là où on était
	  }
	}
	else {
	  (option == 0) ? (write (2, "rm: impossible de supprimer '", 29)) : (write (2, "rmdir: impossible de supprimer '", 32));
	  write(2, cmd[i], strlen(cmd[i]));
	  write(2, "': Aucun fichier ou dossier de ce type\n", 40);
	}
	if(rev != NULL) arrayfree(rev);
	arrayfree(path);
      }
    }
  }
}


void mv_tar(char **cmd) {
  
   if(strarraylen(cmd) <= 2) write(2, "mv: opérande de fichier manquant\n", 34);
  
   else {
     
     int x = cp_tar_aux(cmd, 1);

     if(x == -1) {
       write(2, "mv: Aucun fichier ou dossier de ce type\n", 40);
       return;
     }
     
     cmd[strarraylen(cmd)-1] = NULL;
     rm_tar_aux(cmd, 1);    
   }
}

  

void execution_tar(char **cmd) {
  if(strcmp(cmd[0], "cat")==0) cat_tar_aux(cmd);
  else if(strcmp(cmd[0], "ls")==0) ls_tar_aux(cmd);
  else if(strcmp(cmd[0], "cp")==0) {
    if(strcmp(cmd[1], "-r")==0) cp_tar_option(cmd);
    else cp_tar_aux(cmd, 0);
  }
  else if(strcmp(cmd[0], "rm")==0) {
    if(strcmp(cmd[1], "-r")==0) rmdir_tar_aux(cmd, 0);
    else rm_tar_aux(cmd, 0);
  }
  else if(strcmp(cmd[0], "rmdir")==0) rmdir_tar_aux(cmd, 1);
  else if(strcmp(cmd[0], "cd")==0) {
    char **tmp = make_path(cmd[1]);
    cd_aux(tmp, 0);
    arrayfree(tmp);
  }
  else if(strcmp(cmd[0], "pwd")==0) pwd();
  else if(strcmp(cmd[0], "exit")==0) exit(EXIT_SUCCESS);
  else if(strcmp(cmd[0], "mkdir")==0) mkdir_tar_aux(cmd);
  else if(strcmp(cmd[0], "mv")==0) mv_tar(cmd);
  else execution(cmd);
}
