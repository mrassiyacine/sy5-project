#include "tsh.h"
#include "cmd.h"
#include "cmd_tar.h"

int main() {

write(1, "\x1b[31m#################### Bienvenue sur TSH ! ####################\x1b[0m\n\n\n", 74);

  ssize_t lu=0;

  /*Le main contient une boucle infinie qui lit l'entrée standard et agit selon ce qui a été lu. Elle se temrine seulement quand il n'y a plus rien à lire (aka CTRL+D) */

  initialize_globals(); //initialisation des variables globales
  
  while(1) {
    size_t t = strlen(REAL_PATH);
    write(1, "~", 1);
    write(1, REAL_PATH, t+1);
    write(1, " \x1b[31m*prompt* \x1b[0m ", 19);
    
    BUF = malloc(sizeof(char) * BUFSIZE); //buffer pour le read
    
    lu=read(0, BUF, BUFSIZE);
    if(lu<=0) break;

    if(strcmp(BUF, "\n")!=0) { //dans le cas où l'utilisateur presse "entrée" on ne fait rien (obligé de mettre une condition car sinon crée des problèmes dans la suite du programme
    
      char *tmp2 = malloc(sizeof(char)*(int)lu); //pr y mettre ce qui a été lu
      for(int i=0; i<lu-1; i++) tmp2[i]=BUF[i];
      
      char *sep = malloc(sizeof(char)*3);
      sep = is_redir_or_pipe(tmp2);
      
      if(sep!=NULL) {
	
	char **redir = make_redir_or_pipe(tmp2, sep);
	redirection_or_pipe(redir, sep);
	arrayfree(redir);
      }
      
      else {
      
      int t = is_cmd_tar(tmp2);
	
	CMD=make_cmd(tmp2); //on stocke la commande dans un tableau de str
	
	//puis on agit selon le cas :
	if(t==0) execution_tar(CMD); //si on est dans un tar
	else execution(CMD);
	
	arrayfree(CMD);
      }
      
      sep = NULL;
      free(sep);
      tmp2 = NULL;
      free(tmp2);
      BUF = NULL;
      free(BUF);
    }
    else {
      BUF = NULL;
      free(BUF);
    }
  }
  
  free(TAR_PATH);
  free(PATH_BUF);
  free(TAR_IN_NAME);
  
  return 0;
}
