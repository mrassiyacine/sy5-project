Comment faire pour tester le projet ?
=====================================
Tout d'abord il vous faut récuperer l'image Docker de notre projet sur DockerHub : 

`$docker pull samuelgerges/projet-de-sy5-l3-info:rendu_final`.

Puis de l'exécuter : `$docker run -ti samuelgerges/projet-de-sy5-l3-info:rendu_final`.

(un Dockerfile est fourni au cas où le répertoire `doc` sur le GitLab de notre projet).

Pour tester le projet, il vous suffit de compiler le code source avec la commande `make` puis d'exécuter le fichier exécutable `tsh` (`$./tsh`) qui sera produit. Pour accéder au code source il faudra vous déplacer dans le bon répertoire. Pour ce faire : `$cd home/projet-de-sy5/soutenance`.

Tests que vous pouvez faire :
=============================

À notre connaissance, tout test devrait marcher sans bug à part ces quelques exceptions :

 * l'impémentation de l'option `-r` de `cp` pour le cas des tarballs n'est pas finie et provoque des erreurs de segmentations.
 
 * les dates affichées lors de l'exécution de la commande `ls -l` pour le cas des tarballs présente des dates erronées.
 
 * les combinaisons de commandes avec `|` ne fonctionnent pas pour le cas des tarballs.
 
Bons tests !
     
