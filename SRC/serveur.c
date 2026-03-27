#include "csapp.h"
#include "serveur_enfant.h"
#include "structure.h"

// Définition du dossier de stockage du serveur
#define SERVER_DIR "./serveur_storage"

// host, comme c'est sur la même machine ça sera localhost
#define HOST "localhost"


// Création d'un tableau contenant les pids de tous les serveurs fils
pid_t pid[NB_SLAVES];

//le tableau des ports
int PORTS[NB_SLAVES] = {PORT_SLAVE1, PORT_SLAVE2};

// Création d'un tableau contenant les information des serveurs slaves
//c'est ici qu'on accedera pour prendre leurs informations afin que le client puisse se connecter
slave_t SLAVES[NB_SLAVES];



// init des slaves
void init_slave(slave_t *SLAVES,int num, char *host , int port){
    SLAVES[num].num = num;
    strcpy(SLAVES[num].host, host);
    SLAVES[num].port = port;

}

// handler pour envoyer le signal SIGINT à tous les serveurs fils (Question 4)
void SIGINT_handler(int signal) {
    for (int i = 0; i < NB_SLAVES; i++) {
        Kill(pid[i], SIGINT);
    }
    exit(0);
}

// Crée et initialise les tubes de communication pour un esclave
void creer_tubes(int pipes[][2], int pipes_retour[][2], int i) {
    pipe(pipes[i]);
    pipe(pipes_retour[i]);
}

// Ferme les extrémités du tube non utilisées côté fils
void fermer_tubes_fils(int pipes[][2], int pipes_retour[][2], int i) {
    close(pipes[i][1]);
    close(pipes_retour[i][0]);
}

// Ferme les extrémités du tube non utilisées côté père
void fermer_tubes_pere(int pipes[][2], int pipes_retour[][2], int i) {
    close(pipes[i][0]);
    close(pipes_retour[i][1]);
}

// Crée les NB_SLAVES serveurs fils
void creer_esclaves(int pipes[][2], int pipes_retour[][2]) {
    for (int i = 0; i < NB_SLAVES; i++) {
        init_slave(SLAVES,i,HOST,PORTS[i]);
        creer_tubes(pipes, pipes_retour, i);
        pid[i] = Fork();
        if (pid[i] == 0) {
            int slave_listenfd;

            Signal(SIGINT, SIG_DFL);
            fermer_tubes_fils(pipes, pipes_retour, i);

            slave_listenfd = Open_listenfd(PORTS[i]);
            serveur_enfant(slave_listenfd, pipes[i][0], pipes_retour[i][1]);

            Close(slave_listenfd);
            exit(0);
        }
    }
}

// Délègue une connexion au prochain esclave disponible et attend sa confirmation

//dans le cadre de l'amelioration alors cette fonction va me renvoyer le port de l'esclave
slave_t deleguer_esclave(int num_esclave) {
    //printf("debut du deleguage\n");
    slave_t slave = SLAVES[num_esclave];
    //printf("fin du deleguage\n");
    return SLAVES[num_esclave];

}


// Envoie des informations du slave au client
void send_slave_info(int connfd, slave_t slave) {
    Rio_writen(connfd, &slave, sizeof(slave_t));
}


int main(int argc, char **argv)
{
    int listenfd, port;
    int pipes[NB_SLAVES][2];
    int pipes_retour[NB_SLAVES][2];
    int prochain_esclave = 0;
    slave_t slave;
   
    mkdir(SERVER_DIR, 0777);
    chdir(SERVER_DIR);
    Signal(SIGINT, SIGINT_handler);
   
    port = PORT;
    listenfd = Open_listenfd(port);
    creer_esclaves(pipes, pipes_retour);
    fd_set readfds;
   
    while (1) {
    int connfd;
    struct sockaddr_in clientaddr;
    socklen_t clientlen = sizeof(clientaddr);

    FD_ZERO(&readfds);
    FD_SET(listenfd, &readfds);
    Select(listenfd + 1, &readfds, NULL, NULL, NULL);

    connfd = Accept(listenfd, (SA *)&clientaddr, &clientlen);

    slave = deleguer_esclave(prochain_esclave);
    send_slave_info(connfd, slave);

    Close(connfd);

    prochain_esclave = (prochain_esclave + 1) % NB_SLAVES;
}
}