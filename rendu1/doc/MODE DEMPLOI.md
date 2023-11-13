Comment faire pour tester le projet ?
=====================================
Tout d'abord il vous faut récuperer l'image Docker de notre projet sur DockerHub : 

`$docker pull samuelgerges/projet-de-sy5-l3-info:img_projetsy5_rendu1`.

Puis de l'exécuter : `$docker run -ti samuelgerges/projet-de-sy5-l3-info:img_projetsy5_rendu1`.

Pour tester ce rendu de mi-semestre, il vous suffit d'exécuter le fichier exécutable `tsh` (`$./tsh`) qui est fourni dans l'image Docker. Pour accéder à cet exécutable il faudra vous déplacer dans le bon répertoire. Pour ce faire : `$cd home/projet-de-sy5/rendu1`. (Un Makefile est fourni au cas où, pour recompiler `tsh.c`).

Tests que vous pouvez faire :
=============================
**Lire le fichier `ARCHITECTURE.MD` avant de lire cette partie / effectuer des tests.**

Pour le moment certaines commandes gérant le cas des tarballs présentent encore des anomalies :

 * `ls` fonctionne seulement sur un tarball (pas sur une arborescence de ce dernier) et à condition d'être allé "dedans" avec un `cd`.
    Exemple de test :
    
   * `home/projet-de-sy5/rendu1 *prompt* cd test.tar`
   * `home/projet-de-sy5/rendu1/test.tar *prompt* ls`
   * tout autre test échouera
                      
 * `cat` fonctionne seulement à condition d'être dans un tarball. En revanche elle ne fonctionnera pas si on se trouve dans une arborescence de ce dernier.
    Exemples : 
   * `home/projet-de-sy5/rendu1/ *prompt* cat test.tar/fic` *ne fonctionne pas*
   * `home/projet-de-sy5/rendu1/ *prompt* cat test.tar/dir/fic` *ne fonctionne pas*
    
    
   * `home/projet-de-sy5/rendu1 *prompt* cd test.tar/dir`
   * `home/projet-de-sy5/rendu1/test.tar/dir *prompt* cat fic` *ne fonctionne pas*
   * `home/projet-de-sy5/rendu1/test.tar/dir *prompt* cat fic1 ... ficn` *ne fonctionne pas*
    
   * `home/projet-de-sy5/rendu1 *prompt* cd test.tar`
   * `home/projet-de-sy5/rendu1/test.tar *prompt* cat fic` *fonctionne*
   * `home/projet-de-sy5/rendu1/test.tar *prompt* cat fic1 ... ficn` *fonctionne*
   * `home/projet-de-sy5/rendu1/test.tar *prompt* cat dir/fic` *fonctionne*
   * `home/projet-de-sy5/rendu1/test.tar *prompt* cat dir1/fic1 ... dirn/ficn` *fonctionne*
              
   * et bien sûr `cat` sans argument fonctionne n'importe où.
              
 * `cp` n'est pas à tester.
 
 * `cd` fonctionne dans tous les cas qu'on peut imaginer et est donc à tester.
 
 * `pwd` est à tester.
 
 * et pour finir, toutes les commandes externes n'incluant pas le traitement de tarballs fonctionnent et sont donc à tester.
 
Bons tests !
     
