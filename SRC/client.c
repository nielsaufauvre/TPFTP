#include "csapp.h"
#include "structure.h"


//Définition du dossier de stockage du client (Question 5)
#define CLIENT_DIR "./client_storage"

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


//handler du sigint pour le client
void SIGINT_handler(int signal){
    exit(0);

}

int main(int argc, char **argv)
{
    int clientfd, port, n;
    char *host, buf[MAXLINE];
    rio_t rio;
    int put_descriptor;
    struct stat file_stat;
    char send_buffer[MAXLINE];

    if (argc != 2) {
        fprintf(stderr, "usage: %s <host>\n", argv[0]);
         exit(0);
    }

    //signal
    Signal(SIGINT,SIGINT_handler);

    // Créer le répertoire de stockage s'il n'existe pas (Question 5)
    mkdir(CLIENT_DIR, 0777);

    // Se déplace dans ce répertoire (Question 5)
    chdir(CLIENT_DIR);

    host = argv[1];
    port = PORT;


    clientfd = Open_clientfd(host, port);
    printf("Clientfd: %d\n",clientfd);

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
                    char fichier_tmp[MAX_NAME_LEN + 20];
                    snprintf(fichier_tmp, sizeof(fichier_tmp), "unfinished_%s", uniqueRequest.nom_fichier);
                   
                    //deplacement dans le repertoire des unfinished
                    mkdir(UNFINISHED_DIR, 0777);
                    chdir(UNFINISHED_DIR);
                   
                    int unfinishedFD =Open(fichier_tmp, O_WRONLY | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR);

                    chdir(".."); //deplacement dans le repertoire parent (client storage)
                  
                    for (int i = 0; i < nb_bloc_a_recevoir; i++) {
                       
                        //memorisation du dernier bloc
                        Rio_writen(unfinishedFD, &i, sizeof(int));
                        //on se place au debut du fichier
                        lseek(unfinishedFD, 0, SEEK_SET);


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

                    //fermeture du fichier tmp d'erreurs
                    Close(unfinishedFD);
                    
                    //supression du fichier temporaire
                    chdir(UNFINISHED_DIR);
                    remove(fichier_tmp);

                    //on revient dans le repertoire principale
                    chdir("..");

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
            //verification du fichier

            if(access(uniqueRequest.nom_fichier,F_OK) == -1 ){
                fprintf(stdout,"ce fichier n'existe pas en local\n");
            }

            else{
                //le fichier existe
            


                put_descriptor = Open(uniqueRequest.nom_fichier,O_RDONLY,S_IRUSR);
                fstat(put_descriptor,&file_stat);
                ssize_t filesize = file_stat.st_size;
                int nb_blocs = (filesize + TAILLE_BLOC - 1) / TAILLE_BLOC;

                //envoie de la taille du fichier
                Rio_writen(clientfd, &filesize, sizeof(size_t));

                //envoie du nombre de blocs
                Rio_writen(clientfd, &nb_blocs, sizeof(int));

                while((n = Rio_readn(put_descriptor,send_buffer, TAILLE_BLOC)) > 0){
                    
                    Rio_writen(clientfd, send_buffer, n);

                
                }

                Close(put_descriptor);
                printf("Transfert du fichier vers le serveur terminé\n");



            }
            


          
        }


        else if (uniqueRequest.type == UNKNOWN) {
            printf("Requête incorrecte.\n");
        }
    }

    Close(clientfd);
    exit(0);
}


