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
  int pipes[NB_SLAVES][2];
  int pipes_retour[NB_SLAVES][2];
  int prochain_esclave = 0;
  char signal = 's';
  char signal_retour;

  // Créer le répertoire de stockage s'il n'existe pas (Question 5)
  mkdir(SERVER_DIR, 0777);

  // Se déplace dans ce répertoire (Question 5)
  chdir(SERVER_DIR);

  // Traitant de signal pour SIGINT (Question 4)
  Signal(SIGINT,SIGINT_handler);

  // Port par défaut (Question 3)
  port = PORT;
  listenfd = Open_listenfd(port);

  // Création des NB_PROC serveurs fils (Question 3)
  for (int i = 0; i < NB_SLAVES; i++) {
    // Création d'un tube pour la synchronisation
    pipe(pipes[i]);
    pipe(pipes_retour[i]);

    pid[i] = Fork();

    if (pid[i] == 0) {
      // Permet de remettre le signal SIGINT à son comportement par défaut dans les serveurs fils (Question 4)
      Signal(SIGINT, SIG_DFL);

      // Fermeture du côté écriture du tube dans le fils
      close(pipes[i][1]);
      close(pipes_retour[i][0]);


      // Gère la logique des serveurs fils (Question 3)

      serveur_enfant(listenfd, pipes[i][0], pipes_retour[i][1]);
      // Si le serveur enfant sort (Question 3)
      Close(listenfd);
      exit(0);
    }
    // Fermeture du côté lecture du tube dans le père
    close(pipes[i][0]);
    close(pipes_retour[i][1]);
  }

  fd_set readfds;
  while (1) {
    FD_ZERO(&readfds);
    FD_SET(listenfd, &readfds);

    Select(listenfd + 1, &readfds,NULL, NULL, NULL);

    // On délègue à l'esclave
    Write(pipes[prochain_esclave][1], &signal, 1);

    // ATTENTE : Le maître bloque ici tant que l'esclave n'a pas fait Accept()
    Read(pipes_retour[prochain_esclave][0], &signal_retour, 1);

    printf("[Maître] Esclave %d a pris la main. Passage au suivant.\n", prochain_esclave);
    prochain_esclave = (prochain_esclave + 1) % NB_SLAVES;
  }
  exit(0);
}