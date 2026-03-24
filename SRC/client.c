#include "csapp.h"
#include "structure.h"


//Définition du dossier de stockage du client (Question 5)
#define CLIENT_DIR "./client_storage"


// taille d'un bloc envoyé (Question 8)
#define TAILLE_BLOC MAXLINE

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

    printf("Connected to %s\n", host);

    Rio_readinitb(&rio, clientfd);

    // initialisation de la requête (Question 7)
    request_t uniqueRequest;
    memset(&uniqueRequest, 0, sizeof(request_t));

    char premier_mot[MAX_NAME_LEN];
    char deuxieme_mot[MAX_NAME_LEN];


    // autorisez plusieurs requetes par connexion (Question 9)
    while (1) {
        // initialise premier et deuxieme mot a chaque tour de boucle
        memset(premier_mot, 0, MAX_NAME_LEN);
        memset(deuxieme_mot, 0, MAX_NAME_LEN);

        // Gestion de la lecture de la requete (Question 6)
        if (Fgets(buf, MAXLINE, stdin) != NULL){

            // préparer la requête sous forme de structure request_t (Question 7)
            sscanf(buf, "%s %s", premier_mot, deuxieme_mot);

            // verifie si le mot est bye pour quitter la connexion (Question 9)
            if (strcmp(premier_mot,"bye") == 0) {
                break;
            }
            uniqueRequest.type = extraire_type(premier_mot);

            strncpy(uniqueRequest.nom_fichier, deuxieme_mot, MAX_NAME_LEN - 1);
            uniqueRequest.nom_fichier[MAX_NAME_LEN - 1] = '\0';


            Rio_writen(clientfd, &uniqueRequest, sizeof(request_t));
        }

        // récupération de la donnée dans le cas de GET (Question 6)
        if (uniqueRequest.type == GET) {

            size_t taille_attendue;
            size_t total_recu = 0;
            int nb_bloc_a_recevoir;
            response_t response;

            // récéption du code de retour du serveur (Question 6)
            Rio_readnb(&rio, &response, sizeof(response_t));


            if (response.code == RESPONSE_ERROR) {
                printf("Erreur : le fichier '%s' n'existe pas sur le serveur.\n", uniqueRequest.nom_fichier);
            } else {
                // récéption de la taille du fichier (Question 7)
                Rio_readnb(&rio, &taille_attendue, sizeof(size_t));
                // récéption du nombre de blocs à envoyer (Question 8)
                Rio_readnb(&rio, &nb_bloc_a_recevoir, sizeof(int));

                if (nb_bloc_a_recevoir > 0) {
                    int readfd = Open(uniqueRequest.nom_fichier, O_WRONLY | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR);

                    // mesure du temps de transfert (Question 7)
                    struct timeval start, end;
                    gettimeofday(&start, NULL);

                    for (int i = 0; i < nb_bloc_a_recevoir; i++) {
                        // calcule la taille réelle du bloc (le dernier peut être plus petit)
                        size_t reste = taille_attendue - total_recu;
                        size_t taille_bloc_actuel = 0;
                        if (reste < TAILLE_BLOC) {
                            taille_bloc_actuel=reste;
                        }else {
                            taille_bloc_actuel = TAILLE_BLOC;
                        }

                        int n = Rio_readnb(&rio, buf, taille_bloc_actuel);
                        if (n <= 0) {
                            break;
                        }

                        Rio_writen(readfd, buf, n);
                        total_recu += n;

                        if (total_recu >= taille_attendue) {
                            break;
                        }
                    }





                    // affichage des statistiques de transfert (Question 7)
                    printf("Transfer successfully complete.\n");

                    Close(readfd);
                }
            }
        }
        // gestion de LS (Question 15)
        else if (uniqueRequest.type == LS) {
            char bufferLS[MAXLINE];

            while (Rio_readlineb(&rio, bufferLS, MAXLINE) > 0) {

                if (strcmp(bufferLS, "\n") == 0) {
                    break;
                }
                printf("%s", bufferLS);
            }
        }

        else if (uniqueRequest.type == RM) {
            //TODO: gerer la reponse de suppression du serveur
            response_t response;
            Rio_readnb(&rio, &response, sizeof(response_t));
            if (response.code == RESPONSE_OK) {
                printf("Suppression du fichier réalisée.\n");

            }
            else {
                printf("Problème lors de la suppression du fichier.\n");
            }

        }

        else if (uniqueRequest.type == PUT) {
            //TODO: gerer la reponse d'ajout du serveur

        }


        else if (uniqueRequest.type == UNKNOWN) {
            printf("Requête incorrecte.\n");
        }
    }

    Close(clientfd);
    exit(0);
}