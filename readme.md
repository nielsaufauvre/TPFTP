#ATTEENTION :
    La prof avait mentionné que le serveur et client devait être 
    dans 2 repertoires differents (je sais plus trop la raison )

#A traiter
-Reponses -> creer plusieurs types
-Gerer le temps d'envois des fichiers


A faire :
    10,12,13,14,17

. Le rôle du Maître (serveur.c)
Le père ne doit plus faire Pause(). Il doit devenir actif :
Il maintient un index prochain_esclave (0, 1, 2...).
Il appelle Accept().
Il envoie le descripteur de fichier connfd à l'esclave i via un tube (pipe).
Il incrémente l'index (Modulo NB_SLAVES) pour le prochain client.
2. Communication Maître-Esclave (Passage de FD)
   On ne peut pas simplement envoyer un entier dans un pipe pour partager une socket entre processus déjà lancés. Il faut utiliser sendmsg avec des données auxiliaires (SCM_RIGHTS).

Schéma de la solution Round-Robin :