#include "csapp.h"
#include "structure.h"
#include "serveur_enfant.h"

// Gère la logique des serveurs fils (Question 3)
void serveur_enfant(int listenfd, int pipe_lecture,int pipe_ecriture) {
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
    char buf[MAXLINE];
    rio_t rio;
    char signal_pere;
    char signal_retour = 'k';

    while (1) {


        // Attente du signal de départ du père
        if (Rio_readn(pipe_lecture, &signal_pere, 1) <= 0) {
            continue;
        }



        // Initialise la structure à chaque itération (Question 3)
        clientlen = (socklen_t)sizeof(clientaddr);

        connfd = Accept(listenfd, (SA *)&clientaddr, &clientlen);

        Write(pipe_ecriture, &signal_retour, 1);



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
                    exit(0);
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
                if (access(request.nom_fichier, F_OK) == 0) {
                    response.code = RESPONSE_OK;
                }
                else {
                    response.code = RESPONSE_ERROR;
                }
                pid_t pid_rm = Fork();
                //fils
                if (pid_rm == 0) {
                    char *args[] = {"rm",request.nom_fichier, NULL};
                    execvp("rm",args);
                    exit(0);
                }
                // père
                else {
                    wait(NULL);
                    Rio_writen(connfd, &response, sizeof(response_t));
                }
                break;
            }

            case PUT: {
                //verification de la reception
                size_t taille_attendue;
                size_t total_recu = 0;
                int nb_bloc_a_recevoir;

                //initialisation de rio
                Rio_readinitb(&rio,connfd);

                //lecture de la taille attendue et du nombre de blocs à recevoir
                Rio_readnb(&rio, &taille_attendue , sizeof(size_t));
                Rio_readnb(&rio, &nb_bloc_a_recevoir , sizeof(int));

                if (nb_bloc_a_recevoir > 0) {
                    //creation du fichier de même nom dans le repertoire
                    int readfd = Open(request.nom_fichier, O_WRONLY | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR);

                    for (int i = 0; i < nb_bloc_a_recevoir; i++) {
                        // calcule la taille réelle du bloc (le dernier peut être plus petit)
                        size_t reste = taille_attendue - total_recu;
                        size_t taille_bloc_actuel = (reste < TAILLE_BLOC) ? reste : TAILLE_BLOC;

                        int n_read = Rio_readnb(&rio, buf, taille_bloc_actuel);
                        if (n_read <= 0) break;

                        Rio_writen(readfd, buf, n_read);
                        total_recu += n_read;
                    }
                    // affichage des statistiques de transfert (Question 7)
                    printf("Transfer successfully complete.\n");
                    Close(readfd);
                }
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