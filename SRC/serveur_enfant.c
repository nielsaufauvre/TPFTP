#include "csapp.h"
#include "structure.h"

#define MAX_NAME_LEN 256

// Gère la logique des serveurs fils (Question 3)

void serveur_enfant(int listenfd) {
    int connfd;
    struct sockaddr_in clientaddr;
    socklen_t clientlen;
    char client_ip_string[INET_ADDRSTRLEN];
    char client_hostname[MAX_NAME_LEN];
    request_t request;
    int get_descriptor;

    while (1) {


        // Initialise la structure à chaque itération (Question 3)
        clientlen = (socklen_t)sizeof(clientaddr);

        connfd = Accept(listenfd, (SA *)&clientaddr, &clientlen);



        Getnameinfo((SA *) &clientaddr, clientlen,
                  client_hostname, MAX_NAME_LEN, 0, 0, 0);


        Inet_ntop(AF_INET, &clientaddr.sin_addr, client_ip_string,
                INET_ADDRSTRLEN);

        printf("server connected to %s (%s)\n", client_hostname,
             client_ip_string);



      // Gestion de la récéption de la requete (Question 6)
      if (Rio_readn(connfd, &request, sizeof(request_t)) > 0) {

        switch (request.type) {
          case GET:
          printf("Le client demande le fichier : %s\n", request.nom_fichier);
          if (access(request.nom_fichier, F_OK) == 0) {
            printf("Le fichier '%s' existe.\n", request.nom_fichier);
            get_descriptor = Open(request.nom_fichier,O_RDONLY,S_IRUSR);

          } else {
            printf("Le fichier '%s' n'existe pas ou n'est pas accessible.\n", request.nom_fichier);
          }
          break;

        case UNKNOWN:
          printf("Requete incorrecte.\n");
          break;


        default:
          break;
        }

      }









        Close(connfd);
    }
}