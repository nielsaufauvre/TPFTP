

/*les differentes types de requêtes (Question 1)*/

typedef enum {
    GET,
    PUT,
    LS
} typereq_t;

/*structure de données pour formatter les requêtes client vers le serveur (Question 2)*/
typedef struct {
    typereq_t type; 
    char *nom_fichier;
} request_t;



