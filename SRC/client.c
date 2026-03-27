#include "csapp.h"
#include "structure.h"
#include <dirent.h>

// Définition du dossier de stockage du client
char client_dir[MAX_NAME_LEN];

// Définition du dossier de stockage des fichiers dont le transfert n'est pas terminé
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
    if (strcasecmp(mot, "AUTH") == 0) {
        return AUTH;
    }
    return UNKNOWN;
}

// Initialise et retourne le répertoire client basé sur le nom d'utilisateur
void init_client_dir() {
    char *username = getlogin();
    if (username == NULL) {
        username = "default_user";
    }
    snprintf(client_dir, sizeof(client_dir), "./storage_%s", username);
    mkdir(client_dir, 0777);
    chdir(client_dir);
}

// Reçoit un fichier depuis le serveur
void recevoir_fichier(rio_t *rio, request_t *req, char *buf) {
    size_t taille_attendue;
    size_t total_recu = 0;
    int nb_bloc_a_recevoir;
    response_t response;
    
    Rio_readnb(rio, &response, sizeof(response_t));
    if (response.code == RESPONSE_ERROR) {
        printf("Erreur : le fichier '%s' n'existe pas sur le serveur.\n", req->nom_fichier);
        return;
    }
    Rio_readnb(rio, &taille_attendue, sizeof(size_t));
    Rio_readnb(rio, &nb_bloc_a_recevoir, sizeof(int));
    if (nb_bloc_a_recevoir <= 0) {
        return;
    }
    //ouverture du fichier en fonction du flag 
    int flags = (req->offset_reprise > 0) ? (O_WRONLY | O_CREAT) : (O_WRONLY | O_CREAT | O_TRUNC);
    int readfd = Open(req->nom_fichier, flags, S_IRUSR | S_IWUSR);
    if (req->offset_reprise > 0) {
        lseek(readfd, req->offset_reprise, SEEK_SET);
    }
    //creation du fichier de transfert unfinished qui sera utile si le transfert n'aboutit pas
    char fichier_tmp[MAX_NAME_LEN + 20];
    snprintf(fichier_tmp, sizeof(fichier_tmp), "unfinished_%s", req->nom_fichier);
    mkdir(UNFINISHED_DIR, 0777);
    chdir(UNFINISHED_DIR);
    int unfinishedFD = Open(fichier_tmp, O_RDWR | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR);
    chdir("..");
   
    //reception des blocs
    for (int i = 0; i < nb_bloc_a_recevoir; i++) {
        size_t reste = taille_attendue - total_recu;
        size_t taille_bloc_actuel = (reste < TAILLE_BLOC) ? reste : TAILLE_BLOC;
        int n = Rio_readnb(rio, buf, taille_bloc_actuel);
        if (n <= 0) {
            break;
        }
        Rio_writen(readfd, buf, n);
        total_recu += n;
        int blocs_recus = i + 1;
        lseek(unfinishedFD, 0, SEEK_SET);
        Rio_writen(unfinishedFD, &blocs_recus, sizeof(int));
        if (total_recu >= taille_attendue) {
            break;
        }
       
        //j'ai ajouté ce bloc de code pour tester si la reprise marche bien
        //il suffit de le decommenter , compiler, executer le code
        //puis refaire l'operation inverse
       /*if(i==nb_bloc_a_recevoir/2) {
            printf("reception interrompue au bloc %d\n",i);
            return ;  //juste pour tester la reprise
        }*/
    }
    //fin de reception des blocs
    printf("Transfer successfully complete.\n");
    Close(unfinishedFD);
    chdir(UNFINISHED_DIR);
    //suppression du fichier temporaire(on en a plus besoin car le transfert s'est bien terminé)
    //ce fichier n'est utile que lorsque le transfert ne s'est pas terminé pour une reprise de transfert
    remove(fichier_tmp);
    //retour au repertoire parent
    chdir("..");
    Close(readfd);
}


int authentifier_client(int clientfd) {
    printf("Veuillez vous identifier\n");
    char buffer[MAXLINE];
    authentification_t auth;
    response_t resp;
    if (Rio_readn(clientfd, buffer, MAXLINE) <= 0) {
        return 0;
    }
    printf("Nom d'utilisateur : ");
    scanf("%s", auth.username);
    printf("Mot de passe : ");
    scanf("%s", auth.password);
    Rio_writen(clientfd, &auth, sizeof(authentification_t));
    if (Rio_readn(clientfd, &resp, sizeof(response_t)) <= 0) {
        return 0;
    }

    if (resp.code == RESPONSE_OK) {
        printf("Authentification réussie.\n");
        return 1;
    } else {
        printf("Échec de l'authentification.\n");
        return 0;
    }
}

// Envoi un fichier vers le serveur
void envoyer_fichier(int clientfd, request_t *req) {
    if (access(req->nom_fichier, F_OK) == -1) {
        printf("Fichier local introuvable.\n");
        return;
    }
    if (!authentifier_client(clientfd)) {
        printf("Authentification non réussie.\n");
        return;
    }
    int fd = Open(req->nom_fichier, O_RDONLY, 0);
    struct stat st;
    fstat(fd, &st);
    size_t filesize = st.st_size;
    int nb_blocs = (filesize + TAILLE_BLOC - 1) / TAILLE_BLOC;

    Rio_writen(clientfd, &filesize, sizeof(size_t));
    Rio_writen(clientfd, &nb_blocs, sizeof(int));

    char send_buf[TAILLE_BLOC];
    int n;
    while ((n = Rio_readn(fd, send_buf, TAILLE_BLOC)) > 0) {
        Rio_writen(clientfd, send_buf, n);
    }
    Close(fd);
    printf("Transfert terminé.\n");
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
void recevoir_rm(int clientfd) {
    if (!authentifier_client(clientfd)) {
        printf("Authentification non réussie.\n");
        return;
    }
    response_t response;
    if (Rio_readn(clientfd, &response, sizeof(response_t)) <= 0) {
        printf("Erreur de lecture de la réponse du serveur.\n");
        return;
    }
    
    if (response.code == RESPONSE_OK) {
        printf("Suppression du fichier réalisée.\n");
    } 
    
    else {
        printf("Problème lors de la suppression du fichier.\n");
    }
}

// Fonction pour réaliser les transferts non terminés
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
            printf("reprise de transfert à partir du bloc %d\n", blocs_recus);
            strcpy(req.nom_fichier, dir->d_name + 11);
            Rio_writen(clientfd, &req, sizeof(request_t));
            char buf[MAXLINE];
            recevoir_fichier(rio, &req, buf);
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
    init_client_dir();
    char *host = argv[1];
    port = PORT;
    clientfd = Open_clientfd(host, port);
    printf("Connected to %s\n", host);
    Rio_readinitb(&rio, clientfd);
    request_t uniqueRequest;
    memset(&uniqueRequest, 0, sizeof(request_t));
    char premier_mot[MAX_NAME_LEN];
    char deuxieme_mot[MAX_NAME_LEN];
    char troisieme_mot[MAX_NAME_LEN];
    reprise_transfert(clientfd,&rio);
    while (1) {
        memset(premier_mot, 0, MAX_NAME_LEN);
        memset(deuxieme_mot, 0, MAX_NAME_LEN);
        memset(troisieme_mot, 0, MAX_NAME_LEN);
        uniqueRequest.offset_reprise = 0;

        if (Fgets(buf, MAXLINE, stdin) != NULL) {
            sscanf(buf, "%s %s %s", premier_mot, deuxieme_mot,troisieme_mot);
            if (strlen(premier_mot) == 0) {
                continue;
            }
            if (strcmp(premier_mot, "bye") == 0) {
                break;
            }
            uniqueRequest.type = extraire_type(premier_mot);

            if (uniqueRequest.type == UNKNOWN) {
                printf("Requête incorrecte.\n");
                continue;
            }

            strncpy(uniqueRequest.nom_fichier, deuxieme_mot, MAX_NAME_LEN - 1);
            uniqueRequest.nom_fichier[MAX_NAME_LEN - 1] = '\0';

            Rio_writen(clientfd, &uniqueRequest, sizeof(request_t));
        }
        if (uniqueRequest.type == GET) {
            recevoir_fichier(&rio, &uniqueRequest, buf);
        }
        else if (uniqueRequest.type == LS) {
            recevoir_ls(&rio);
        }
        else if (uniqueRequest.type == RM) {
            recevoir_rm(clientfd);
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
