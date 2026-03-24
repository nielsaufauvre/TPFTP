#define MAX_NAME_LEN 256

//Définition du nombre de serveurs (Question 11)
#define NB_SLAVES 1

// taille d'un bloc envoyé (Question 8)
#define TAILLE_BLOC MAXLINE

//Définition du numéro de port (Question 11)
#define PORT 10000

/*les differentes types de requêtes (Question 1)*/

typedef enum {
    UNKNOWN,
    GET,
    PUT,
    RM,
    LS
} typereq_t;

/*structure de données pour formatter les requêtes client vers le serveur (Question 2)*/
typedef struct {
    typereq_t type; 
    char nom_fichier[MAX_NAME_LEN];
} request_t;

// codes de retour pour les réponses du serveur au client (Question 6)
typedef enum {
    RESPONSE_OK,
    RESPONSE_ERROR
} response_code_t;

// structure pour les réponses du serveur au client (Question 6)
typedef struct {
    response_code_t code;
} response_t;



