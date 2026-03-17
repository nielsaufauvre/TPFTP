#include "csapp.h"

#define MAX_NAME_LEN 256
#define NB_PROC 2
#define PORT_ECOUTE 2121  //port d'écoute predefini

void echo(int connfd);


int main(int argc, char **argv)
{
    int listenfd, connfd, port;
    socklen_t clientlen;
    struct sockaddr_in clientaddr;
    char client_ip_string[INET_ADDRSTRLEN];
    char client_hostname[MAX_NAME_LEN];
    pid_t childpid;
    pid_t tableau_pid[NB_PROC];
    
    if (argc != 2) {
        fprintf(stderr, "usage: %s <port>\n", argv[0]);
        exit(0);
    }
    port = PORT_ECOUTE;
    
    clientlen = (socklen_t)sizeof(clientaddr);

    listenfd = Open_listenfd(port);

    //apres la connexion il le passe a un de ses fils
    while (1) {
        
        for(int i=0; i<NB_PROC;i++){
            childpid = FORK();
            if(childpid==0){//fils
                //vas faire quelque chose
                //on met le fils en attente de connexion 
                //le fils reçoit la connexion et fait son traitement puis revient
                //au debut ici
                while(1){
                    //le fils boucle ici
                    ///on met le fils en attente de connexion 
                    //le fils reçoit la connexion et fait son traitement puis revient
                    //au debut ici


                    

                }

                //break; //pour casser la boucle à la fin
            }
            //pere
            //on ajoute le pid dans du fils dans le tableau
            tableau_pid[i] = childpid;
            
        }

        //le père va attendre une demande de connextion et passe à un de ses fils libre

         connfd = Accept(listenfd, (SA *)&clientaddr, &clientlen);


        //fin de creation , le pere attends tous les processus fils qu'ils se terminent

        for(int i=0;i<NB_PROC;i++){
            wait(NULL);
            //je ne pense pas que ça soit necessaire car les fils
            //ne se termine pas , donc faudra gérer les sigint quand 
            //on veut tuer le serveur
        }

        

        Getnameinfo((SA *) &clientaddr, clientlen,
                    client_hostname, MAX_NAME_LEN, 0, 0, 0);
        
        Inet_ntop(AF_INET, &clientaddr.sin_addr, client_ip_string,
                  INET_ADDRSTRLEN);
        
        printf("server connected to %s (%s)\n", client_hostname,
               client_ip_string);

        echo(connfd);
        Close(connfd);
    }
    exit(0);
}

