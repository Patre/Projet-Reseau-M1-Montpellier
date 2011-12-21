

#include "fonctionsServeurR.h"


char* grille;
int arret = 0;
int camMoving = 0;
int nbMouvements = 0;

/* Thread principal du serveur de reception */
int main(int argc, char* argv[])
{
	int nbClients = 0;
	struct sockaddr_in server;
	int addr_in_size = sizeof(struct sockaddr_in);
	int i = 1;
	int sd_client;
	pthread_t thread_id;
	int *socketClients_trans = NULL;
	int *socketClients = NULL;
	struct sockaddr_in* client;
	int sd;
	int prems = 0;
	time_t timeBthread, timeCur, timeControl = 20;
	
	client = (struct sockaddr_in *)malloc(addr_in_size);
	bzero(client,sizeof(client));
	
	/* Gestion des signaux */
	attachSignals();
	
	/* Recuperation du segment de memoire partagee de la grille */
	grille = gridRecupAddr(argv[1]);
	
	/* Init reseau */
	if((sd = gstArgs(argv, &server)) < 1)
	{
		free(client);
		if(sd == 0)
			exit(EXIT_SUCCESS);
		else
			exit(EXIT_FAILURE);
	}
	
	/* Ecoute des demandes de connexion des clients */
	if(listen(sd, 1) == -1)
	{
		perror("listen");
		free(client);
		close(sd);
		exit(EXIT_FAILURE);
	}

	printf ("Ready\n");
	
	/* boucle d'execution : acceptation des clients et creation du thread */
	while(!arret)
	{
		if(nbClients < 19 && (sd_client = accept(sd, (struct sockaddr *)client, (socklen_t*)(&addr_in_size))) != -1)
		{
			// accepter la connexion entrante
			if(socketClients == NULL) {
				printf ("Première connexion ok...\n");
				socketClients = malloc(sizeof(int));
				if (!prems) {
					setNonblocking(sd);
					prems ++;
				}
			}
			else
			{
				socketClients_trans = malloc((nbClients+1)*sizeof(int));
				for(i = 0 ; i < nbClients ; i++)
				{
					socketClients_trans[i] = socketClients[i];
				}
				printf("Nouvelle connexion ok...\n");
				free(socketClients);
				socketClients = socketClients_trans;
				socketClients_trans = NULL;
			}
			socketClients[nbClients] = sd_client;
			nbClients++;
			printf("Client en Liste d'attente...");
		}
		if(camMoving == 1)
		{
			timeCur = time(NULL);
			if(timeCur - timeBthread >= timeControl)
			{
				printf("Le client sur la socket %d perd la controle de la camera.\n", socketClients[0]);
				pthread_kill(thread_id, SIGINT);
				close(socketClients[0]);
				for(i = 0 ; i < nbClients-1 ; i++)
				{
					socketClients[i] = socketClients[i+1];
				}
				nbClients--;
				socketClients = realloc(socketClients, nbClients*sizeof(int));
				camMoving = 0;
			}
		}
		if(camMoving == 0 && nbClients > 0)
		{
			timeBthread = time(NULL);
			
			/* lancement de l'execution du thread qui s'occupera du client */
			if(pthread_create(&thread_id, NULL, thread_deplacement, &(socketClients[0])) != 0)
			{
				fprintf(stderr, "Thread creation failure.\n");
				arret = 1;
			}
		}
	}
	
	
	/* clean */
	for(i = 0 ; i < nbClients ; i++)
	{
		printf("Close socket %d\n", socketClients[i]);
		if(close(socketClients[i]) < 0)
		{
			perror("erreur close ");
		}
	}
	
	
	free(client);
	free(socketClients);
	close(sd);
	if(shmdt(grille) == -1)
	{
		perror("Shared memory segment's detachment impossible");
	}
	
	printf("Terminaison du serveur de reception.\n");
	
	return 0;
}


