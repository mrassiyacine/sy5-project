ARCHITECTURE
============

Le but de ce projet, rappelons-le, est d'implémenter un shell ("tsh") qui traiterait les tarballs comme des répertoires. Pour traiter le sujet nous avons décidé de le séparer en deux parties pour pouvoir mieux aborder la chose : le cas où le shell fonctionne comme un shell "normal" et le cas où il traite les tarballs de la façon souhaitée.

En ce qui concerne l'organisation des fichiers, il y a pour le moment un seul fichier principal qui contient tout le code source et qui est le seul nécéssaire à l'exécution du programme. Nous attendons l'avis des professeurs quant à ce choix. Nous aurions préféré "modulariser" le projet en l'organisant de cette façon : 4 fichiers; le fichier principal avec le `main`, un fichier contenant les méthodes pour le shell "normal", un fichier pour les méthodes traitant le cas des tarballs, un fichier contenant des méthodes auxiliaires, puis inclure les 3 derniers dans le premier avec des headers. Cependant l'utilisation de variables globales dans le fichier principal et dans de nombreuses méthodes rend cela impossible (à moins qu'il n'y ait une solution ?).

En quelques lignes, le corps de notre shell est typiquement une boucle `while(1)` dans le `main` du fichier principal qui affiche le chemin du répertoire courant et un prompt, et qui lit (grâce à l'appel système `read()`) l'entrée standard et attend donc que l'utilisateur écrive quelque chose. Ce quelque chose est par la suite traité par une fonction (`make_cmd()`) qui va transformer la chaîne de caractères entrée par l'utilisateur en `char**` (ici nous nous sommes inspirés des arguments du `main`) ce qui rendra plus facile le traitement de cette entrée. Par la suite, selon le cas dans lequel on se trouve, le programme agira en conséquence :
* on se trouve dans un tarball ou on agit sur un tarball, dans ce cas le programme fait appel aux fonctions que nous avons écrites (selon ce que l'utilisateur a tapé). Pour cela il fait appel d'abord à une méthode : `execution_tar()` qui va elle décider quelle fonction appeler.
* on ne se trouve pas dans un tarball et on n'agit pas sur un tarball : le programme fait appel à la fonction `execution()` qui va soit appeler une fonction qui n'agit pas sur un tarball mais qui n'est pas exécutable pas `exec()` et que nous avons dû implémenter nous-mêmes (e.g. `cd()`, `pwd()`) soit simplement faire un appel à `exec()` sur la chaîne de caractères transformée avec `make_cmd()`.

Voici une description de toutes les fonctions / variables globales que nous avons implémentées pour le moment :

* *VARIABLES GLOBALES*
   * `REAL_PATH` sert à stocker le chermin vers le répertoire actuel qui sera modifié quand il impliquera des tarballs.
   * `PATH_BUF` sert à stocker le `getcwd()` que l'on fait sur `REAL_PATH`.
   * `in_tar` indique si l'on est dans un tarball ou non.
   * `tar_in_name` stocke le nom du tarball dans lequel on se trouve.
   * `TAR_PATH` contient le chemin de l'arborescence d'un tarball dans lequel on se trouve. Exemple si on fait : `cd test.tar/dir1/dir2`, `TAR_PATH` vaudra dir1/dir2.

* *FONCTIONS POUR COMMANDES EXÉCUTABLES PAR EXEC*
   * `void execution(char **cmd)` : cette fonction gère le cas où on n'est pas dans un tarball : soit `cmd[0]` est égal à `cd` ou `pwd` et dans ce cas là on appelle les fonctions correspondantes soit ce n'est pas le cas et alors on `fork` le processus; dans le cas où le fork échoue, il y a un perror et on quitte le programme. Dans le cas où le fork réussi : le fils fait un `execvp` sur `cmd` et le père attend que le fils ait fini.

* *FONCTIONS POR COMMANDES NON EXÉCUTABLES PAR EXEC*
  * `int cd(char *dir_or_tar)` : pour la fonction `cd` il a fallu gérer tous les cas possibles d'application : dans un premier temps il a fallu considérer le cas où l'utilisateur rentre en argument un chemin et non pas un simple nom de répertoire. Pour cela nous avons écrit une fonction auxiliaire `cd_aux(char **path)` qui va utiliser `make_path` (voir plus bas) sur `path` puis appeler `cd` sur chaque chaînes de caractères du résultat de `make_path(path)`.
 De plus, toujours dans cette fonction, s'il s'avère qu'un des répertoires du chemin passé en argument n'existe pas, on revient en arrière ou avant du nombre de fois que l'on a fait `cd`.
Cette fonction gère en fait les deux possibilités (un nom de répertoire en argument ou un chemin en argument).

Ensuite cinq sous-cas se dressent. Pour les gérer : une boucle while qui parcourt les entrées du répertoire dans lequel on se trouve grâce à `opendir` et `readdir` :

   *1/* on ne se trouve pas dans un tarball et l'argument n'est pas un tarball : dans ce cas on vérifie si l'entrée du répertoire (ou fichier) qu'on est entrain de lire a le même nom que `dir_or_tar` Si oui on vérifie que le fichier/répertoire est un répertoire. Si oui on appelle `chdir` sur ce répertoire. Sinon on continue de chercher, à chaque tour de la boucle, le bon nom. Si le nom n'a pas été trouvé parmi les entrées du répertoire courant, une erreur est affichée.
   
   *2/* l'argument sur lequel on fait `cd` est un tarball : ici on ajoute l'argument à la variable globale `REAL_PATH` puis on passe `IN_TAR` à 0 pour montrer qu'on est à présent dans un tarball (en le simulant bien sûr).
   
   *3/* on se trouve dans un tarball : dans ce cas on utilise une variable globale `TAR_PATH` qui sert à stocker le chemin du répertoire courant à partir du moment où l'on est "entré" dans le tarball. À cette variable globale on ajoute l'argument et `"/"` puis on la compare au nom de chaque fichier du tarball dans lequel on est. S'il y'a un fichier avec le même nom, on ajoute `"/"` et ce nom à `REAL_PATH`. Dans le cas où on ne trouve pas de fichier avec le même nom on enlève ce qu'on a ajouté à `TAR_PATH`.
   
   *4/* l'argument est `"."` et on est dans un tarball : dans ce cas on ne veut rien changer, mais on ne veut pas renvoyer d'erreur. On a choisi simplement de `write("")`.
   
   *5/* l'argument est `".."` et on est dans un tarball : dans ce cas on simule, encore une fois, que l'on veut revenir en arrière dans l'arborescence et on supprime de `REAL_PATH` et `TAR_PATH` le dernier nom qui y apparaissait (le nom du "répertoire" que l'on vient de quitter). Si on n'est plus dans un tarball (que l'on vérifie ici avec `is_path_tar`), on efface le contenu de `TAR_PATH` et on met `IN_TAR` à 1.
   La fonction retourne 0 si elle réussit, -1 si elle échoue (si l'argument est erroné).

   * `void pwd()` : affiche le nom du répertoire courant en affichant la variable globale `REAL_PATH`.
 
* *FONCTIONS POUR COMMANDES TRAITANT LE CAS DES TARBALLS*
   * `void cat_tar(char **files)` : cette fonction est censée afficher le contenu du ou des fichiers passé(s) en argument(s). `char **files` est un tableau de chaînes de caractères qui va représenter le ou les arguments. On fait une boucle for qui réitère X fois (X désignant la taille de `files`) et pour chaque indice de `files` l'algorithme suivant :
on parcourt les entêtes du tarball dans lequel on se trouve et on cherche si le nom stocké dans ces entêtes est celui du fichier. Si oui on lit dans le tarball (grâce à `read`) le nombre de blocs de 512 octets correspondant à la taille du contenu du fichier correspondant à cet entête, puis on l'écrit sur la sortie standard avec `write`. Sinon on "saute" le contenu de ce fichier avec `lseek` et on recommence. Si le fichier n'a pas été trouvé, un message d'erreur s'affiche.
_Comme précisé dans 'MODE DEMPLOI.md' cette fonction n'est pas complète_

   * `void ls_tar(char *file_or_option)` : 3 cas d'affichage :
   
 *1/* les fichiers contenus dans `file_or_option` si c'est un tarball, avec le même algorithme que cat_tar mais en affichant juste le nom stocké dans chaque entête.
 
 *2/* si on est dans un tarball et que la fonction est appelée sans argument : soit la même que *1/* si on est pas dans une arborescence du tarball. Exemple : `$cd test.tar` puis `ls`. 
 
 (Cette implémentation présente des anomalies et ne figure donc pas dans le rendu, voir `MODE DEMPLOI.md` pour plus de précisions). En revanche si on est dans une arborescence du tarball (exemple : `$cd test.tar/dir1/dir2`), la fonction va alors parcourir le tarball et vérifier pour chaque entête si le nom stocké contient ou non le nom du "répertoire" dans lequel on se trouve (ici `/dir2`). Si oui alors on analyse le nom stocké dans cet entête en le transformant avec `make_path` (dans le code : `char **tmp2 = make_path(h.name);`), puis si à l'avant dernier indice se trouve le nom du répertoire courant, c'est que au dernier indice se trouve un fichier/dossier contenu dans le répertoire actuel et que donc on veut l'afficher, ce qu'on fait par la suite.
 
 L'option `l` a été commencé mais ne fonctionne pas pour le moment.

 
   * `void cp_tar(char *file1, char *file2)` : *nous travaillons actullement sur cette fonction, elle présente encore des anomalies et n'est donc pas à tester pour ce rendu de mi-semestre.*

 
   * `void execution_tar(char **cmd)` : cette fonction va décider de quelle fonction appeler (selon ce qui se trouve dans `cmd`) dans le cas où on gère un tarball ou un/des fichier(s) de ce dernier. 

* FONCTIONS AUXILIAIRES
   * `initialize_globals()` : cette fonction sert simplement à initialiser les variables globales.
 
   * `is_tar(char *name)` : sert à vérifier si un fichier est un tarball ou non. La fonction `strstr` va simplement vérifier si `name` contient `.tar` en fin de nom.
 
   * `is_path_tar(char *name)` : même utilité que `is_tar` mais dans un chemin : sert à savoir si le chemin passé en paramètre passe par un tarball.
 
   * `make_cmd(char *str)` : construit un tableau de chaînes de caractères contenant les différentes chaînes de caractères séparées par un espace dans `str`. Ceci est fait grâce à un simple appel à `strtok`. À la fin on met la dernière chaîne de caractères à `NULL` pour pouvoir se repérer dans le parcours de ce genre de tableau (voir `strarraylen`).
 
   * `make_path(char *str)` : exactement la même fonction que `make_cmd` à la différence qu'elle traite les chaînes de caractères séparées par `/`. Utile pour gérer le cas où l'utilisateur rentre un chemin en argument d'une commande.
 
   * `int strarraylen(char **strarray)` : retourne la taille d'un tableau de chaînes de caractères.
 
   * `char *getLastPath(char *path)` : cette fonction retourne la dernière chaîne de caractères d'un tableau de chaînes de caractères crée avec `make_path`. Utilisée dans `cd_aux`.
