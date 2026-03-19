
#include "csapp.h"
#include "structure.h"


//Définition du dossier de stockage du client (Question 5)
#define CLIENT_DIR "./client_storage"

//Définition du numéro de port prédéfini (Question 3)
#define PORT 2121

// Fonction permettant de connaitre le type de la requête
typereq_t extraire_type(char *mot) {
    if (strcasecmp(mot, "GET") == 0) {
        return GET;
    }
    if (strcasecmp(mot, "LS") == 0) {
        return LS;
    }
    if (strcasecmp(mot, "PUT") == 0) {
        return PUT;
    }

    return UNKNOWN;
}


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


        // préparer la requête sous forme de structure request_t (Question 7)
        sscanf(buf, "%s %s", premier_mot,deuxieme_mot);

        uniqueRequest.type = extraire_type(premier_mot);



        strncpy(uniqueRequest.nom_fichier, deuxieme_mot, MAX_NAME_LEN - 1);
        uniqueRequest.nom_fichier[MAX_NAME_LEN - 1] = '\0';

        printf("NOM fichier = %s\n",uniqueRequest.nom_fichier);

        Rio_writen(clientfd, &uniqueRequest, sizeof(request_t));

    }

    // récupération de la donnée dans le cas de GET (Question 6)
    char buffer3[MAXLINE];
    size_t taille_attendue;
    size_t total_recu=0;
    int readfd = Open(uniqueRequest.nom_fichier, O_WRONLY | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR);
    Rio_readn(clientfd, &taille_attendue, sizeof(size_t));
    int n;



    while (total_recu < taille_attendue) {
        n = Rio_readnb(&rio, buffer3, MAXLINE);
        Rio_writen(readfd, buffer3, n);
        total_recu += n;
    }

    printf("Transfert de %s terminé (%ld octets).\n", uniqueRequest.nom_fichier, taille_attendue);

    Close(readfd);

    Close(clientfd);
    exit(0);
}
