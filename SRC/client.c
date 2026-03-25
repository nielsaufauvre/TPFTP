#include "csapp.h"
#include "structure.h"
#include <dirent.h>

//Définition du dossier de stockage du client (Question 5)
char client_dir[MAX_NAME_LEN];

#define UNFINISHED_DIR "./unfinished"



// Fonction permettant de connaitre le type de la requête
typereq_t extraire_type(char *mot) {
    if (strcasecmp(mot, "GET") == 0) {
        return GET;
    }
    if (strcasecmp(mot, "LS") == 0) {
        return LS;
    }
    if (strcasecmp(mot, "RM") == 0) {
        return RM;
    }
    if (strcasecmp(mot, "PUT") == 0) {
        return PUT;
    }

    return UNKNOWN;
}


// Initialise et retourne le répertoire client basé sur le nom d'utilisateur
void init_client_dir() {
    char *username = getlogin();
    if (username == NULL) {
        username = "default_user";
    }

    // Construire le nom du dossier unique
    snprintf(client_dir, sizeof(client_dir), "./storage_%s", username);

    printf("DIR: %s\n", client_dir);

    // Créer et se déplacer dans le répertoire personnel
    mkdir(client_dir, 0777);
    chdir(client_dir);
}


// Reçoit un fichier depuis le serveur et l'écrit sur le disque (Question 6, 7, 8)
void recevoir_fichier(rio_t *rio, request_t *req, int clientfd, char *buf) {
    size_t taille_attendue;
    size_t total_recu = 0;
    int nb_bloc_a_recevoir;
    response_t response;

    // récéption du code de retour du serveur (Question 6)
    Rio_readnb(rio, &response, sizeof(response_t));

    if (response.code == RESPONSE_ERROR) {
        printf("Erreur : le fichier '%s' n'existe pas sur le serveur.\n", req->nom_fichier);
        return;
    }

    // récéption de la taille du fichier (Question 7)
    Rio_readnb(rio, &taille_attendue, sizeof(size_t));
    // récéption du nombre de blocs à envoyer (Question 8)
    Rio_readnb(rio, &nb_bloc_a_recevoir, sizeof(int));

    if (nb_bloc_a_recevoir <= 0) return;


    int flags = (req->offset_reprise > 0) ? (O_WRONLY | O_CREAT) : (O_WRONLY | O_CREAT | O_TRUNC);
    int readfd = Open(req->nom_fichier, flags, S_IRUSR | S_IWUSR);

    // Et se positionner à la bonne position :
    if (req->offset_reprise > 0) {
        lseek(readfd, req->offset_reprise, SEEK_SET);
    }
    char fichier_tmp[MAX_NAME_LEN + 20];
    snprintf(fichier_tmp, sizeof(fichier_tmp), "unfinished_%s", req->nom_fichier);

    //deplacement dans le repertoire des unfinished
    mkdir(UNFINISHED_DIR, 0777);
    chdir(UNFINISHED_DIR);

    int unfinishedFD = Open(fichier_tmp, O_RDWR | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR);

    chdir(".."); //deplacement dans le repertoire parent (client storage)

    for (int i = 0; i < nb_bloc_a_recevoir; i++) {



        // calcule la taille réelle du bloc (le dernier peut être plus petit)
        size_t reste = taille_attendue - total_recu;
        size_t taille_bloc_actuel = (reste < TAILLE_BLOC) ? reste : TAILLE_BLOC;

        int n = Rio_readnb(rio, buf, taille_bloc_actuel);
        if (n <= 0) break;

        Rio_writen(readfd, buf, n);
        total_recu += n;

        // ecrit le nombre de blocs recus dans le fichier temporaire
        int blocs_recus = i + 1;
        lseek(unfinishedFD, 0, SEEK_SET);
        Rio_writen(unfinishedFD, &blocs_recus, sizeof(int));

        if (total_recu >= taille_attendue) break;
    }

    // affichage des statistiques de transfert (Question 7)
    printf("Transfer successfully complete.\n");

    //fermeture du fichier tmp d'erreurs
    Close(unfinishedFD);

    //supression du fichier temporaire
    chdir(UNFINISHED_DIR);
    remove(fichier_tmp);

    //on revient dans le repertoire principale
    chdir("..");

    Close(readfd);
}


// Envoie un fichier local vers le serveur
void envoyer_fichier(int clientfd, request_t *req) {
    //TODO: gerer la reponse d'ajout du serveur
    //verification du fichier

    if (access(req->nom_fichier, F_OK) == -1) {
        fprintf(stdout, "ce fichier n'existe pas en local\n");
        return;
    }

    //le fichier existe
    int put_descriptor = Open(req->nom_fichier, O_RDONLY, S_IRUSR);
    struct stat file_stat;
    fstat(put_descriptor, &file_stat);
    ssize_t filesize = file_stat.st_size;
    int nb_blocs = (filesize + TAILLE_BLOC - 1) / TAILLE_BLOC;

    //envoie de la taille du fichier
    Rio_writen(clientfd, &filesize, sizeof(size_t));

    //envoie du nombre de blocs
    Rio_writen(clientfd, &nb_blocs, sizeof(int));

    char send_buffer[MAXLINE];
    int n;
    while ((n = Rio_readn(put_descriptor, send_buffer, TAILLE_BLOC)) > 0) {
        Rio_writen(clientfd, send_buffer, n);
    }

    Close(put_descriptor);
    printf("Transfert du fichier vers le serveur terminé\n");
}


// Reçoit et affiche la liste des fichiers du serveur
void recevoir_ls(rio_t *rio) {
    char bufferLS[MAXLINE];

    while (Rio_readlineb(rio, bufferLS, MAXLINE) > 0) {
        if (strcmp(bufferLS, "END\n") == 0) {
            break;
        }
        printf("%s", bufferLS);
    }
}


// Reçoit et affiche le résultat d'une suppression
void recevoir_rm(rio_t *rio) {
    response_t response;
    Rio_readnb(rio, &response, sizeof(response_t));
    if (response.code == RESPONSE_OK) {
        printf("Suppression du fichier réalisée.\n");
    } else {
        printf("Problème lors de la suppression du fichier.\n");
    }
}


// fonction pour réaliser les transferts non terminés
void reprise_transfert(int clientfd,rio_t *rio) {
    DIR *d = opendir("unfinished");
    struct dirent *dir;
    if (d) {
        while ((dir = readdir(d)) != NULL) {
            if (dir->d_name[0] == '.') {
                continue;
            }

            char path[256];
            snprintf(path, sizeof(path), "unfinished/%s", dir->d_name);
            int fd_log = open(path, O_RDONLY);
            int blocs_recus;
            read(fd_log, &blocs_recus, sizeof(int));
            close(fd_log);


            request_t req;
            req.type = GET;
            req.offset_reprise = (long)blocs_recus * TAILLE_BLOC;

            strcpy(req.nom_fichier, dir->d_name + 11);


            Rio_writen(clientfd, &req, sizeof(request_t));
            char buf[MAXLINE];
            recevoir_fichier(rio, &req, clientfd, buf);
        }
        closedir(d);
    }
}

int main(int argc, char **argv)
{
    int clientfd, port;
    char buf[MAXLINE];
    rio_t rio;

    if (argc != 2) {
        fprintf(stderr, "usage: %s <host>\n", argv[0]);
         exit(0);
    }


    // Créer et se déplacer dans le répertoire personnel
    init_client_dir();


    char *host = argv[1];
    port = PORT;

    clientfd = Open_clientfd(host, port);
    printf("Clientfd: %d\n", clientfd);

    printf("Connected to %s\n", host);

    Rio_readinitb(&rio, clientfd);

    // initialisation de la requête (Question 7)
    request_t uniqueRequest;
    memset(&uniqueRequest, 0, sizeof(request_t));

    char premier_mot[MAX_NAME_LEN];
    char deuxieme_mot[MAX_NAME_LEN];


    reprise_transfert(clientfd,&rio);


    // autorisez plusieurs requetes par connexion (Question 9)
    while (1) {
        // initialise premier et deuxieme mot a chaque tour de boucle
        memset(premier_mot, 0, MAX_NAME_LEN);
        memset(deuxieme_mot, 0, MAX_NAME_LEN);

        // Gestion de la lecture de la requete (Question 6)
        if (Fgets(buf, MAXLINE, stdin) != NULL) {

            // préparer la requête sous forme de structure request_t (Question 7)
            sscanf(buf, "%s %s", premier_mot, deuxieme_mot);

            // verifie si le mot est bye pour quitter la connexion (Question 9)
            if (strcmp(premier_mot, "bye") == 0) {
                break;
            }
            uniqueRequest.type = extraire_type(premier_mot);

            strncpy(uniqueRequest.nom_fichier, deuxieme_mot, MAX_NAME_LEN - 1);
            uniqueRequest.nom_fichier[MAX_NAME_LEN - 1] = '\0';

            Rio_writen(clientfd, &uniqueRequest, sizeof(request_t));
        }

        // récupération de la donnée dans le cas de GET (Question 6)
        if (uniqueRequest.type == GET) {
            recevoir_fichier(&rio, &uniqueRequest, clientfd, buf);
        }
        // gestion de LS (Question 15)
        else if (uniqueRequest.type == LS) {
            recevoir_ls(&rio);
        }
        else if (uniqueRequest.type == RM) {
            recevoir_rm(&rio);
        }
        else if (uniqueRequest.type == PUT) {
            envoyer_fichier(clientfd, &uniqueRequest);
        }
        else if (uniqueRequest.type == UNKNOWN) {
            printf("Requête incorrecte.\n");
        }
    }

    Close(clientfd);
    exit(0);
}
