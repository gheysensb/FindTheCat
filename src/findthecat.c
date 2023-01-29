#include<unistd.h>
#include<stdio.h>
#include<stdlib.h>
#include<stdbool.h>
#include<string.h>
#include<ctype.h>
#include<sys/types.h>
#include<sys/stat.h>
#include<dirent.h>
#include<time.h>
#include<regex.h>
#include "MegaMimes.h"
#include <setjmp.h>
#include <sys/stat.h>

#define TRY do { jmp_buf buf_state; if ( !setjmp(buf_state)) {
#define CATCH } else {
#define ENDTRY }} while(0)

//      ftc starting-point [-option [parametre]] 
//      -test renvoi flag suivant et son param�tre (le print � la fin l�)
//      -name cherche fichiers dont nom = cha�ne, si regex non trait�, nom.extension, si oui, regex
//      + regex
//      -size cherche noms selon taille
//      -date  cherche fichiers dont date dernier acc�s = date param
//      -mime cherche fichiers selon leur type mime. accepte en param le type ou sous type
//      -ctc cherche chaine ou regex dans fichiers et renvoi fichiers contenant
//      -dir recherche par nom sur les dossiers

//-color affiche r�sultats en couleur ???
//-perm chercher fichier par permission
//-link demande a ftc de suivre liens symboliques (lol c koi)
//-threads lance ftc en multithreading, avec nbr de threads en param
//-ou indique de traiter option comme un OU et non ET

// STRUCT DU FTC
typedef struct noeud{
    struct dirent * element;
    char* chemin ;
    struct noeud * next;
}noeud;
typedef struct liste_element{
    struct noeud *first;
}liste_element;

liste_element* liste_create(){
    liste_element * laliste = malloc(sizeof(liste_element));
    laliste->first = NULL;
    return laliste;
}
void liste_add(liste_element* liste ,struct dirent* elt, char* chemin ){
    noeud * add = malloc(sizeof(noeud));
    add->element = elt;
    add->chemin = chemin;
    add->next = NULL;
    if(liste->first == NULL){
        liste->first = add;
    }
    else{
        noeud* noeudencours = liste->first;
        while(noeudencours != NULL){
            if(noeudencours->next == NULL){
                noeudencours->next = add;
                break;
            }
            noeudencours = noeudencours->next;
        }
    }
}
noeud* liste_get(liste_element* liste,int i){
    noeud * noeudencourd = liste->first;
   if(i==0){
        return noeudencourd;
    }
    
    else{
        
        for(int j=1;j<i+1;j++){
            noeudencourd = noeudencourd->next;
        }
        
    } 
    return noeudencourd;
}

void liste_rm(liste_element* liste,int i){
    if(i==0){
        noeud* nouv = liste->first->next;
        free(liste->first->chemin);
        free(liste->first->element);
        free(liste->first);
        
        liste->first = nouv;
    }
    
    else{
        noeud* noeudavant = NULL;
        noeud * noeudencourd = liste->first;
        for(int j=1;j<i+1;j++){
            noeudavant = noeudencourd;
            noeudencourd = noeudencourd->next;
        }
        noeudavant->next = noeudencourd->next;
        free(noeudencourd->chemin);
        free(noeudencourd->element);
        free(noeudencourd);
        
    }
}

void print_chemin(liste_element* liste,bool color){
    noeud* noeudencours = liste->first;
    while(noeudencours !=NULL){
        char * modif = noeudencours->chemin;
        int n = strlen(modif);
        modif[n]='\0';
        if(color){
            if(noeudencours->element->d_type == DT_DIR){
            printf("\033[%sm","33");
        }
        else{
            printf("\033[%sm","34");
        }
        }
        

        printf("%s\n",modif);
        noeudencours=noeudencours->next;
    }
}

void liste_destroy(liste_element* liste){
    if(liste->first != NULL){
        noeud * noeudsupp = liste->first;
        noeud * next = noeudsupp->next;
        while(noeudsupp != NULL){
            free(noeudsupp->chemin);
            free(noeudsupp->element);
            next = noeudsupp ->next;
            free(noeudsupp);
            noeudsupp = next;
        }
    }
    free(liste);
}
// FONCTIONS DU FTC

double find(char** liste,char* elt,int longeur){ // Fonction qui chercher si un élément est dans la liste recherché
    for(int i=0;i<longeur;i++){
        if(strcmp(liste[i],elt)==0){
            return (double) i;
        }
    }
    return -1;
}

void affichage_ligne_de_commande(char** tabOption,char** tabParam,int i,bool color){ // Fonction qui affiche la valeur des flags
    if(color){
        printf("\033[%sm","32");
    }
    printf("La valeur du flag %s est %s\n",tabOption[i],tabParam[i]);
    
}

liste_element* explorateur(struct liste_element* liste,char* chemin){ // elle creer la liste de tout les fichier avec leur chemin
        DIR *dirp;
        struct dirent *dp;
        dirp = opendir(chemin);
        dp = readdir(dirp);
        while(dp != NULL){
            char* add = malloc(sizeof(char)*1000);

            strcpy(add,chemin);
            strcat(add,"/");
            strcat(add,dp->d_name);
            
            if((strcmp(dp->d_name,".")!=0) & (strcmp(dp->d_name,"..")!=0)){
                struct dirent* leadd = malloc(sizeof(struct dirent));
                *leadd  = *dp ;
                liste_add(liste,leadd,add);
                if(dp->d_type == DT_DIR){
                explorateur(liste,add);
            }
            }
            else{
                free(add);
            }
            dp = readdir(dirp);
            
        }
        closedir(dirp);
        return liste;
}

void recherche_name(liste_element* liste, char* name,bool dir) { //regex à la fin à faire
    noeud* noeudencours = liste->first;
    int i = 0;
    //partie regex 
    regex_t regex;
    int reti;
    unsigned char search;
    reti = regcomp(&regex, name,0);
    if(reti){
        printf("Impossible de compiler regex\n");
        exit(1);
    }
    if (dir){
        search = DT_DIR ;
    }
    else{
        search = DT_REG;
    }
    //
    while (noeudencours != NULL) {
        //if (strcmp(noeudencours->element->d_name, name) != 0) {
        reti = regexec(&regex, noeudencours->element->d_name,0,NULL,0);
        if (search == noeudencours->element->d_type){
            if (reti){
            noeudencours = noeudencours->next;
            liste_rm(liste, i);
            i--;
            }
            else
            {
            noeudencours = noeudencours->next;
            }
        
        }
        else{
            noeudencours = noeudencours->next;
            liste_rm(liste, i);
            i--;
        }
        i++;
        
    }
    regfree(&regex);
}



time_t getDateModif(char* path){
    struct stat attr ;
    stat(path,&attr);
    time_t t = attr.st_atime;
    return t;
}

bool compareDate(time_t datemodif, int symbole, int date, int plus){
    time_t rawtime = time(NULL);
    double temps = difftime(rawtime,datemodif);
    if(symbole==0){         //0 = m
        if (plus) { return temps < date * 60; }
        else { return(temps > date * 60); }
    } else if(symbole==1){  //1 = h
        if (plus) { return temps < date * 3600; }
        else { return(temps > date * 3600); }
    } else if(symbole==2){  //2 = j
        if (plus) { return temps < date *24*3600; }
        else { return(temps > date * 24*3600); }
    }
    return true;
}

bool compareSize(off_t size,int sizeparam,int symbole){
    if(symbole ==0){
        return  (size == sizeparam);
    }
    else if(symbole ==1){
        return  (size> sizeparam);
    }
    else {
        return (size < sizeparam);
    }
}

void searchdate(liste_element* liste,int symbole, int date, int plus){ //en gros int symbole pour ne pas avoir à re-parser h, m et j
    noeud* noeudencours = liste->first;
    int i = 0;
    while(noeudencours!=NULL){ 
        if((compareDate( getDateModif(noeudencours->chemin) ,symbole, date, plus))){
            noeudencours = noeudencours->next;
            liste_rm(liste,i);
            i--;
        }
        else{
            noeudencours = noeudencours->next;
        }
        i++;
    }
}




void recherche_date(liste_element* liste, char* temps) {
    //convertir temps en int date et int symbole
    int plusmoins = 0;
    int symbole = -1;
    int date = 0;
    int n = strlen(temps);
    if (temps[n-1] == 'j'){
        symbole = 2;
    } 
    else if(temps[n-1] == 'h'){
        symbole = 1;
    }
    else if(temps[n-1] == 'm'){
        symbole = 0;
    }
    temps[n - 1] = '\0';
    if (temps[0] == '+') {
        plusmoins = 1;
        temps++;
    }
    else if(temps[0] == '-'){
        temps++;
    }
    date = atoi(temps);
    searchdate(liste, symbole, date, plusmoins);
}

off_t getSize(char* path){
    struct stat attr ;
    stat(path,&attr);
    off_t t = attr.st_size;
    return t;

}

void searchsize(liste_element* liste, int size,int symbole){
    noeud* noeudencours = liste->first;
    int i=0;
    while(noeudencours!=NULL){
        if(compareSize(getSize(noeudencours->chemin),size,symbole)==false){
            noeudencours = noeudencours->next;
            liste_rm(liste,i);
            i--;
        }
        else{
            noeudencours = noeudencours->next;
        }
        i++;
    }
}

void rechercheSize(liste_element* liste_fichier, char* Param){
    int n;
    int size;
    int symbole;
    if(Param[0]=='+'){
        symbole =1;
        Param++;
    }
    else if(Param[0]=='-'){// PROBLEME PARSING AU NIVEAU DU -
        symbole = 2;
        Param++;
    }
    else{
        symbole =0;
    }
    n = strlen(Param);
    char extension = Param[n-1];
    switch (extension){// On sépare selon l'extension finale 
        case 'c':
            Param[n-1]='\0';
            size = atoi(Param) ;
            //printf("%d\n",size);
            break;
        case 'k':
            Param[n-1]='\0';
            size = atoi(Param)*1024;
            //printf("%d\n",size);
            break;
        case 'M':
            Param[n-1]='\0';
            size = atoi(Param)*1024*1024;
            //printf("%d\n",size);
            break;
        case 'G':
            Param[n-1]='\0';
            size = atoi(Param)*1024*1024*1024;
            //printf("%d\n",size);
            break;
        default:
            size = atoi(Param);
            break;
    }
    // on a séparé les cas 
    searchsize(liste_fichier,size,symbole);


}

void rechercheMime(liste_element * liste, char* typemime){
    noeud* noeudencours = liste->first;
    int i=0;
    char* atest = malloc(sizeof(char)*1000);
    while(noeudencours != NULL){
        const char* sortie = getMegaMimeType(noeudencours->chemin);
        if(sortie == NULL){
            noeudencours = noeudencours->next;
            liste_rm(liste,i);
            i--;
        }
        else{
            
            strcpy(atest,sortie);
            int n = strlen(typemime);
            atest[n]='\0';
             if(strcmp(atest,typemime)!=0){
            noeudencours = noeudencours->next;
            liste_rm(liste,i);
            i--;
        }
        else{
           noeudencours = noeudencours->next; 
        }
        
        }
        i++;
        
    }
    free(atest);
}

void rechercheCtc(liste_element* liste, char* name){
    noeud* noeudencours = liste->first;
    int i = 0;
    bool isntIn = 1;
    FILE* fp;
    char word[150];
    //partie regex 
    regex_t regex;
    int reti;
    unsigned char search = DT_REG; //car ctc s'applique uniquement aux fichiers
    reti = regcomp(&regex, name,0); //permet de créer notre comparateur regex
    if(reti){
        printf("Impossible de compiler regex\n");
        exit(1);
    }
    //
    while (noeudencours != NULL) {
        if (search == noeudencours->element->d_type){
            isntIn = 1;
            fp = fopen(noeudencours->chemin,"r");
            while(fgets(word,150,fp) && isntIn){
                //ch = fscanf(fp,"%s",word);
                //printf("test ch : %d\n",ch);
                if(regexec(&regex,word,0,NULL,0)==0){
                    //printf("test word : %s\n",word);
                    isntIn = 0;
                }
            }  
            fclose(fp);

            if (isntIn){
                noeudencours = noeudencours->next;
                liste_rm(liste, i);
                i--;
            }
            else
            {
                noeudencours = noeudencours->next;
            }
        }
        else{
            noeudencours = noeudencours->next;
            liste_rm(liste, i);
            i--;
        }
        i++;
        
    }
    regfree(&regex);
}

char* getPerm(char* chemin){
    struct stat st;
    stat(chemin, &st);
    char* permission = malloc(sizeof(char)*4);
    sprintf(permission,"%o",st.st_mode & 0777);
    return permission;
}

void recherchePerm(liste_element* liste, char* perm){
    noeud* noeudencours = liste->first;
    int i = 0;
    while (noeudencours != NULL) {
        char * permission = getPerm(noeudencours->chemin);
        if (strcmp(permission,perm)!=0){
            noeudencours = noeudencours->next;
            liste_rm(liste, i);
            i--;
        }
        else
        {
            noeudencours = noeudencours->next;
        }
        free(permission);
        i++;
    }
}


/* a function that return a bool that correspond to if the parameters is -size or -name or -dir or -mime or -date or -color or -perm or -ctc or -ou*/

bool isOption(char* param){
    if(strcmp(param,"-size")==0){
        return true;
    }
    else if(strcmp(param,"-name")==0){
        return true;
    }
    else if(strcmp(param,"-dir")==0){
        return true;
    }
    else if(strcmp(param,"-mime")==0){
        return true;
    }
    else if(strcmp(param,"-date")==0){
        return true;
    }
    else if(strcmp(param,"-color")==0){
        return true;
    }
    else if(strcmp(param,"-perm")==0){
        return true;
    }
    else if(strcmp(param,"-ctc")==0){
        return true;
    }
    else if(strcmp(param,"-ou")==0){
        return true;
    }
    else if(strcmp(param,"-test")==0){
        return true;
    }
    else if(strcmp(param,"-link")==0){
        return true;
    }
    else if(strcmp(param,"-threads")==0){
        return true;
    }
    else{
        return false;
    }
}

// MAIN

int main(int argc, char* argv[]) {
    
    if (argc < 2){
        printf("\nErreur : nombre d'arguments invalide\n");
        return(EXIT_FAILURE);
    }
    // DECLARATION VARIABLE
    bool isctc = false; //on utilise ces variables afin de savoir le cas où on est daans un ctc, càd qu'il faut considérer les " " avec
    int ictc = 0;
    char** tabOption = malloc(argc * sizeof(char*)); //on malloc un tableau pour les options
    char** tabParams = malloc(argc * sizeof(char*)); // on malloc un tableau pour les paramètres
    for(int i=0 ; i<argc;i++){ // ON INITIALISE LES VALEURS A NULL
        tabOption[i]= "NULL";
        tabParams[i]= "NULL";
    }
    char*  starting_point = argv[1]; // on récupère le point de départ
    int longeur = strlen(starting_point);
    if(starting_point[longeur-1] == '/'){
        starting_point[longeur-1]='\0';
    }
    int posOption = 0; // on repère ou en est dans les options
    bool color = false; // on initialise la couleur à false
    for(int i=2;i<argc;i++){
        if((strcmp(argv[i],"-size")==0 )| (strcmp(argv[i],"-date")==0)){
            isctc = false;
            tabOption[posOption] = argv[i];
            tabParams[posOption] = argv[i+1];
            posOption++;
            i++;
        } // On parse les options et paramètres de la ligne de commande
        else if((strcmp(argv[i],"-ctc")==0)){
            isctc = true;
            ictc = posOption;
            tabOption[posOption] = argv[i];
            tabParams[posOption] = malloc(sizeof(char)*1000);
            strcat(tabParams[posOption], argv[i+1]);
            posOption++;
            i++;
        }
        else if(strcmp(argv[i],"-color")==0){
            isctc = false;
            color = true;
            tabOption[posOption] = argv[i];
            tabParams[posOption] = argv[i+1];
            posOption++;
            i++;
        }
        else if(argv[i][0]=='-'){
            if(isOption(argv[i]) == false){
                printf("Le flag %s n'est pas correct\n",argv[i]);
                return(EXIT_FAILURE);
            }
            isctc = false;
            tabOption[posOption] = argv[i];// Si c'est une option on le mets dans la table option
            posOption++;
        }
        else{
            if(isctc){
                strcat(tabParams[ictc]," ");
                strcat(tabParams[ictc],argv[i]);
            }
            else{
                tabParams[posOption-1] = argv[i];// Sinon dans le paramètre de l'option précèdente
            }
        }
    }// LIGNE DE COMMANDE PARSEE
    liste_element* liste_fichier = liste_create();
    // ON VERIFIE SI ON EST PAS EN MODE TEST
    int indice = find(tabOption,"-test",argc);
    if(indice >=0){
        affichage_ligne_de_commande(tabOption,tabParams,indice+1,color);
    }
    else{
        //FIN VERIF MODE TEST
    //DEBUT VERIF MODE OU
    indice = find(tabOption, "-ou", argc);
    if (indice >= 0) {
        //peut être un bool 0 ou 1 qui entrerai en param de la vérif  à la fin ?
    }

    //FIN VERIF MODE OU
    else{
        
        DIR *directory;
        struct dirent *dep ;
        /* verifier si le dossier starting_point existe*/
        if (opendir(starting_point) == NULL) {
            perror("Le dossier n'existe pas\n");
            return(EXIT_FAILURE);
        }

        char* addcar = malloc(sizeof(char)*1000);

        directory = opendir(starting_point);
        dep = readdir(directory);

        struct dirent * ajout =  malloc(sizeof(struct dirent));
        *ajout = *dep;
        closedir(directory);
        strcpy(addcar,starting_point);
        liste_add(liste_fichier,ajout,addcar);
        explorateur(liste_fichier,starting_point);
        //DEBUT VERIF DES PARAMETRES
        for (int i = 0; i < argc; i++)
        {
            if (strcmp(tabOption[i], "-name") == 0) {
                recherche_name(liste_fichier, tabParams[i],false);
            }
            if (strcmp(tabOption[i], "-date") == 0) {
                recherche_date(liste_fichier, tabParams[i]);
            } 
            if (strcmp(tabOption[i], "-size")==0){
                rechercheSize(liste_fichier,tabParams[i]);
            }
            if (strcmp(tabOption[i],"-mime")==0){
                rechercheMime(liste_fichier,tabParams[i]);
            }
            if (strcmp(tabOption[i],"-dir")==0){
                recherche_name(liste_fichier,tabParams[i],true);
            }
            if (strcmp(tabOption[i],"-ctc")==0){
                rechercheCtc(liste_fichier, tabParams[i]);
            }
            if (strcmp(tabOption[i],"-perm")==0){
                recherchePerm(liste_fichier,tabParams[i]);
            }
            
        }

        //FIN VERIF DES PARAMETRES
        
        print_chemin(liste_fichier,color); 
        // EXPLICATION POUR JUJU
        // A cette étape on a la liste fichier qui contient tout les fichiers / dossier récursivement on va ensuite simplement rexplorer cette liste a chaque
        // flags pour enlever ceux qui correspondent pas à ce flags ( le tour est jouwouer ).
        
    }
    }
    
    free(tabOption);
    free(tabParams);
    liste_destroy(liste_fichier);
    return 0;
}
