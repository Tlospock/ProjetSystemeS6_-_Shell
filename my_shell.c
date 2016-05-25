
/*Compiler: gcc my_shell.c copy.c -o shell*/

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <signal.h>
#include <sys/wait.h>
#include <dirent.h>
#include <time.h>
#include <fcntl.h>
#include <utime.h>
#include "copy.h"


#define TAILLE_MAX_COMMANDE 600
#define TAILLE_MAX_CHEMIN 1024
#define TAILLE_MAX_HISTORY 11
#define TAILLE_MAX_PATH 7000
#define TAILLE_MAX_NOM_FICHIER 150

extern char** environ;
int compteurCommandeHistorique;

void invite_commande();
void lire_commande(char* commande);
char** decouper(char* commande, char** cmdDecoupee);
void parseCommande(char* commande, char** history);
int executer_commande(char* commande, char** history);
void addHistory(char* commande, char** history);
int historyCmd(char* commande, char** cmdDecoupee, char** history);
void cd(char* finchemin, DIR* repertoire);
void touch(char** cmdDecoupee);
void cat(char** cmdDecoupee);
char* findPath(char* commande);


void invite_commande()
{
        char userName[1024];

        /*Opérations pour récupérer le userName*/
        sprintf(userName, "%s", getlogin());
        if (userName == NULL)
        {
                fprintf(stderr, "Get of user information failed.\n");
                exit(EXIT_SUCCESS);
		}
		strcat(userName, "@");			/*On ajoute le @*/


        /*Opération pour récupérer le nom de la machine*/
        char hostname[1024];
        hostname[1023] = '\0';
        gethostname(hostname, 1023);

        /*On vire tout ce qui est après le premier point de l'adresse locale*/
        int i=0;
        while(hostname[i]!='.')
        {
                ++i;
        }
        hostname[i] = '\0';

        strcat(userName, hostname);
        strcat(userName, " ");

        /*Opérations pour récupérer le repertoire courant*/
        char buff[1024];
        if(getcwd(buff, sizeof(buff)) == NULL)
                exit(EXIT_SUCCESS);
        strcat(userName, buff);

        fprintf(stdout, "%s: >> ", userName);
}


void lire_commande(char* commande)
{
	if(!fgets(commande, TAILLE_MAX_COMMANDE, stdin)){
        fprintf(stdout, "\n");
        exit(EXIT_SUCCESS);
	}
	char *c = strchr(commande, '\n');
	if(c)
		*c=0;
}

char** decouper(char* commande, char** cmdDecoupee)
{
	int i=0, nbEspace=0, j=0;
	while(commande[i] == ' ')
		++i;

	for(i; commande[i]!='\0';++i)
	{
		if(commande[i] == ' ')
		{
			++nbEspace;
			while(commande[i+1]==' ')
				++i;
		}

	}
	if(commande[i-1] != ' ' && commande[i-1]>0 && commande[i-1]<256)
	{
		++nbEspace;
	}
	if(nbEspace==0)
	{
		++nbEspace;
    }
    cmdDecoupee = (char**)realloc(cmdDecoupee, (nbEspace+1)*sizeof(char*));
    for(i=0; i<nbEspace+1; ++i)
    {
        cmdDecoupee[i] = (char*)malloc((1+TAILLE_MAX_COMMANDE)*sizeof(char));
    }
    cmdDecoupee[nbEspace] = NULL;
	i=0;
	j=0;																/*N° de l'argument*/
	int k=0;
															/*Cran de la chaine découpée*/
	while(commande[i] == ' ')
		++i;

	while(commande[i]!='\0')
	{
        /*fprintf(stderr, "boucle");*/
		if(commande[i] == ' ' && j<nbEspace-1)
		{
            		cmdDecoupee[j][k] = '\0';
			++j;
			k=0;
			while(commande[i] == ' ')
				++i;
		}
		else if(commande[i] != ' ')
		{
			cmdDecoupee[j][k] = commande[i];
			++k;
			++i;

		}
		else
		{
            ++i;
		}

	}
    if(commande[i] == '\0')
        cmdDecoupee[j][k] = '\0';
    return cmdDecoupee;

}


void parseCommande(char* commande, char** history)
{

    int i, j, k, redirige = 0, status;
    ssize_t n;
    FILE* fichier;
    char* nomFichier;
    int estInterne = 0;
    
    //char* buffer = (char*)malloc(TAILLE_MAX_COMMANDE*sizeof(char));
    int nPipe = 0;
    
    for(i=0; commande[i] != '\0'; i++){
        if(commande[i]=='|')
            ++nPipe;
	if(commande[i] == '>' || commande[i] == '<')
	{
	    nomFichier = (char*)malloc(sizeof(char)*TAILLE_MAX_NOM_FICHIER);
	    j=0;
	    if(commande[i] == '>')
		redirige = 1;
	    else
		redirige = 2;
	    commande[i] = '\0';
	    if(commande[i+1] == '>' && redirige == 1){
		i++;
		redirige = 3;
	   }
	    while(commande[i+1]==' ')
		i++;
	    while(commande[i+1] != ' ' && commande[i+1] != '|' && commande[i+1] != '\0')
	    {
		nomFichier[j] = commande[i+1];
		commande[i+1] = '\0';
		j++;
		i++;
	    }
	}
		
    }

    if(redirige == 2) // redirecton <
    {
	if((fichier = fopen(nomFichier, "r")) == NULL)
	{
	    fprintf(stderr, "Erreur d'ouverture ou de création du fichier\n");
	    return;
	}
	i=0;
	while(commande[i] != '\0')
	    ++i;
	commande[i] = ' ';
	char c = 'a';
	do
	{
	    ++i;
	    c=fgetc(fichier);
	    if(c != '\n')
		commande[i] = c;
	}while(c != EOF);
	++i;
	commande[i] = '\0';
    }
	
    char** miniCmd = (char**)malloc(sizeof(char*) * (1+nPipe));
    for(i=0; i<nPipe+1; i++)
        miniCmd[i] = (char*)malloc(sizeof(char)*TAILLE_MAX_COMMANDE);
    j=0;
    k=0;
    for(i=0; commande[i] != '\0'; i++)
    {
	
        if(commande[i] == '|' || commande[i] == '\0')
        {
            miniCmd[j][k] = '\0';
            ++j;
            k=0;
        }
	
        else{
            miniCmd[j][k] = commande[i];
            k++;
        }
    }
	miniCmd[j][k] = '\0';

    if(nPipe == 0){
	char** cmdDecoupee = NULL;
	cmdDecoupee = decouper(commande, cmdDecoupee);
	if((strcmp(cmdDecoupee[0], "exit") == 0) || (strcmp(cmdDecoupee[0], "quit") == 0))
	{
	    free(cmdDecoupee);
	    free(miniCmd[0]);
	    free(miniCmd);
	    if(redirige != 0)
		free(nomFichier);
	    if(redirige == 2)
		fclose(fichier);
	    exit(EXIT_SUCCESS);
	}
	if((strcmp(cmdDecoupee[0], "cd") == 0))
	{
	    DIR* repertoire;
	    if(cmdDecoupee[1]== NULL)	/*Si l'utilisateur a oublié un parametre*/
	    {
		fprintf(stderr, "Il manque un parametre à cette fonction!\n");
		free(cmdDecoupee);
		return;
	    }
	    cd(cmdDecoupee[1], repertoire);
	    return;
	}
    }
	
    
    for(i=0; i<nPipe+1; i++)
    {
        int tuyau[2];
        int redir;
	int estInterne = 0;
        pipe(tuyau);
        int pid = fork();
	

        if(pid == 0)
        {
	
	dup2(redir, STDIN_FILENO);
	if(i!=nPipe){
	    dup2(tuyau[1], STDOUT_FILENO);
	}
	else if(redirige == 1){
	    if((fichier = fopen(nomFichier, "w")) == NULL)
	    {
	        fprintf(stderr, "Erreur d'ouverture ou de création du fichier\n");
	        return;
	    }  
	    dup2(fileno(fichier), STDOUT_FILENO);
	}

	

	else if(redirige == 3){
	    if((fichier = fopen(nomFichier, "a+")) == NULL)
	    {
	        fprintf(stderr, "Erreur d'ouverture ou de création du fichier\n");
	        return;
	    }  
	    dup2(fileno(fichier), STDOUT_FILENO);
	}
	
	close(tuyau[0]);
	estInterne = 0;
        estInterne = executer_commande(miniCmd[i], history);
	if(redirige != 0) 
	    fclose(fichier);
	if(estInterne != 0)
	{
	    exit(EXIT_SUCCESS);
	}
    }

    else
    {
        do
        {
  	    if(estInterne == -1)
		exit(EXIT_SUCCESS);
            waitpid(-1, &status, 0);
            if (WIFEXITED(status)) {
                //fprintf(stderr, "terminé, code=%d\n", WEXITSTATUS(status));
            }
	    else if (WIFSIGNALED(status)) {
                fprintf(stderr, "tué par le signal %d\n", WTERMSIG(status));
            } 
	    else if (WIFSTOPPED(status)) {
                fprintf(stderr, "arrêté par le signal %d\n", WSTOPSIG(status));
            }
	    else if (WIFCONTINUED(status)) {
                fprintf(stderr, "relancé\n");
            }
        }while (!WIFEXITED(status) && !WIFSIGNALED(status));
	close(tuyau[1]);
	redir = tuyau[0];
    }
    }
    for(i=0; i<nPipe+1; i++)
        free(miniCmd[i]);
    free(miniCmd);
    if(redirige != 0)
    {
	free(nomFichier);
    }

    /*sleep(1);*/
    fprintf(stdout, "\n");
}


int executer_commande(char* commande, char** history)
{
	char** cmdDecoupee = NULL;
	cmdDecoupee = decouper(commande, cmdDecoupee);
	DIR* repertoire;

	if(cmdDecoupee[0][0] == '!' && cmdDecoupee[0][1] != '\0')
	{

		cmdDecoupee[0][0] = '0';
		if(cmdDecoupee[0][1] == '!')
			strcpy(commande, history[compteurCommandeHistorique-2]);
		else if(atoi(cmdDecoupee[0])>0 && atoi(cmdDecoupee[0])<compteurCommandeHistorique && compteurCommandeHistorique - atoi(cmdDecoupee[0]) >=0)	/*On vérifie que le commande[i] == '|'mbre est bien dans le range défini*/
		{
			strcpy(commande, history[atoi(cmdDecoupee[0])]);
		}
		free(cmdDecoupee);
		cmdDecoupee = decouper(commande, cmdDecoupee);
	}

	if(strcmp(cmdDecoupee[0], "history") ==0)
	{
		while(strcmp(cmdDecoupee[0], "history") ==0)
		{
			int command = 0;
			command = historyCmd(commande, cmdDecoupee, history);
			if(command == 0)
			{
				return 1;
			}
			cmdDecoupee = decouper(commande, cmdDecoupee);
			free(cmdDecoupee);
		}
	}



    else if(strcmp(cmdDecoupee[0], "copy") == 0)
    {
        int test;
        int ca_marche = 1;
        struct stat* origine = malloc(sizeof(struct stat));
        if(stat(cmdDecoupee[1], origine) == -1){
            printf("Erreur dans la reconnaissance de l'élément d'origine");
            ca_marche = 0;
        }
        if(S_ISDIR(origine->st_mode)) test = copier_dossier(cmdDecoupee[1], cmdDecoupee[2]);
        else test = copier_fichier(cmdDecoupee[1], cmdDecoupee[2]);

        if(test==-1) printf("\nLa copie a echouee.");

        free(origine);
        free(cmdDecoupee);
        return 1;
    }

    else if(strcmp(cmdDecoupee[0], "touch") == 0)
    {
		if(cmdDecoupee[1] == NULL)
		{
			fprintf(stdout, "Erreur, il manque un parametre a cette fonction.\n");
		}
		else
			touch(cmdDecoupee);

		free(cmdDecoupee);
		return 1;
	}

	else if(strcmp(cmdDecoupee[0], "cat") == 0)
	{
		cat(cmdDecoupee);

		free(cmdDecoupee);
		return 1;
	}

	execv(findPath(cmdDecoupee[0]), cmdDecoupee);
	free(cmdDecoupee);
	return 1;

}

void addHistory(char* commande, char** history)
{
	int i=0;
	while(i<TAILLE_MAX_HISTORY && history[i][0] != '\0')
		++i;

	if(i>=TAILLE_MAX_HISTORY)		/*Le buffer d'historique est déja plein, on remplace l'entrée la plus ancienne*/
	{
		for(i=0; i<TAILLE_MAX_HISTORY-1;++i)
		{
			strcpy(history[i], history[i+1]);
		}
		strcpy(history[i], commande);
	}
	else 							/*Sinon on prend la premiere entree libre qui est à l'indice i*/
	{
			strcpy(history[i], commande);
			++compteurCommandeHistorique;
	}



}

int historyCmd(char* commande, char** cmdDecoupee, char** history)
{
	int j=0;

	if (cmdDecoupee[1] == NULL)
	{
		fprintf(stdout, "Historique des dernieres commandes(de la plus ancienne à la plus récente):\n");
		while(history[j][0] != '\0' && j<compteurCommandeHistorique-1)			/*De cette manière, on a pas la derniere commande "history" affichee si l'history est plein*/
		{																/*En vrai si y'a pas le -1, ça fait une SEG_FAULT qui sort de nulle part */

			fprintf(stdout, "%d : %s\n", j, history[j]);
			j+=1;
		}
	}
	else if(cmdDecoupee[1][0] == '!' && cmdDecoupee[1][0] != '\0')
	{
		cmdDecoupee[1][0] = '0';
		if(cmdDecoupee[1][1] == '-')
			cmdDecoupee[1][1] = '0';
		if(atoi(cmdDecoupee[1])>0 && atoi(cmdDecoupee[1])<compteurCommandeHistorique && compteurCommandeHistorique - atoi(cmdDecoupee[1]) >=0)	/*On vérifie que le nombre est bien dans le range défini*/
		{
			strcpy(commande, history[compteurCommandeHistorique - atoi(cmdDecoupee[1])]);
			return 1;													/*Si on doit executer une commande on renvoie 1 pour que le booleen de la fonction appelante soit mis à 1*/
		}
		else if(cmdDecoupee[1][1] == '!' && compteurCommandeHistorique >1)	/*Si on tape history !!, éxécute la dernière commande tapée avant history*/
		{
			strcpy(commande, history[compteurCommandeHistorique-2]);
			return 1;
		}
		else
			fprintf(stdout, "%c, %c, Nombre en dehors de la taille de l'historique (%d maximum), ou alors NaN\n", cmdDecoupee[1][0], cmdDecoupee[1][1], TAILLE_MAX_HISTORY);
	}
	else if(atoi(cmdDecoupee[1])>0 && atoi(cmdDecoupee[1])<compteurCommandeHistorique)
	{
		for(j=compteurCommandeHistorique - atoi(cmdDecoupee[1])-1; history[j+1][0] != '\0' && j<compteurCommandeHistorique-1; ++j)
			fprintf(stdout, "%d : %s\n", j, history[j]);
	}
	else
		fprintf(stdout, "Argument incorrect! La taille maximum de l'historique est %d\n", TAILLE_MAX_HISTORY);

	if(j==0)
		fprintf(stdout, "Aucune commande  a afficher\n");

	fprintf(stdout, "\n");
	return 0;
}

void cd(char* finchemin, DIR* repertoire){

    char chemincourant[TAILLE_MAX_CHEMIN];
    if(getcwd(chemincourant, sizeof(chemincourant)) != NULL)
    {
        strcat(chemincourant, "/");
        chdir(strcat(chemincourant, finchemin));

	return;
    }
    else
    {
        fprintf(stderr, "fatality\n");
        return;
    }
}

/* ************* TOUCH ************ */
void touch(char** cmdDecoupee)											/*On a vérifié avant que le deuxième paramètre au moins n'étais pas nul*/
{
	int file, nbArg=0, nbOpt=0, i=0, mod=0;
	int continuer = 1;
	time_t timeStamp = time(NULL);

	while(continuer==1)
	{
		if(cmdDecoupee[nbArg][0] == '-')
			++nbOpt;
		++nbArg;
		if(cmdDecoupee[nbArg] == NULL)
			continuer=0;
	}
	--nbArg;
	fprintf(stderr, "Sorti de arg, %s\n", cmdDecoupee[nbArg]);
	file = open(cmdDecoupee[nbArg], O_WRONLY | O_CREAT, 0777);				/*On créé le fichier s'il n'existe pas*/
	if(file == -1)
	{
		fprintf(stderr, "L'ouverture du fichier destinataire a echoue\n");
		return;
	}

	close(file);
	struct utimbuf ubuf;
	struct stat *buf;
	buf = malloc(sizeof(struct stat));

	stat(cmdDecoupee[nbArg], buf);
	ubuf.actime = buf->st_atime;
	ubuf.modtime = buf->st_mtime;

	for(i=1;i<nbArg;++i)
	{
		if(strcmp(cmdDecoupee[i], "-a") == 0)							/*Option -a*/
		{
			ubuf.actime = timeStamp;								/*Date du dernier accès*/
			mod=1;
		}
		else if(strcmp(cmdDecoupee[i], "-m") == 0)							/*Option -m*/
		{
			ubuf.modtime = timeStamp;								/*Date de la dernière modification*/
			mod=1;
		}
		else
		{
			fprintf(stderr, "Option inconnue, la commande est incorrecte!\n");
		}
	}
	printf("MOD %d\n", mod);
	if(mod==0)
	{
		printf("ENTREEMOD %d\n", mod);
		ubuf.actime = timeStamp;
		ubuf.modtime = timeStamp;
	}

	if(utime(cmdDecoupee[nbArg], &ubuf) != 0)
	{
		fprintf(stderr, "Erreur lors de la fonction utime()\n");
		return;
	}
	else
	{
		stat(cmdDecoupee[nbArg], buf);
		printf("  utime.file modification time is %ld\n", buf->st_atime);

		printf("  utime.file modification time is %ld\n", buf->st_mtime);
	}


	free(buf);
}

/* ********** CAT ********** */
void cat(char** cmdDecoupee)
{
	int i;
	int file, nOption=0, lineCounter=2, demarre=0;
	char* buffer = (char*)malloc(sizeof(char));
	char bufferTemp = 'A';

	if(cmdDecoupee[1] == NULL || (cmdDecoupee[1][0] == '-' && cmdDecoupee[2] == NULL))
	{
		fprintf(stderr, "Erreur: chemin non-précisé\n");
		return;
	}

	for(i=1; cmdDecoupee[i] != NULL; ++i)
	{
		if(strcmp(cmdDecoupee[i], "-n") ==0)
			nOption=1;

		if(cmdDecoupee[i][0] != '-')
		{
			file = open(cmdDecoupee[i], O_RDONLY);
			if(file == -1)
			{
				fprintf(stderr, "L'ouverture du fichier source a echoue\n");
				free(buffer);
				return;
			}

			if(nOption == 1 && demarre == 0)
			{
				fprintf(stdout, "1: ");
				demarre = 1;
			}
			while(read(file, buffer, 1) >0)
			{
				if(nOption ==1 && bufferTemp == '\n')
				{
					fprintf(stdout, "%d: ", lineCounter);
					++lineCounter;
				}
				fprintf( stdout,"%c", *buffer);
				bufferTemp = *buffer;
			}

			close(file);
		}
	}

	free(buffer);
}

char* findPath(char* commande) //Retourne le chemin vers la commande demandée en entrée.
{

	int i = 0;
	DIR* rep = NULL;
	char* token;
	char* copie = malloc(sizeof(TAILLE_MAX_CHEMIN));
	struct dirent* fichier = NULL;
	while(environ[i] != NULL && !(environ[i][0] == 'P' && environ[i][1] == 'A' && environ[i][2] == 'T' && environ[i][3] == 'H' && environ[i][4] == '='))
	{
        i++; //On trouve la ligne du PATH dans les variables d'environnement
	}

	//On segmente la ligne en virant le début et en découpant selon les ":"
	char* ligne= (char*)malloc(sizeof(char)*TAILLE_MAX_PATH);
	strcpy(ligne, environ[i]);
	token = strtok(ligne, "="); //Attention, strtok est peu lisible et peu intuitive.
	token = strtok(NULL, "=");
	token = strtok(token, ":");


	while(token != NULL) //Pour tous les chemins du PATH
	{
		rep = opendir(token);

		if(rep == NULL)
		{
            /*printf("Erreur d'ouverture d'un dossier : %s\n", token);*/
        }
		else
		{
            //Les deux readdir inutiles
            fichier = readdir(rep);
            fichier = readdir(rep);
            while((fichier = readdir(rep)) != NULL) //Pour tous les fichiers
            {
                if(strcmp(commande, fichier->d_name) == 0) //Quand on a trouvé la commande
                {
                    closedir(rep);
                    free(ligne);
                    strcpy(copie, token);
                    strcat(copie, "/");
                    strcat(copie, commande);
                    return copie;
                }
            }

        }
        closedir(rep);
        token = strtok(NULL, ":"); //Passe au prochain chemin du PATH.
	}
	free(ligne);
	free(fichier);
	free(token);
	fprintf(stderr, "Erreur, commande inconnue...\n");
	return " "; //En cas d'erreur, une commande vide.
}

int main(int argc, char *argv[])
{
	int i;
	char* commande = (char*)malloc(sizeof(char)*TAILLE_MAX_COMMANDE);

	char** history = (char**)malloc(sizeof(char*)*TAILLE_MAX_HISTORY);
	for(i=0; i<TAILLE_MAX_HISTORY; ++i)
	{
		history[i] = (char*)malloc(sizeof(char)*TAILLE_MAX_COMMANDE);
		history[i][0] = '\0';
	}

	compteurCommandeHistorique = 0;

	fprintf(stdout, "Bienvenue dans My_Shell!\nPour Quitter, taper Ctrl+D\n");
	while(1)
	{
		
		invite_commande();
		lire_commande(commande);
		addHistory(commande, history);
		parseCommande(commande, history);
	}


	free(commande);
	for(i=0; i<TAILLE_MAX_HISTORY; ++i)
		free(history[i]);
	free(history);


	printf("Hello World!\n");
	return 0;
}
