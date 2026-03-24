#include "csapp.h"
#include "serveur_enfant.h"
#include "structure.h"






//Définition du dossier de stockage du serveur (Question 5)
#define SERVER_DIR "./serveur_storage"

// Création d'un tableau contenant les pid de tous les serveurs fils (Question 4)
pid_t pid[NB_SLAVES];


// handler pour envoyer le signal SIGINT à tous les serveurs fils (Question 4)
void SIGINT_handler(int signal) {

  for (int i=0 ; i<NB_SLAVES; i++) {
    Kill(pid[i],SIGINT);
  }
  exit(0);
}


int main(int argc, char **argv)
{
  int listenfd, port;


  //compte le nombre de connexion
  int compteur_connexion = 0;

  // Créer le répertoire de stockage s'il n'existe pas (Question 5)
  mkdir(SERVER_DIR, 0777);

  // Se déplace dans ce répertoire (Question 5)
  chdir(SERVER_DIR);

  // Traitant de signal pour SIGINT (Question 4)
  Signal(SIGINT,SIGINT_handler);


  // Port 2121 par défaut (Question 3)
  port = PORT;
  listenfd = Open_listenfd(port);

  // Création des NB_PROC serveurs fils (Question 3)
  for (int i = 0; i < NB_SLAVES; i++) {

    pid[i] = Fork();



    if (pid[i] == 0) {

      // Permet de remettre le signal SIGINT à son comportement par défaut dans les serveurs fils (Question 4)
      Signal(SIGINT, SIG_DFL);


      // Gère la logique des serveurs fils (Question 3)
      serveur_enfant(listenfd);

      // Si le serveur enfant sort (Question 3)
      Close(listenfd);

    }

  }



  while (1) {

    // Permet que le père ne consomme pas de CPU (Question 3)
    Pause();
  }
  exit(0);
}
