
#include "csapp.h"
#include "structure.h"


//Définition du dossier de stockage du client (Question 5)
#define CLIENT_DIR "./client_storage"

//Définition du numéro de port prédéfini (Question 3)
#define PORT 2121

int main(int argc, char **argv)
{
    int clientfd, port;
    char *host, buf[MAXLINE];
    rio_t rio;

    if (argc != 2) {
        fprintf(stderr, "usage: %s <host>\n", argv[0]);
        exit(0);
    }

    // Créer le répertoire de stockage s'il n'existe pas (Question 5)
    mkdir(CLIENT_DIR, 0777);

    // Se déplace dans ce répertoire (Question 5)
    chdir(CLIENT_DIR);

    host = argv[1];
    port = PORT;


    clientfd = Open_clientfd(host, port);
    

    printf("client connected to server OS\n"); 
    
    Rio_readinitb(&rio, clientfd);

    request_t uniqueRequest;

    char buf2[MAXLINE];
    char premier_mot[MAX_NAME_LEN];
    char deuxieme_mot[MAX_NAME_LEN];


    // Gestion de la lecture de la requete (Question 6)
    if (Fgets(buf, MAXLINE, stdin) != NULL){

        sscanf(buf, "%s %s", premier_mot,deuxieme_mot);
        if (strcmp(premier_mot,"GET") == 0) {
            uniqueRequest.type = GET;
        }
        else {
            uniqueRequest.type = UNKNOWN;
        }

        // préparer la requête sous forme de structure request_t (Question 7)
        strncpy(uniqueRequest.nom_fichier, deuxieme_mot, MAX_NAME_LEN - 1);
        uniqueRequest.nom_fichier[MAX_NAME_LEN - 1] = '\0';

        printf("NOM fichier = %s\n",uniqueRequest.nom_fichier);

        Rio_writen(clientfd, &uniqueRequest, sizeof(request_t));

    }

    Close(clientfd);
    exit(0);
}
