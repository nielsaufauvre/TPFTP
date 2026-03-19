#include "csapp.h"
#include "serveur_enfant.c"



//Définition du nombre de serveurs (Question 3)
#define NB_PROC 2
//Définition du numéro de port prédéfini (Question 3)
#define PORT 2121

// Création d'un tableau contenant les pid de tous les serveurs fils (Question 4)
pid_t pid[NB_PROC];


// handler pour envoyer le signal SIGINT à tous les serveurs fils (Question 4)
void SIGINT_handler(int signal) {

  for (int i=0 ; i<NB_PROC; i++) {
    Kill(pid[i],SIGINT);
  }
  exit(0);
}


int main(int argc, char **argv)
{
  int listenfd, port;

  Signal(SIGINT,SIGINT_handler);



  port = PORT;
  listenfd = Open_listenfd(port);

  // Création des NB_PROC serveurs fils (Question 3)
  for (int i = 0; i < NB_PROC; i++) {

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



