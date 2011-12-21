#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <netinet/in.h>
#include <string.h>
#include <errno.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <signal.h>
#include <fcntl.h>
#include <pthread.h>

#include "gui.h"
#include "pers_sock.h"
#include "fonctionsClient.h"


#define GRID_W 48
#define GRID_H 32
#define CLEAN()	if(close(sd) == -1){perror("Erreur de close");}

struct buff {
	int _fd_in;
	int _fd_out;
	int _pid_fils;
};

int arret = 0;
int pris_en_charge = 0;
int en_attente = 0;
int changement = 0;
int sommeil = 1;

/* Fonction du thread chargé de la lecture clavier */
void fn_thread (void *tube) {
	struct buff *fd = (struct buff *) tube;
	int c = 0;
	while (1) {
		c = getch();
		switch (c) {
			case 'c' :
				if (!pris_en_charge && !en_attente) {
					kill(fd->_pid_fils, SIGUSR1);
					en_attente = 1; changement = 1;
				}
				break;
			case 'q' :
				if ((!pris_en_charge && en_attente) || pris_en_charge) {
					write (fd->_fd_out, (void *)'q', sizeof(char));
					en_attente = 0; pris_en_charge = 0; changement = 1;
				}
				break;
			case KEY_LEFT:
				write (fd->_fd_out, (void *)'l', sizeof(char));
				break;
			case KEY_RIGHT:
				write (fd->_fd_out, (void *)'r', sizeof(char));
				break;
			case KEY_UP:
				write (fd->_fd_out, (void *)'u', sizeof(char));
				break;
			case KEY_DOWN:
				write (fd->_fd_out, (void *)'d', sizeof(char));
				break;
			default:
				break;
		}
		usleep(10000);
	}
}

void endloop () {
	arret = 1;
}

void endwait() {
	sommeil = 0;
	fprintf (stderr, "Signal SIGUSR1 reçu...\n");
}

void connected () {
	en_attente = 0;
	pris_en_charge = 1;
	changement = 1;
}

void disconnected () {
	en_attente = 0;
	pris_en_charge = 0;
	changement = 1;
}

int main(int argc, char **argv) {
	p_host_t hote = NULL;
	sockin_t em_server, rc_server, client;
	int grid_size = GRID_H * (GRID_W + 1) + 1;
	char recvit[grid_size];
	int nbLus, sd;
	int sommeil = 1;
	int pid = 0, tube[2];	
	int ppid = getpid();
	struct sigaction action;
	struct sigaction action_sigusr1, action_sigusr2;
	struct buff comm;

	if (pipe(tube)) {
		perror("pipe");
		return -1;
	}

	bzero(&em_server,sizeof(em_server));
	bzero(&rc_server,sizeof(rc_server));

	/* Lecture des options */
	read_options_client(argc, argv, &em_server, &rc_server);

	/* Fork du programme */
	pid = fork();

	switch(pid) {
		case -1:
			perror("fork");
			return -1;
		case 0:
			#include "code_fork.c"
			break;
		default :
			#include "code_pere.c"
			break;
	}
	return EXIT_SUCCESS;
}


