#include "csapp.h"
#include "structure.h"
#include "serveur_enfant.h"



// taille d'un bloc envoyé (Question 8)
#define TAILLE_BLOC MAXLINE

// Gère la logique des serveurs fils (Question 3)
void serveur_enfant(int listenfd) {
    int connfd;
    struct sockaddr_in clientaddr;
    socklen_t clientlen;
    char client_ip_string[INET_ADDRSTRLEN];
    char client_hostname[MAX_NAME_LEN];
    request_t request;
    response_t response;
    int get_descriptor;
    struct stat file_stat;
    int n;
    char send_buffer[MAXLINE];

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

        //Gestion de plusieurs requetes par connexion (Question 9)
        while (Rio_readn(connfd, &request, sizeof(request_t)) > 0) {
            // Gestion de la récéption de la requete (Question 6)

            switch (request.type) {
            case GET:{
                printf("Le client demande le fichier : %s\n", request.nom_fichier);
                if (access(request.nom_fichier, F_OK) == 0) {
                    printf("Le fichier '%s' existe.\n", request.nom_fichier);

                    get_descriptor = Open(request.nom_fichier, O_RDONLY, S_IRUSR);
                    fstat(get_descriptor, &file_stat);
                    size_t filesize = file_stat.st_size;
                    int nb_bloc = (filesize + TAILLE_BLOC - 1) / TAILLE_BLOC;
                    printf("taille: %zu, nb bloc: %d\n", filesize, nb_bloc);

                    // envoi du code de retour succès (Question 6)
                    response.code = RESPONSE_OK;
                    Rio_writen(connfd, &response, sizeof(response_t));
                    // envoi de la taille du fichier (Question 7)
                    Rio_writen(connfd, &filesize, sizeof(size_t));
                    // envoi du nombre de blocs à envoyer (Question 8)
                    Rio_writen(connfd, &nb_bloc, sizeof(int));

                    while ((n = Rio_readn(get_descriptor, send_buffer, TAILLE_BLOC)) > 0) {
                        Rio_writen(connfd, send_buffer, n);
                    }

                    Close(get_descriptor);
                    printf("Transfert de %s terminé (%ld octets).\n", request.nom_fichier, filesize);

                } else {
                    printf("Le fichier '%s' n'existe pas ou n'est pas accessible.\n", request.nom_fichier);
                    // envoi du code de retour erreur pour éviter que le client ne reste bloqué en attente (Question 6)
                    response.code = RESPONSE_ERROR;
                    Rio_writen(connfd, &response, sizeof(response_t));
                }
                break;
            }
                // implémentation de la commande LS (Question 15)
            case LS: {

                pid_t pid = Fork();
                // fils
                if (pid == 0) {

                    dup2(connfd, 1);
                    close(connfd);
                    char *args[] = {"ls", NULL};
                    execvp("ls", args);

                }
                // père
                else {
                    wait(NULL);
                    Rio_writen(connfd, "\n", 1);

                }

                break;
            }
            // gestion des requêtes RM (Question 16)
            case RM: {
                pid_t pid_rm = Fork();
                //fils
                if (pid_rm == 0) {
                    char *args[] = {"rm",request.nom_fichier, NULL};

                    execvp("rm",args);
                }
                // père
                else {
                    response.code = RESPONSE_OK;
                    Rio_writen(connfd, &response, sizeof(response_t));
                }


                break;
            }

            case PUT: {

                break;
            }
            case UNKNOWN: {
                printf("Requete incorrecte.\n");
                break;
            }

            default:
                break;
            }

        }

        Close(connfd);
    }
}