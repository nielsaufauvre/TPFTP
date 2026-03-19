
#include "csapp.h"
#include "client.h"


//Définition du dossier de stockage du client (Question 5)
#define CLIENT_DIR "./client_storage"


int main(int argc, char **argv)
{
    int clientfd, port;
    char *host, buf[MAXLINE];
    rio_t rio;

    if (argc != 3) {
        fprintf(stderr, "usage: %s <host> <port>\n", argv[0]);
        exit(0);
    }

    // Créer le répertoire de stockage s'il n'existe pas (Question 5)
    mkdir(CLIENT_DIR, 0777);

    // Se déplace dans ce répertoire (Question 5)
    chdir(CLIENT_DIR);

    host = argv[1];
    port = atoi(argv[2]);


    clientfd = Open_clientfd(host, port);
    

    printf("client connected to server OS\n"); 
    
    Rio_readinitb(&rio, clientfd);

    while (Fgets(buf, MAXLINE, stdin) != NULL) {
        Rio_writen(clientfd, buf, strlen(buf));
        if (Rio_readlineb(&rio, buf, MAXLINE) > 0) {
            Fputs(buf, stdout);
        } else { 
            break;
        }
    }
    Close(clientfd);
    exit(0);
}
