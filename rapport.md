##  BARRY MAMADOU OURY ET AUFAUVRE NIELS
# Rapport – Système de transfert de fichiers (FTP)


## 1. Introduction

Dans ce projet, nous avons réalisé un système de transfert de fichiers inspiré du protocole FTP.
L’architecture repose sur trois types d’entités : les clients, un serveur maître (jouant le rôle de répartiteur de charge) et plusieurs serveurs esclaves.

L’objectif est de permettre à plusieurs clients d’effectuer différentes opérations (GET, PUT, LS, RM) tout en répartissant efficacement la charge entre plusieurs serveurs.

---
## 2. Architecture globale du système

Le système suit une architecture distribuée avec un serveur maître et plusieurs serveurs esclaves.

Le serveur maître écoute sur le port 2121. Lorsqu’un client se connecte, il ne traite pas directement la requête mais redirige le client vers un serveur esclave.

Les serveurs esclaves écoutent chacun sur un port différent (ici on a choisi 3000 et 3001 avec 2 serveurs esclaves) et s’occupent réellement du traitement des requêtes.

Le fonctionnement global peut être résumé ainsi :

* Le client se connecte au serveur maître
* Le serveur maître choisit un serveur esclave (selon une politique de tourniquet)
* Le client se reconnecte au serveur esclave
* Toutes les opérations sont ensuite réalisées avec ce serveur esclave

---
## 3. Entités et rôles

### Client

Le client est responsable de l’envoi des requêtes et de la réception des fichiers.

Lors de son lancement, un répertoire personnel est automatiquement créé. Ce répertoire est nommé en fonction de l’utilisateur, sous la forme :

**storage_nomUtilisateur**

Tous les fichiers téléchargés par le client sont stockés dans ce dossier.

À l’intérieur de ce répertoire, on trouve également un sous-dossier :

**unfinished**

Ce dossier contient les fichiers dont le transfert n’a pas été terminé. Il permet de reprendre automatiquement un téléchargement interrompu.

Le client gère également la reprise de transfert en envoyant un offset correspondant à la partie déjà reçue du fichier 

---
### Serveur maître

Le serveur maître écoute sur le port 2121 et maintient la liste des serveurs esclaves 

Lorsqu’un client se connecte :

* il accepte la connexion
* il choisit un serveur esclave selon une politique round-robin
* il envoie au client les informations de connexion de ce serveur esclave (adresse et port)
* il ferme ensuite la connexion

---
### Serveurs esclaves

Les serveurs esclaves sont responsables du traitement des requêtes des clients.

Chaque serveur esclave possède un répertoire de travail nommé :

**serveur_storage**

Ce répertoire contient tous les fichiers accessibles aux clients.

On trouve également un dossier :

**authentification**

Dans ce dossier se trouve un fichier nommé **admins.txt** contenant les identifiants autorisés, sous la forme :

**nom:motdepasse**

Ce fichier est utilisé pour vérifier l’identité des utilisateurs lors des commandes sensibles qui sont PUT et RM.

Les serveurs esclaves gèrent les commandes suivantes :

* GET : téléchargement d’un fichier
* PUT : envoi d’un fichier vers le serveur
* LS : affichage de la liste des fichiers
* RM : suppression d’un fichier

Chaque serveur esclave dispose de la même collection de fichiers 

---
## 4. Connexions et sockets

Le système repose sur des connexions TCP via des sockets.

Le processus de connexion se déroule en plusieurs étapes :

1. Le client se connecte au serveur maître via une socket TCP sur le port 2121
2. Le serveur maître envoie les informations d’un serveur esclave (structure contenant l’hôte et le port) 
3. Le client ferme la première connexion
4. Le client ouvre une nouvelle connexion vers le serveur esclave
5. Toutes les communications se font ensuite entre le client et ce serveur esclave

---

## 5. Protocole de communication

Le protocole repose sur l’échange de structures définies en C.

### Requête client → serveur

La structure request_t contient :

* le type de requête (GET, PUT, LS, RM)
* le nom du fichier
* un offset permettant la reprise de transfert

Cette structure permet au serveur d’identifier clairement la demande.

---
### Réponse serveur → client

La structure response_t contient :

* un code indiquant si la requête a réussi ou échoué

Ce mécanisme permet au client de savoir comment réagir à la réponse 

---
## 6. Exemple de fonctionnement : commande GET

Le déroulement d’un téléchargement est le suivant :

1. Le client envoie une requête GET avec le nom du fichier
2. Le serveur vérifie si le fichier existe
3. Le serveur envoie une réponse (succès ou erreur)
4. En cas de succès, il envoie :

   * la taille du fichier
   * le nombre de blocs à transmettre
5. Le serveur envoie ensuite le fichier bloc par bloc

Le découpage en blocs permet d’éviter de charger entièrement le fichier en mémoire 

---
## 7. Gestion des transferts par blocs

Les fichiers sont découpés en blocs de taille fixe.

Le nombre de blocs est calculé à partir de la taille du fichier.

Ce mécanisme permet :

* une meilleure gestion de la mémoire
* la reprise des transferts interrompus

---
## 8. Reprise de transfert

Le client conserve les informations de transfert dans le dossier unfinished.

Pour chaque fichier incomplet, il enregistre le nombre de blocs déjà reçus.

Lors d’une nouvelle exécution :

* il envoie une requête GET avec un offset
* le serveur reprend l’envoi à partir de cet offset 

---
## 9. Authentification

Les commandes RM et PUT nécessitent une authentification.

Le déroulement est le suivant :

1. Le serveur demande une identification
2. Le client envoie un nom d’utilisateur et un mot de passe
3. Le serveur vérifie ces informations dans le fichier authentification/admins.txt
4. Le serveur renvoie une réponse indiquant le succès ou l’échec

Le format des identifiants est de type :

nom:motdepasse

Ce mécanisme permet de sécuriser les opérations sensibles 

---
## 10. Question 14 – Gestion de panne

Proposition de solution :

Lorsqu’un serveur esclave tombe en panne, il faut mettre en place un mécanisme permettant de détecter cette panne.

Le client connecté à ce serveur doit alors effectuer une reconnexion vers le serveur maître.

Le serveur maître lui fournira les informations d’un autre serveur esclave fonctionnel.

Le client pourra ensuite se connecter à ce nouveau serveur et poursuivre ses opérations.

---
## 11. Conclusion

Ce projet nous a permis de mettre en place un système complet de transfert de fichiers avec :

* une architecture distribuée
* une communication réseau basée sur TCP
* un protocole structuré
* une organisation claire des fichiers côté client et serveur
* des fonctionnalités avancées comme la reprise de transfert et l’authentification

# Remarque

Toutes les questions du projet ont été traitées.  
Les numéros présents dans ce document ne correspondent pas nécessairement à ceux du TP, car ils résultent d’un choix personnel d’organisation du rapport.
