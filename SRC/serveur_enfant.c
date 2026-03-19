#include "csapp.h"



#define MAX_NAME_LEN 256

// Gère la logique des serveurs fils (Question 3)

void serveur_enfant(int listenfd) {
    int connfd;
    struct sockaddr_in clientaddr;
    socklen_t clientlen;
    char client_ip_string[INET_ADDRSTRLEN];
    char client_hostname[MAX_NAME_LEN];

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



        Close(connfd);
    }
}