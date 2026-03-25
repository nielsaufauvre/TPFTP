#include "csapp.h"
#include "structure.h"
#include "serveur_enfant.h"

// Envoie le fichier demandé par le client bloc par bloc
void traiter_get(int connfd, request_t *request) {
    response_t response;
    int get_descriptor;
    struct stat file_stat;
    int n;
    char send_buffer[MAXLINE];
    printf("Le client demande le fichier : %s\n", request->nom_fichier);
    if (access(request->nom_fichier, F_OK) == 0) {
        printf("Le fichier '%s' existe.\n", request->nom_fichier);
        get_descriptor = Open(request->nom_fichier, O_RDONLY, S_IRUSR);
        if (request->offset_reprise > 0) {
            lseek(get_descriptor, request->offset_reprise, SEEK_SET);
        }
        fstat(get_descriptor, &file_stat);
        size_t filesize = file_stat.st_size;
        size_t taille_restante = filesize - request->offset_reprise;
        int nb_bloc = (taille_restante + TAILLE_BLOC - 1) / TAILLE_BLOC;
        printf("taille: %zu, nb bloc: %d\n", filesize, nb_bloc);
        response.code = RESPONSE_OK;
        Rio_writen(connfd, &response, sizeof(response_t));
        Rio_writen(connfd, &taille_restante, sizeof(size_t));
        Rio_writen(connfd, &nb_bloc, sizeof(int));
        while ((n = Rio_readn(get_descriptor, send_buffer, TAILLE_BLOC)) > 0) {
            Rio_writen(connfd, send_buffer, n);
        }
        Close(get_descriptor);
        printf("Transfert de %s terminé (%ld octets).\n", request->nom_fichier, filesize);
    } else {
        printf("Le fichier '%s' n'existe pas ou n'est pas accessible.\n", request->nom_fichier);
        response.code = RESPONSE_ERROR;
        Rio_writen(connfd, &response, sizeof(response_t));
    }
}

// implémentation de la commande LS
void traiter_ls(int connfd) {
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
    wait(NULL);
    Rio_writen(connfd, "END\n", 4);
}

// gestion des requêtes RM
void traiter_rm(int connfd, request_t *request) {
    response_t response;
    if (access(request->nom_fichier, F_OK) == 0) {
        response.code = RESPONSE_OK;
    } else {
        response.code = RESPONSE_ERROR;
    }
    pid_t pid_rm = Fork();
    //fils
    if (pid_rm == 0) {
        char *args[] = {"rm", request->nom_fichier, NULL};
        execvp("rm", args);
        exit(0);
    }
    // père
    else {
        wait(NULL);
        Rio_writen(connfd, &response, sizeof(response_t));
    }
}

// Reçoit un fichier envoyé par le client
void traiter_put(int connfd, request_t *request) {
    size_t taille_attendue;
    size_t total_recu = 0;
    int nb_bloc_a_recevoir;
    char buf[MAXLINE];
    rio_t rio;
    Rio_readinitb(&rio, connfd);
    Rio_readnb(&rio, &taille_attendue, sizeof(size_t));
    Rio_readnb(&rio, &nb_bloc_a_recevoir, sizeof(int));
    if (nb_bloc_a_recevoir <= 0) return;
    int readfd = Open(request->nom_fichier, O_WRONLY | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR);
    for (int i = 0; i < nb_bloc_a_recevoir; i++) {
        size_t reste = taille_attendue - total_recu;
        size_t taille_bloc_actuel = (reste < TAILLE_BLOC) ? reste : TAILLE_BLOC;
        int n_read = Rio_readnb(&rio, buf, taille_bloc_actuel);
        if (n_read <= 0) break;
        Rio_writen(readfd, buf, n_read);
        total_recu += n_read;
    }
    printf("Transfer successfully complete.\n");
    Close(readfd);
}

// Traitement d'une requête d'authentification
void traiter_auth(int connfd,request_t *request) {
    //TODO
}

// Traitement des requêtes inconnues
void traiter_unknown() {
    printf("Requete incorrecte.\n");
}

// Gère la logique des serveurs fils
void serveur_enfant(int listenfd, int pipe_lecture, int pipe_ecriture) {
    int connfd;
    struct sockaddr_in clientaddr;
    socklen_t clientlen;
    char client_ip_string[INET_ADDRSTRLEN];
    char client_hostname[MAX_NAME_LEN];
    request_t request;
    char signal_pere;
    char signal_retour = 'k';
    while (1) {
        if (Rio_readn(pipe_lecture, &signal_pere, 1) <= 0) {
            continue;
        }
        clientlen = (socklen_t)sizeof(clientaddr);
        connfd = Accept(listenfd, (SA *)&clientaddr, &clientlen);
        Write(pipe_ecriture, &signal_retour, 1);
        Getnameinfo((SA *)&clientaddr, clientlen,
                    client_hostname, MAX_NAME_LEN, 0, 0, 0);
        Inet_ntop(AF_INET, &clientaddr.sin_addr, client_ip_string,
                  INET_ADDRSTRLEN);
        printf("server connected to %s (%s)\n", client_hostname, client_ip_string);
        while (Rio_readn(connfd, &request, sizeof(request_t)) > 0) {
            switch (request.type) {
            case GET:
                traiter_get(connfd, &request);
                break;
            case LS:
                traiter_ls(connfd);
                break;
            case RM:
                traiter_rm(connfd, &request);
                break;
            case PUT:
                traiter_put(connfd, &request);
                break;
            case AUTH:
                traiter_auth(connfd,&request);
            case UNKNOWN:
                traiter_unknown();
                break;
            default:
                break;
            }
        }
        Close(connfd);
    }
}