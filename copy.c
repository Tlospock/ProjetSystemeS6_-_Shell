#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include <signal.h>
#include <dirent.h>
#include <string.h>
#include "copy.h"

#define TAILLE_NOM

int copier_dossier(char* nom_origine, char* nom_destination);
int copier_fichier(char* nom_origine, char* nom_destination);



//argv[1] = nom du fichier source
//argv[2] = nom de fichier destination
/*int main(int argc, char *argv[]) {

	int test;
	int ca_marche = 1;
	struct stat* origine = malloc(sizeof(struct stat));
	if(stat(argv[1], origine) == -1){
        printf("Erreur dans la reconnaissance de l'élément d'origine");
        ca_marche = 0;
	}
	if(S_ISDIR(origine->st_mode)) test = copier_dossier(argv[1], argv[2]);
    else test = copier_fichier(argv[1], argv[2]);

    if(test==-1) printf("\nLa copie a echouee.");

	free(origine);
}*/

int copier_dossier(char* nom_origine, char* nom_destination) {
    DIR* d_origine;
    DIR* d_destination;
	int test;
	struct stat* orig_info = malloc(sizeof(struct stat));


    if(stat(nom_origine, orig_info) == -1){
        printf("Erreur dans la verification des protections du dossier d'origine.");
        free(orig_info);
        return -1;
	}

    d_destination = opendir(nom_destination);
	if(d_destination==NULL){
		if(mkdir(nom_destination, orig_info->st_mode)==-1){
            printf("Erreur dans la création d'un dossier.");
            free(orig_info);
            return -1;
		}
		else{
            d_destination = opendir(nom_destination);
		}
	}

   test = closedir(d_destination);
    if(test == -1){
		printf("\nErreur de fermeture du dossier de destination.");
		free(orig_info);
		return -1;
	}

	if(chmod(nom_destination, orig_info->st_mode) == -1){
        printf("Erreur dans la gestion des protections du dossier de destination.");
        free(orig_info);
        return -1;
	}
    free(orig_info);

	d_origine = opendir(nom_origine);
	if(d_origine==NULL){
		printf("\nErreur : erreur d'ouverture du dossier d'origine.");
		return -1;
	}


    //Apparemment il faut commencer par l'appeler deux fois dans le vide.

    struct dirent* prochain_fichier = malloc(sizeof(struct dirent));
     //Apparemment il faut commencer par l'appeler deux fois dans le vide.
    prochain_fichier = readdir(d_origine);
    prochain_fichier = readdir(d_origine);
    char* lieu_destination = malloc(600*sizeof(char));
    char* lieu_origine = malloc(600*sizeof(char));
    struct stat* origine = malloc(sizeof(struct stat));

    do{
        prochain_fichier = readdir(d_origine);
        if(prochain_fichier != NULL){
        strcpy(lieu_origine, nom_origine);
                strcat(lieu_origine, "/");
                strcat(lieu_origine, prochain_fichier->d_name);
                strcpy(lieu_destination, nom_destination);
                strcat(lieu_destination, "/");
                strcat(lieu_destination, prochain_fichier->d_name);
            if(stat(lieu_origine, origine) == -1){
                printf("Erreur de la reconnaissance d'un fichier/dossier");
            }
            if(S_ISDIR(origine->st_mode)){ //Si c'est un fichier

                if(copier_dossier(lieu_origine, lieu_destination) == -1){
                    printf("\nErreur de copie d'un dossier");
                }
            }
            else{ //Si c'est un dossier

                if(copier_fichier(lieu_origine, lieu_destination) == -1){
                    printf("\nErreur de copie d'un fichier");
                }

            }
        }
    }while(prochain_fichier != NULL);

    free(lieu_destination);
    free(lieu_origine);
    free(prochain_fichier);
    free(origine);

    test = closedir(d_origine);
	if(test == -1){
		printf("\nErreur de fermeture du dossier d'origine.");
		return -1;
	}



	return 0;
}

int copier_fichier(char* nom_origine, char* nom_destination){

	int f_origine, f_destination;
	int test;
	char* c;
	struct stat* orig_info = malloc(sizeof(struct stat));

	c = malloc(sizeof(char));


	f_origine = open(nom_origine, O_RDONLY, S_IRUSR | S_IRGRP | S_IROTH);
	if(f_origine==-1){
		printf("\nErreur : fichier d'origine inconnu.");
        free(c);
        free(orig_info);
		return -1;
	}
	if(fstat(f_origine, orig_info)==-1){
        printf("Erreur dans la verification des protections du fichier d'origine.");
        free(c);
        free(orig_info);
        return -1;
	}


	f_destination = open(nom_destination, O_WRONLY | O_CREAT, S_IWUSR | S_IWGRP | S_IWOTH);
	if(f_destination==-1){
		printf("\nErreur : fichier de destination inconnu.");
        free(c);
        free(orig_info);
		return -1;
	}

	if(fchmod(f_destination, orig_info->st_mode) == -1){
        printf("Erreur dans la gestion des protections du fichier de destination.");
        free(c);
        free(orig_info);
        return -1;
	}

	int r_test = 1, w_test = 1;
	while(r_test==1){
		r_test = read(f_origine, c, 1);
		if(r_test>0) w_test = write(f_destination, c, 1);
		if(r_test == -1){
			printf("\nErreur de lecture du fichier d'origine.");
            free(c);
            free(orig_info);
			return -1;
		}
		if(w_test == -1){
			printf("\nErreur d'écriture dans le fichier de destination.");
            free(c);
            free(orig_info);
			return -1;
		}
	}

	test = close(f_origine);
	if(test == -1){
		printf("\nErreur de fermeture du fichier d'origine.");
        free(c);
        free(orig_info);
		return -1;
	}

	test = close(f_destination);
	if(test == -1){
		printf("\nErreur de fermeture du fichier de destination.");
		free(c);
        free(orig_info);
		return -1;
	}


	free(c);
	free(orig_info);
	return 0;

}
