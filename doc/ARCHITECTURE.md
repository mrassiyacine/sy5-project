ARCHITECTURE
============

_Précision : un fichier `MODE DEMPLOI.md` est contenu dans le répertoire `doc` du gitlab de notre projet vous donnant des indications sur l'image Docker fournie et sur quelques points du projet._

Le but de ce projet, rappelons-le, est d'implémenter un shell ("tsh") qui traiterait les tarballs comme des répertoires. Pour traiter le sujet nous avons décidé de le séparer en deux parties bien distinctes pour pouvoir mieux aborder la chose : le cas où le shell fonctionne comme un shell "normal" et le cas où il traite les tarballs de la façon souhaitée.

En ce qui concerne l'organisation des fichiers, il y a 4 fichiers; le fichier principal `main.c` avec le `main`, un fichier contenant les méthodes pour le shell "normal" `cmd.c`, un fichier pour les méthodes traitant le cas des tarballs `cmd_tar.c`, un fichier contenant des méthodes auxiliaires et les variables globales (qui sont utilisées dans tous les autres fichiers) `tsh.c`, et un fichier `tar.c` qui, avec son header `tar.h`, reprend les définitions relatives à la structure des tarballs (ces fichiers sont inspirés du `tar.h` qui nous était proposé au premier TP de ce cours). Sont présents également les headers `cmd.h`, `cmd_tar.h` et `tsh.h`, et un Makefile permettant de compiler le tout.

En quelques lignes, le corps de notre shell est typiquement une boucle `while(1)` dans le `main` du fichier principal qui affiche le chemin du répertoire courant et un prompt, et qui lit (grâce à l'appel système `read()`) l'entrée standard et attend donc que l'utilisateur écrive quelque chose. Tout d'abord, on vérifie si cette expression est une redirection ou une combinaison de commandes, avec la fonction `is_redir_or_pipe()`. Si ce n'est pas le cas, c'est alors une commade basique. Dans les deux cas l'expression est traitée par une fonction de parsing (`make_redir_or_pipe()` ou `make_cmd()` ) qui va la transformer en `char**` (ici nous nous sommes inspirés des arguments du `main`) ce qui rendra plus facile le traitement de cette entrée. Par la suite, selon le cas dans lequel on se trouve, le programme agira en conséquence :
* l'expression est une commande basique :
 * on se trouve dans un tarball ou on agit sur un tarball, dans ce cas le programme fait appel aux fonctions que nous avons écrites (selon ce que l'utilisateur a tapé). Pour cela il fait appel d'abord à une méthode : `execution_tar()` qui va elle décider quelle fonction appeler.
 * on ne se trouve pas dans un tarball et on n'agit pas sur un tarball : le programme fait appel à la fonction `execution()` qui va soit appeler une fonction qui n'est pas exécutable pas `exec()` et que nous avons implémenté nous-mêmes (i.e. `cd()`, `pwd()`) soit simplement faire un appel à `exec()` sur la chaîne de caractères transformée avec `make_cmd()`.
* l'expression est une redirection ou une combinaison de commandes :
On appelle la fonction `redir_or_pipe()` qui va vérifier dans un premier temps si c'est une redirection ou une combinaison de commandes et ensuite agir selon les cas (`redir_or_pipe()` est expliquée plus loin).


Voici une description de toutes les fonctions / variables globales / constantes que nous avons implémentées / définies :

* *VARIABLES GLOBALES (présentes dans tsh.c mais utilisées dans tous les fichiers grâce au mot clé extern)*
 * `char *BUF` est le buffer dans lequel sera écrit ce que l'on va lire sur l'entrée standard à chaque tour de boucle.
 * `char **CMD` va stocker l'expression entrée par l'utilisateur, modifiée par la fonction `make_cmd`.
 * `char *REAL_PATH` sert à stocker le chermin vers le répertoire actuel qui sera modifié quand il impliquera des tarballs.
 * `char *PATH_BUF` sert à stocker le `getcwd()` que l'on fait sur `REAL_PATH` à son initialisation.
 * `int IN_TAR` indique si l'on est dans un tarball ou non (0 pour oui, 1 pour non).
 * `char *TAR_IN_NAME` stocke le nom du tarball dans lequel on se trouve.
 * `char *TAR_PATH` contient le chemin de l'arborescence d'un tarball dans lequel on se trouve. Exemple si on fait : `cd test.tar/dir1/dir2`, `TAR_PATH` sera `dir1/dir2/`.
   
* *CONSTANTES*
 * `BLOCKSIZE` : définie dans le fichier `tar.h`. Vaut 512 et représente la taille d'un bloc d'octets dans un tarball.
 * `BUFSIZE` : vaut 1000 (1Ko). Utilisée comme taille pour des buffers dans lesquels on a besoin de stocker des données.
 * `FILESIZE` : vaut 100000 (100Ko). Utilisée comme taille pour des buffers dans lesquels on va stocker beaucoup de données, le plus souvent pour copier des fichiers ou parties de fichiers.
 * `PATHSIZE` : vaut 100. Utilisée comme taille arbitraire d'un chemin / nom de fichier lorsqu'on remplace des noms dans des entêtes de tarballs par exemple.

* *FONCTIONS POUR COMMANDES 'NON TARBALLS'*
   
 * `int cd(char *dir_or_tar)` : pour la fonction `cd` il a fallu gérer tous les cas possibles d'application : dans un premier temps il a fallu considérer le cas où l'utilisateur rentre en argument un chemin et non pas un simple nom de répertoire. Pour cela nous avons écrit une fonction auxiliaire `cd_aux` qui va utiliser `make_path` (voir plus bas) sur `path` puis appeler `cd` sur chaque chaînes de caractères du résultat de `make_path(path)`.
 De plus, toujours dans cette fonction, s'il s'avère qu'un des répertoires du chemin passé en argument n'existe pas, on revient en arrière ou avant du nombre de fois que l'on a fait `cd`.
Cette fonction gère en fait les deux possibilités (un nom de répertoire en argument ou un chemin en argument).

Ensuite cinq sous-cas se dressent. Pour les gérer : une boucle while qui parcourt les entrées du répertoire dans lequel on se trouve grâce à `opendir` et `readdir` :

  * *1/* on ne se trouve pas dans un tarball et l'argument n'est pas un tarball : dans ce cas on vérifie si l'entrée du répertoire (ou fichier) qu'on est entrain de lire a le même nom que `dir_or_tar` Si oui on vérifie que le fichier/répertoire est un répertoire. Si oui on appelle `chdir` sur ce répertoire. Sinon on continue de chercher, à chaque tour de la boucle, le bon nom. Si le nom n'a pas été trouvé parmi les entrées du répertoire courant, une erreur est affichée.
   
  * *2/* l'argument sur lequel on fait `cd` est un tarball : ici on ajoute l'argument à la variable globale `REAL_PATH` puis on passe `IN_TAR` à 0 pour montrer qu'on est à présent dans un tarball (en le simulant bien sûr).
   
  * *3/* on se trouve dans un tarball : dans ce cas on utilise une variable globale `TAR_PATH` qui sert à stocker le chemin du répertoire courant à partir du moment où l'on est "entré" dans le tarball. À cette variable globale on ajoute l'argument et `"/"` puis on la compare au nom de chaque fichier du tarball dans lequel on est. S'il y'a un fichier avec le même nom, on ajoute `"/"` et ce nom à `REAL_PATH`. Dans le cas où on ne trouve pas de fichier avec le même nom on enlève ce qu'on a ajouté à `TAR_PATH`.
   
  * *4/* l'argument est `"."` et on est dans un tarball : dans ce cas on ne veut rien changer, mais on ne veut pas renvoyer d'erreur. On a choisi simplement de `write("")`.
   
  * *5/* l'argument est `".."` et on est dans un tarball : dans ce cas on simule, encore une fois, que l'on veut revenir en arrière dans l'arborescence et on supprime de `REAL_PATH` et `TAR_PATH` le dernier nom qui y apparaissait (le nom du "répertoire" que l'on vient de quitter). Si on n'est plus dans un tarball (que l'on vérifie ici avec `is_path_tar`), on efface le contenu de `TAR_PATH` et on met `IN_TAR` à 1.
   La fonction retourne 0 si elle réussit, -1 si elle échoue (si l'argument est erroné).
   
 * `char **dir_exists(char **path)` : prend un chemin sous la forme de tableaux de strings en paramètre et, crée et retourne le chemin "inverse". Exemples : `reverse_path(dir/dir1/dir2) = ../../..` ; si le chemin vers le répertoire dans lequel on se trouve est : `dir/dir1/dir2`, `reverse_path({"..", "..", ".."}) = "dir/dir1/dir2"`.
   
 * `int cd_aux(char **path, int errorMsg)` : cette fonction auxiliaire à `cd` est appelée lorque la première chaîne de l'expression que l'utilisateur a entré est `"cd"`. Elle prend en argument `path` (qui est en fait `CMD[1]`, qui représente alors l'argument de la commande), transformé avec la fonction `make_path`, et un entier qui sert à savoir si l'on doit afficher le message d'erreur de `cd` car cette fonction est utilisée dans d'autres fonctions et qu'on ne veut pas forcément voir ce message. Elle va d'abord vérifier si la commande a un argument (si `path[0] == NULL`). Si elle n'en pas, la fonction va appeler `cd("..")` jusqu'à revenir avant le répertoire parent (c'est-à-dire avant `home/` dans notre shell). Si elle a un argument et que ce dernier commence par `"~"`, c'est que l'utilisateur a rentré un chemin absolu. Dans ce cas on revient avant le répertoire parent (avec le même procédé qu'avant) et on fait `cd` succéssivement sur chaque string de `path`. Si le chemin est erroné, on revient dans le répertoire où l'on était en faisant `cd` sur chaque string du chemin inverse qu'on a eu avec la fonction `get_rev_path()`. Dans le cas où l'argument n'est pas un chemin absolu, la fonction va vérifier que le chemin `path` mène bien à un répertoire qui existe (avec la fonction `dir_exists()`). S'il existe, on lance `cd` sur chaque string de `path`, sinon on affiche un message d'erreur si `errorMsg == 0`.

 * `void pwd()` : affiche le nom du répertoire courant en affichant la variable globale `REAL_PATH`.
   
 * `void execution(char **cmd)` : cette fonction exécute les commandes dans le cas où l'on n'est pas dans un tarball et qu'on n'agit pas sur un tarball : soit `cmd[0]` est égal à `cd`, `pwd` ou à `exit` et dans ce cas là on appelle les fonctions correspondantes (pour `exit` on fait juste `exit(EXIT_SUCCESS)`) soit ce n'est pas le cas et alors on `fork` le processus; dans le cas où le fork échoue, il y a un perror et on quitte le programme. Dans le cas où le fork réussi : le fils fait un `execvp` sur `cmd` et le père attend que le fils ait fini.
   
 * `void redirection_or_pipe(char **redir, char *sep)` : cette fonction va appliquer la redirection ou la combinaison de commandes, selon la valeur du paramètre `sep` qui peut être : `|`, `<`, `>` ou `2>`. Au début de la fonction, on appelle `make_cmd()` sur le premier argument de `redir` car dans tous les cas ça sera une commande. Reste à savoir si nous avons à faire à une redirection ou un pipe.
  * premier cas de figure, `sep = |` :
  Dans ce cas là, on va appeler `make_cmd()` sur le deuxième argument de `redir` ça sera forcément une commande. Ensuite fait un appel `pipe` sur un tableau d'entiers `fd` de taille 2, on `fork` le processus et on re-`fork` le père. Par la suite on applique les bonnes redirections grâce à l'appel système `dup2()`, puis on fait des appels à `exec()` avec en argument la première ou la deuxième commande que nous transformées précédemment.
  * deuxième cas de figure, `sep = <` ou `sep = >` ou `sep = 2>` :
  Ici, on d'abord regarder si le dexuième argument de `redir` est un nom de fichier ou un chemin vers un fichier. Dans les deux cas on met à jour le champ `file` qui correspondera au nom du fichier. Si c'est un chemin, on se déplace dans le répertoire dans lequel se trouve le fichier. Qu'on se soit déplacé ou non :
   * si l'on est pas dans un tar on ouvre le fichier avec `open()` (s'il n'existe pas on le crée) puis selon la nature de la redirection, on sauvegarde dans un entier (soit STDIN, STDOUT ou STDERR déclarés au début de la fonction) le descripteur que l'on souhaite sauvgarder, avec l'appel système `dup()`. Ensuite, on effectue la redirection avec `dup2()` puis on exécute la commande (que l'on a transformée au tout début). La commande peut-être une commande impliquant un tarball ou non. 
   * si l'on est dans un tar, le procédé est le même sauf qu'avant de rediriger et d'exécuter la commande, on ouvre le tarball dans lequel on se trouve avec `open()` puis on se place au bon endroit (c'est-à-dire juste après l'entête du fichier dans lequel on souhaite effectuer la redirection) puis on redirige dans le descripteur correspondant à l'ouverture du tarball.
 
* *FONCTIONS POUR COMMANDES INTERNES TRAITANT LE CAS DES TARBALLS*

 * pour les fonctions gérant le cas des tarballs, chacune d'elle à une fonciton auxiliaire qui va appeler la fonction correspondante (ex : `ls_tar_aux` va faire appel à `ls_tar`), gérant les différents cas d'applications pour la commande en question. 
   Les fonctions `cat_tar_aux`, `ls_tar_aux`, `mkdir_tar_aux`, `rm_tar_tar`, ont quasiment le même comportement : elles gèrent toutes le cas où il n'y a pas d'argument, le cas où il y a un ou plusieurs arguments et les sous-cas où ces arguments sont des noms ou des chemins. 
   Toutes les commandes qui nous sont demandées d'implémenter pour les .tar peuvent s'exécuter sur plusieurs cibles (`cat fic1 fic2 ... ficn`, `ls dir1 dir2 ... dirn`), sur des chemins ou même pour certaines sans argument. Ces méthodes auxiliaires vont toutes prendre le contenu de `CMD` en paramètre et appliquer la fonction correspondante à chaque élément de `CMD`. Si `CMD` ne contient que le nom de la commande, on agit selon les cas (`cp` toute seule renvoie une erreur alors que `cat` fonctionne). Elles vont aussi gérer l'affichage de certaines erreurs selon les cas.
   
   Nous allons juste détailler le fonctionnement de `cat_tar` et `cat_tar_aux`.
   
 * `int cat_tar(char *file)` : on stocke d'abord dans `current_dir` le répertoire courant puis on parcourt les entêtes du tarball dans lequel on se trouve et à chaque tour de boucle on fait :
   
   Si `current_dir` est un .tar cela veut dire qu'on est au premier niveau de l'arborescence du tarball dans lequel on se trouve (car le cas des tarballs imbriqués n'est à prendre en compte). Dans ce cas, on regarde si le champ `h.name` du header est égal à `file`. Si oui, on lit dans le tarball (avec `read`) le nombre de blocs de 512 octets correspondant à la taille du contenu du fichier correspondant à cet entête en le stockant dans `char res[FILESIZE]`. Puis on affiche `res` sur la sortie standard avec `write`. Sinon on saute le contenu de ce fichier avec `lseek`.
   Si `current_dir` n'est pas un .tar c'est qu'on est dans une arborescence du .tar (exemple : `test.tar/dir1/dir2`), la fonction va alors parcourir le tarball et vérifier pour chaque entête si le nom stocké contient ou non le nom du répertoire dans lequel on se trouve (ici `/dir2`). Si oui, alors on analyse le nom stocké dans cet entête en le transformant avec `make_path` (dans le code : `char **tmp2 = make_path(h.name);`), puis si à l'avant dernier indice se trouve le nom du répertoire courant, et qu'au dernier indice se trouve le nom `file` on va alors afficher le contenu de ce fichier, de la même façon expliquée précédemment.
   
 * `void cat_tar_aux(char **cmd)` : la fonction auxiliaire vérifie si `cmd` contient autre chose que `"cat"` (si la commande a des arguments). Si ce n'est pas le cas, elle va appeler la fonction `execution` avec `{"cat", NULL}` en argument. Sinon, dans une boucle avec `n` nombre de tours où `n = (taille de cmd) - 1`, elle va appeler `cat_tar` sur chaque élément de `cmd`. Deux sous-cas : l'élément est un simple nom de fichier et dans ce cas on appelle juste `cat_tar`. Ou alors l'élément est un chemin vers un fichier et dans ce cas là on stocke le nom du fichier (qui est la dernière string du chemin) dans une variable `file` et on stocke le chemin qui mène vers le répertoire dans lequel se trouve `file` dans une variable `path`. Exemple : si on a `cat dir/dir1/fic`, `file = fic` et `path = dir/dir1`. Ensuite on vérifie si `path` est un chemin valide avec `dir_exists()` puis s'il l'est on fait `cd_aux(path, 1)` (le `1` en argument pour ne pas afficher le message d'erreur de `cd_aux()` si cette dernière échoue), on appelle `cat_tar`, puis on revient là où on était grâce à la valeur de retour du `dir_exists()`. Dans les différents cas, si `cat_tar()`, `cd_aux()` ou `dir_exists()` échouent, un message d'erreur approprié est affiché.
 
 Ainsi, comme dit précédemment, `cat_tar_aux()`, `cat_tar()`, `ls_tar_aux()`, `ls_tar()`, `mkdir_tar_aux()`, `mkdir_tar()`, `rm_tar_aux()` et `rm_tar()` fonctionnent de la même manière à quelques détails près. Nous allons juste expliquer pour chaque fonction `_tar` ce qui s'y fait sachant qu'on sait déjà comment sont traités les arguments lorsque ce sont des chemins ou autres.
 
 * `void ls_tar(int option)` :
  * dans le cas où on est dans un tar mais pas dans dossier de ce tar, on va parcourir le tar dans lequel on se trouve puis afficher le nom contenu dans chaque header.
  * dans le cas où on est dans un dossier dans un tar, on va vérifier pour chaque header si le nom de ce dossier y est (avec la fonction `make_path()`). Si oui, on va afficher le nom qui se trouve après le nom du dossier dans le name du header.
 dans les deux cas on concatène ce qu'on affiche dans un champ `listed` afin de ne pas afficher plusieurs fois le même nom quand on parcourt le tar. Selon la valeur de l'argument `option` on procède ou non à un affichage correspondant à celui de `ls -l` grâce à des fonctions auxiliaires implémentées dans `tsh.c`. Les répertoires sont affichés en bleu pour le style.
 
 * `int mkdir_tar(char *name)` : cette fonction va simplement ajouter une entête à la fin du tarball dans lequel on est, avec le bon nom, le bon typeflag (`'5'`), un dossier n'ayant pas de contenu.
 
 * `int rm_tar(char *file, int errorMsg)` : dans cette méthode on va récupérer la taille du tarball, le parcourir puis lorsqu'on tombe sur le fichier qu'on veut supprimer on va lire dans un `buffer1` la partie précédant ce fichier et son entête, puis la partie après le fichier dans un `buffer2`. On va enusite fermer le tar puis le réouvrir en écrasant le contenu (`O_TRUNC`) puis coller à la suite `buffer1` et `buffer2`. Le fichier est supprimé.
 
 * `int cp_tar(char *file1, char *file2, char *dest, int errorMsg)` et `int cp_tar_aux(char **cmd, int errorMsg)` : `cp_tar_aux()` se contente d'appeler `cp_tar()` avec les bons arguments. `cp_tar_aux()` prend en compte toutes les possibilités d'exécution de la commande `cp` :
  * `cp file1 file2`
  * `cp dir1/.../file1 dir2/.../file2`
  * `cp file1 file2 ... filen dest`
  * `cp dir1/.../file1 dir2/.../file2 ... dirn/.../filen dest`
 On va à chaque fois copier le contenu que ce soit dans un tarball ou non et le coller au bon endroit, que ce soit dans un tarball ou non. On utilise pour cela des buffers de taille `FILESIZE` et on se déplace aux bons endroits quand il le faut grâce à `cd_aux()`.
 
 * `int cp_tar_option(char **dirs)` : implémentation de l'option `-r` de `cp`. `dirs` est un tableau de strings contenant le ou les nom(s) de répertoire(s) à supprimer. Dans cette fonction, s'il y'a une destination en dernier argument, on vérifie son existense avec `is_directory()`, puis s'il existe, on crée un répertoire avec (`mkdir_tar()`) du nom du répertoire qu'on veut copier (en vérifiant également que ce dernier existe). On va ensuite parcourir le tarball dans lequel on est, puis pour le nom de chaque fichier, on vérifie si c'est un fichier contenu dans le répertoire en question. Si oui, on copie ce fichier dans le répertoire que l'on a créé avec un appel à la fonction `cp_tar()`.
 
 * `int rmdir_tar(char *file, int option)` et `void rmdir_tar_aux(char **cmd, int option)` :
 l'argument option correspont au `-r` de `rm` qui est ici implémenté dans la même fonction que `rmdir`.
 
 * `void mv_tar(char **cmd)` : dans cette fonction on fait un appel à `cp_tar_aux()` et à `rm_tar_aux()`. Ainsi le fichier est déplacé au bon endroit. On met les arguments `errorMsg` des deux fonctions à `1` pour ne pas avoir leurs messages d'erreurs en cas d'échec.

 * `void execution_tar(char **cmd)` : cette fonction va décider de quelle fonction appeler (selon ce qui se trouve dans `cmd`) dans le cas où on gère un tarball ou un/des fichier(s) de ce dernier.
   

* *FONCTIONS AUXILIAIRES*
 * `void initialize_globals()` : cette fonction sert simplement à initialiser les variables globales.
   
 * `void arrayfree(char **array)` : libère la mémoire allouée dynamiquement pour un tableau de chaînes de caractères. Libère d'abord la mémoire allouée pour chaque case du tableau, puis la mémoire allouée pour le tableau lui-même.
   
 * `is_tar(char *name)` : sert à vérifier si un fichier est un tarball ou non. La fonction `strstr` va simplement vérifier si `name` contient `.tar` en fin de nom.
   
 * `int is_path(char *name)` : vérifie si `name` est un chemin ou non à l'aide de la fonction `strstr`.
 
 * `int contains_tar(char *name)` : vérifie si `name` contient un nom de tarball ou non à l'aide de la fonction `strstr`.
 
 * `int is_cmd_tar(char *cmd)` : vérifie si une `cmd` est une commande impliquant un tarball ou non. Si on est dans un tarball ou si la `cmd` contient un nom de tarball, la fonction retourne vrai, sinon faux.
   
 * `make_cmd(char *str)` : construit un tableau de chaînes de caractères contenant les différentes chaînes de caractères séparées par un espace dans `str`. Ceci est fait grâce à un appel à `strtok`. À la fin on met la dernière chaîne de caractères à `NULL` pour pouvoir se repérer dans le parcours de ce genre de tableau (voir `strarraylen`).
 
 * `make_path(char *str)` : exactement la même fonction que `make_cmd` à la différence qu'elle traite les chaînes de caractères séparées par `/`. Utile pour gérer le cas où l'utilisateur rentre un chemin en argument d'une commande.
 
 * `int strarraylen(char **strarray)` : retourne la taille d'un tableau de chaînes de caractères.
 
 * `char *getLastPath(char *path)` : cette fonction retourne la dernière sous-chaîne de caractères de REAL_PATH (ici les sous-chaînes sont les chaînes séparées par des "/").
 
 * `char *getCpy(char *str)` : renvoie une copie de `str`. Utilisée quand certaines fonctions modifient les chaînes de caractères qu'elles prennent en argument(s).
   
 * `char *octToRights(char *octal)` : cette fonction convertit des droits écrits en octal en des droits écrits en lettres. Elle est utilisée pour l'implémentation de l'option `-l` de `ls`.
   
 * `char *UTCtime(char *time)` : cette méthode convertit un temps Unix en temps réel, avec quelques abréviations. Elle reprend le format d'affichage du temps pour l'option `-l` de `ls` et est donc utilisée pour reproduire cet affichage.
   
 * `int get_links(char *tar, char *name)` : compte le nombre de liens d'un fichier dans un tarball. Utilisée pour l'option `-l` de `ls`.
   
 * `char *is_redir_or_pipe(char *name)` : vérifie si la ligne de commande entrée par l'utilisateur contient un pipe (`|`) ou une redirection (`<` ou `>` ou `2>`).
   
 * `char **make_redir_or_pipe(char *cmd, char *sep)` : même utilité que `make_cmd()` ou `make_path()` sauf qu'on utilise ici comme séparateur `|` ou `<` ou `>` ou `2>` selon la valeur de `sep`.
