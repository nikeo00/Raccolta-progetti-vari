#include <sys/types.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <unistd.h>
#include <stdio.h>
#include <fcntl.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <dirent.h>
#include <time.h>   
#include <signal.h>
#include <math.h>

#define MAXLINE 1024 
#define maxClient 3  
#define BUF_SIZE (600)		

struct timeval tv;

int cont = 0;
int porta = 0;

char bufferletti[BUF_SIZE];
int numletti = 0;
int base = 0;
int tattesa = 1;
int numdatagramma = 0;
int limitefinestra = 0; 
int ritrasmetti = 0;


struct __attribute__((packed)) frame_t {
	char lengthFrame[10];
	char tipo[10]; 
	char ID[20];	
	char data[BUF_SIZE];
	char acked[10];
};


struct __attribute__((packed)) message
{
    uint8_t tipomess;
    char porta[30];
    char msg[MAXLINE];
};

int list(FILE *f) 
{ 
	struct dirent **dirent; int n = 0;
	if ((n = scandir(".", &dirent, NULL, alphasort)) < 0) { 
		perror("Scanerror"); 
		return -1; 
	}   
	while (n--) {
		fprintf(f, "%s\n", dirent[n]->d_name);	
		free(dirent[n]); 
	}
	free(dirent); 
	return 0; 
}                                             

void handler_timeout(int sig){
	alarm(0);
	ritrasmetti = 1;
}

static void print_error(const char *msg, ...)
{
	perror(msg);
	exit(EXIT_FAILURE);
}

float stimaRTT = 0 ;   
float deviazioneRTT = 0 ;
float campioneN = 0 ;
float stimaRTTpassata = 0 ;
int timeout = 0;

int calcola_timer (float *stimaRTT,float *deviazioneRTT,float *campioneN, float *stimaRTTpassata){
	float alpha = 0.125;
	float beta = 0.25;
	*stimaRTT = (1-alpha)*(*stimaRTTpassata)+alpha*(*campioneN);
	*deviazioneRTT = (1-beta)*(*deviazioneRTT)+beta*(*campioneN - *stimaRTT);
	*stimaRTTpassata = *stimaRTT;
	int timeout = 0;
	timeout = *stimaRTT +4*(*deviazioneRTT);
	timeout = round(timeout);
	(timeout == 0) ? 1 : timeout ;
	printf("timeout calcolato = %d\n",timeout);
	return timeout;
}

int main (int argc, char **argv)
{	
	struct sigaction sa;
	sigfillset(&sa.sa_mask);
	sa.sa_handler = handler_timeout;
	sa.sa_flags = 0;
	sigaction(SIGALRM,&sa,NULL);
	clock_t inizio, fine;
	if(argc!=6)
    {
        printf("Modo di uso --> ./[%s] [Port Number] [larghezza finestra di invio] [probabilità di perdita pacchetti p in %% ] [durata timer] [timer fisso o adattivo]\n", argv[0]);		//Should have a port number > 5000
		exit(EXIT_FAILURE);
    }
	int adattivo = 0;
	if (!strcmp(argv[5],"adattivo")){
		adattivo = 1;
	}
	timeout = atoi(argv[4]);
    int listenfdTCP ,listenfdUDP , connfdTCP ,serv_port = atoi(argv[1]);
    struct message msg;
	int probabilitàPerdita = atoi(argv[3]);
	srand(time(NULL)); 
	int numeroRandom = 0;
	int datoRicevuto = 0;
	int datoMandato = 0;
	struct sockaddr_in sv_addr, cl_addr;
	struct stat st;
	char msg_recv[BUF_SIZE];
	char flname_recv[20];         
	char comando_recv[10];
	ssize_t numRead;
	ssize_t length;
	off_t f_size; 	
	uint32_t ack_num = 0;   
	int ack_send = 0;
	int sfd;
	struct timeval t_out = {0, 0}; 
	struct frame_t frame, framePerso;
	uint32_t totale_datagrammi = 0;
	FILE *fptr;
	int sizeTXwindow = atoi(argv[2]);
	struct frame_t *listaDatagrammiTX[sizeTXwindow];
	struct frame_t *listaDatagrammiRX[sizeTXwindow];
	for(int i = 0;i<sizeTXwindow;i++){
		
		listaDatagrammiTX[i] = (struct frame_t *)malloc(sizeof(struct frame_t)); //ricordati di fare free dopo alla fine
		listaDatagrammiRX[i] = (struct frame_t *)malloc(sizeof(struct frame_t)); //ricordati di fare free dopo alla fine
		// setto a 0 la struttura
		memset(listaDatagrammiTX[i],0,sizeof(struct frame_t));	
		memset(listaDatagrammiRX[i],0,sizeof(struct frame_t));	
		
	}
    pid_t childpid;
    socklen_t clilen;
    char buf[MAXLINE];
    struct sockaddr_in cliaddr, servaddr;
    if ((listenfdTCP = socket (AF_INET, SOCK_STREAM, 0)) <0)
    {
        perror("Problem in creating tcp socket\n");
        exit(2);
    }
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
    servaddr.sin_port = htons(serv_port);
    if (bind (listenfdTCP, (struct sockaddr *) &servaddr, sizeof(servaddr))<0)
    {
        perror("Problem in binding to tcp socket\n");
        exit(3);
    }
    if (listen (listenfdTCP, maxClient)<0)
    {
        perror("Problem in listening on tcp socket\n");
        exit(4);
    }
    int child_number = 0;
    printf("Server attivo\n");
    printf("Client No \tChild PID \tClient IP \tTCP/UDP \tClient Porta\n");
    printf("-------------------------------------------------------------------------------------------------------------------\n");
    for ( ; ; )
    {	
        clilen = sizeof(cliaddr);
        connfdTCP = accept (listenfdTCP, (struct sockaddr *) &cliaddr, &clilen);
        printf("richiesta di connessione ricevuta\n");
		child_number++;
        if ( (childpid = fork ()) == 0 )  
        {
            printf("sono il processo figlio n°%d con id %d\n", child_number, getppid());
			close (listenfdTCP);
            if( recv(connfdTCP, &msg, MAXLINE,0) == 0)
            {
                perror("The client terminated prematurely\n");
                exit(5);
            } 
        printf("%d\t\t%d\t\t%s\tTCP\t\t%d\t\tRicevuto messaggio di tipo: %d\n", child_number, getpid(), inet_ntoa(cliaddr.sin_addr), ntohs(cliaddr.sin_port), msg.tipomess);  
	    servaddr.sin_port = htons(0);
		//UDP start
	    if ((listenfdUDP = socket (AF_INET, SOCK_DGRAM, 0)) <0)
	    {
		    perror("errore nel creare socket udp\n");
		    exit(2);
	    }
	    if (bind (listenfdUDP, (struct sockaddr *) &servaddr, sizeof(servaddr))<0)
	    {
		    perror("errore nel binding della socket udp\n");
		    exit(3);
	    }
	    struct sockaddr_in localAddress;
	    socklen_t addressLength = sizeof localAddress;
	    getsockname(listenfdUDP, (struct sockaddr*)&localAddress,&addressLength); 
		printf("info circa che porta udp sto inviando\n");
	    printf("figlio numero = %d id = %d client addr = %s client porta = %d porta udp = %d\n", child_number, getpid(), inet_ntoa(cliaddr.sin_addr), cliaddr.sin_port,localAddress.sin_port);
	    msg.tipomess = 2;	
		printf("porta numero = %d\n",localAddress.sin_port);
		porta = localAddress.sin_port ;
        send(connfdTCP, &porta, sizeof(porta), 0);	    
	    printf("%d\t\t%d\t\t%s\tTCP\t\t%d\t\tmandato messaggio di tipo messaggio:\t  %d\n", child_number, getpid(), inet_ntoa(cliaddr.sin_addr), cliaddr.sin_port, msg.tipomess);	    
	    printf("%d\t\t%d\t\t%s\t---\t\t%d\t\tchiusa connessione tcp:\t  %d\n", child_number, getpid(), inet_ntoa(cliaddr.sin_addr),cliaddr.sin_port, close(connfdTCP));
							
		for(;;) {
			printf("iniziata connessione udp,aspetto comando\n");
			printf("Server:aspetto client che si connette\n");
			struct timeval no_timeout = {0, 0}; 
			setsockopt(listenfdUDP, SOL_SOCKET, SO_RCVTIMEO, &no_timeout, sizeof(no_timeout));      
			alarm(0); 
			ritrasmetti = 0;
			memset(msg_recv, 0, sizeof(msg_recv));
			memset(comando_recv, 0, sizeof(comando_recv));
			memset(flname_recv, 0, sizeof(flname_recv));
			length = sizeof(cliaddr);
			numRead = recvfrom(listenfdUDP, msg_recv, BUF_SIZE, 0, (struct sockaddr *) &cliaddr, (socklen_t *) &length);	
			if (numRead <= 0) {
				if (errno == EAGAIN || errno == EWOULDBLOCK || errno == EINTR) {
					continue; 
				}
				perror("Server: errore critico in recvfrom");
				break; 
			}
			printf("Server: Comando ricevuto ---> %s\n", msg_recv);
			sscanf(msg_recv, "%s %s", comando_recv, flname_recv);

/*----------------------------------------------------------------------"get case"-------------------------------------------------------------------------*/
			if ((strcmp(comando_recv, "get") == 0) && (flname_recv[0] != '\0')) {
				printf("sono nel caso get\n");
				if (access(flname_recv, F_OK) == 0) {
					int trasferimento_concluso = 0; 
					base = 0; 
					numdatagramma = 0; 
					int i = 0; 
					stat(flname_recv, &st); 
					f_size = st.st_size;
					fptr = fopen(flname_recv, "rb");
					if ((f_size % BUF_SIZE) != 0)
						totale_datagrammi = (f_size / BUF_SIZE) + 1;
					else
						totale_datagrammi = (f_size / BUF_SIZE);
					printf("Total number of packets ---> %d\n", totale_datagrammi);
					totale_datagrammi = htonl(totale_datagrammi);
					sendto(listenfdUDP, &(totale_datagrammi), sizeof(totale_datagrammi), 0, (struct sockaddr *) &cliaddr, sizeof(cliaddr));	
					recvfrom(listenfdUDP, &(ack_num), sizeof(ack_num), 0, (struct sockaddr *) &cliaddr, (socklen_t *) &length);
					totale_datagrammi = ntohl(totale_datagrammi);
					ack_num = ntohl(ack_num);
					while (ack_num != totale_datagrammi) {
						sendto(listenfdUDP, &(totale_datagrammi), sizeof(totale_datagrammi), 0, (struct sockaddr *) &cliaddr, sizeof(cliaddr)); 
						recvfrom(listenfdUDP, &(ack_num), sizeof(ack_num), 0, (struct sockaddr *) &cliaddr, (socklen_t *) &length);
					}
					printf("Handshake completato. Inizio invio file...\n");
					while(!trasferimento_concluso) {	
						limitefinestra = ((base + sizeTXwindow) > totale_datagrammi) ? totale_datagrammi : (base + sizeTXwindow);
						while(numdatagramma < limitefinestra) {
							cont = numdatagramma % sizeTXwindow;
							numletti = fread(bufferletti, 1, BUF_SIZE, fptr);
							sprintf(listaDatagrammiTX[cont]->lengthFrame, "%d", numletti);
							strcpy(listaDatagrammiTX[cont]->tipo, "get");
							sprintf(listaDatagrammiTX[cont]->ID, "%d", numdatagramma);
							memcpy(listaDatagrammiTX[cont]->data, bufferletti, numletti);
							sprintf(listaDatagrammiTX[cont]->acked, "%d", 0);
							numeroRandom = rand() % 101;
							if (numeroRandom < probabilitàPerdita) {
								printf("ho perso pacchetto ID: %d PERSO\n", numdatagramma);
							} else {
								printf("ho inviato pacchetto ID: %d\n", numdatagramma);
								sendto(listenfdUDP, listaDatagrammiTX[cont], sizeof(struct frame_t), 0, (struct sockaddr *) &cliaddr, sizeof(cliaddr));
							}
							if(adattivo) {
								inizio = clock(); 
							}
							printf("timeout in secondi = %d\n",timeout);
							alarm(timeout); 
							numdatagramma++;
						}
						tv.tv_sec = timeout;
						setsockopt(listenfdUDP, SOL_SOCKET, SO_RCVTIMEO, (const char*)&tv, sizeof tv);
						if (recvfrom(listenfdUDP, &frame, sizeof(frame), 0, (struct sockaddr *) &cliaddr, (socklen_t *) &length) > 0) {
							
							if (strcmp(frame.tipo, "fineget") == 0) {
								printf("ricevuto fineget. Invio conferma e chiudo.\n");
								strcpy(frame.tipo, "fineget");
								sprintf(frame.acked, "%d", 1);
								sendto(listenfdUDP, &frame, sizeof(frame), 0, (struct sockaddr *) &cliaddr, sizeof(cliaddr));
								trasferimento_concluso = 1;
								alarm(0);
							}
							else if (!strcmp(frame.tipo, "ack")) {
								int id_ack = atoi(frame.ID);
								if (id_ack >= base && id_ack < limitefinestra) {
									int idx = id_ack % sizeTXwindow;
									if (atoi(listaDatagrammiTX[idx]->acked) == 0) {
										printf("ricevuto ack con ID =  %d\n", id_ack);
										sprintf(listaDatagrammiTX[idx]->acked, "%d", 1);
										i++;
										
										if(adattivo && id_ack == base) {
											fine = clock();
											campioneN = (float)(fine - inizio) / CLOCKS_PER_SEC;
										}

										while (atoi(listaDatagrammiTX[base % sizeTXwindow]->acked) == 1 && base < numdatagramma) {
											base++;
											alarm(0);
											if (base < numdatagramma) alarm(timeout);
											if (base == totale_datagrammi) break;
										}
									}
								}
							}
						}

						if (ritrasmetti && !trasferimento_concluso) {
							ritrasmetti = 0;
							if (base < totale_datagrammi) {
								printf("scaduto timeout, ritrasmissione finestra [%d - %d]\n", base, limitefinestra-1);
								for (int k = 0; k < sizeTXwindow; k++) {
									int id_k = atoi(listaDatagrammiTX[k]->ID);
									if (id_k >= base && id_k < limitefinestra && atoi(listaDatagrammiTX[k]->acked) == 0) {
										if ((rand() % 101) >= probabilitàPerdita) {
											sendto(listenfdUDP, listaDatagrammiTX[k], sizeof(struct frame_t), 0, (struct sockaddr *) &cliaddr, length);
											printf("ho rimandato un pacchetto con id = %s",frame_t.ID);
										}
									}
								}
								alarm(timeout);
							} else {
								printf("finito,aspetto un fineget\n");
								tv.tv_sec = 1; 
								setsockopt(listenfdUDP, SOL_SOCKET, SO_RCVTIMEO, (const char*)&tv, sizeof tv);
							}
						}
					}
					fclose(fptr);
					alarm(0);           
					ritrasmetti = 0;    
				} else {
					printf("Invalid Filename\n");
				}
			}


						/*----------------------------------------------------------------------"put case"-------------------------------------------------------------------------*/
			else if (strcmp(comando_recv, "put") == 0) {
				printf("Ricezione file (put) da client: %s\n", flname_recv);
				socklen_t clilen_udp = sizeof(cliaddr); 
				uint32_t totale_net;
				recvfrom(listenfdUDP, &totale_net, sizeof(totale_net), 0, (struct sockaddr *)&cliaddr, &clilen_udp);
				totale_datagrammi = ntohl(totale_net);
				sendto(listenfdUDP, &totale_net, sizeof(totale_net), 0, (struct sockaddr *)&cliaddr, clilen_udp);
				fptr = fopen(flname_recv, "wb");
				for(int k = 0; k < sizeTXwindow; k++) {
					sprintf(listaDatagrammiRX[k]->acked, "0"); 
				}			
				int i_put = 0;
				int limite_inf_put = 0;
				while (i_put < totale_datagrammi) {
					int limite_sup_put = limite_inf_put + sizeTXwindow;
					tv.tv_sec = timeout;
					setsockopt(listenfdUDP, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));
					if (recvfrom(listenfdUDP, &frame, sizeof(frame), 0, (struct sockaddr *)&cliaddr, &clilen_udp) > 0) {
						int id_rx = atoi(frame.ID);
						if (strcmp(frame.tipo, "put") == 0) {
							if (id_rx >= limite_inf_put && id_rx < limite_sup_put) {
								int idx = id_rx % sizeTXwindow;
								if (atoi(listaDatagrammiRX[idx]->acked) == 0) {
									printf("ricevuto pacchetto con  ID: %d\n", id_rx);
									strcpy(listaDatagrammiRX[idx]->lengthFrame, frame.lengthFrame);
									strcpy(listaDatagrammiRX[idx]->ID, frame.ID);
									memcpy(listaDatagrammiRX[idx]->data, frame.data, BUF_SIZE);
									strcpy(listaDatagrammiRX[idx]->acked, "1");

									if (id_rx == limite_inf_put) {
										while (atoi(listaDatagrammiRX[limite_inf_put % sizeTXwindow]->acked) == 1) {
											int c_idx = limite_inf_put % sizeTXwindow;
											fwrite(listaDatagrammiRX[c_idx]->data, 1, atoi(listaDatagrammiRX[c_idx]->lengthFrame), fptr);
											strcpy(listaDatagrammiRX[c_idx]->acked, "0");
											limite_inf_put++;
											i_put++;
											if (i_put == totale_datagrammi) break;
										}
									}
								}
								if ((rand() % 101) >= probabilitàPerdita) {
									strcpy(frame.tipo, "ack");
									sendto(listenfdUDP, &frame, sizeof(frame), 0, (struct sockaddr *)&cliaddr, clilen_udp);
								}
							} else if (id_rx < limite_inf_put) {
								strcpy(frame.tipo, "ack");
								sendto(listenfdUDP, &frame, sizeof(frame), 0, (struct sockaddr *)&cliaddr, clilen_udp);
							}
						}
					}
				}

				int concluso = 0;
				while (!concluso) {
					tv.tv_sec = 2; 
					setsockopt(listenfdUDP, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));
					if (recvfrom(listenfdUDP, &frame, sizeof(frame), 0, (struct sockaddr *)&cliaddr, &clilen_udp) > 0) {
						if (strcmp(frame.tipo, "fineput") == 0) {
							printf("Ricevuto fineput. Invio ACK finale.\n");
							strcpy(frame.tipo, "fineput");
							sprintf(frame.acked, "1");
							sendto(listenfdUDP, &frame, sizeof(frame), 0, (struct sockaddr *)&cliaddr, clilen_udp);
							concluso = 1;
						}
					}else{
						concluso = 1;
					}
				}
				fclose(fptr);
			}
	
	/*----------------------------------------------------------------------"list case"----------------------------------------------------------------------------*/

			else if (strcmp(comando_recv, "list") == 0) {
				printf("sono nel caso list\n");
				
				// creo la lista di file
				char file_entry[200];
				
				//setto tutta la lista  a 0
				memset(file_entry, 0, sizeof(file_entry));
				
				//creo un file di appoggio con permesso in lettura
				fptr = fopen("a.log", "wb");	
				
				// su fptr ossia a.log ora ci sta la lista di file ,controllo anche che non ho errori
				if (list(fptr) == -1)		
					print_error("list");
				
				//chiudo il file perchè ci ho scritto
				fclose(fptr);
				
				//apro il file in lettura dopo che ci ho scritto
				fptr = fopen("a.log", "rb");	
				printf ("aperto file a.log in lista\n");
				
				
				//ottengo la dimensione del file
				int filesize = fread(file_entry, 1, 200, fptr);
				printf("ho preso la dim del file con fread ->%d\n",filesize);
				remove("a.log");  //delete the temp file
				printf("rimosso a.log\n");
				//calcolo il numero di datagrammi da mandare
				if ((f_size % BUF_SIZE) != 0)
						totale_datagrammi = (filesize / BUF_SIZE) + 1;				
					else
						totale_datagrammi = (filesize / BUF_SIZE);

				printf("Total number of packets ---> %d\n", totale_datagrammi);
				printf("Filesize = %d	%ld\n", filesize, strlen(file_entry));
				
				//creo il pacchetto, che è una semplice stringa qui ,  da inviare e lo invio
				if (sendto(listenfdUDP, file_entry, filesize, 0, (struct sockaddr *) &cliaddr, sizeof(cliaddr)) == -1)  
					print_error("Server: send");
				fclose(fptr);
			}

	/*--------------------------------------------------------------------"exit case"----------------------------------------------------------------------------*/

			else if (strcmp(comando_recv, "exit") == 0) {
				close(listenfdUDP);   //close the server on exit call
				exit(EXIT_SUCCESS);
			}

	/*--------------------------------------------------------------------"Invalid case"-------------------------------------------------------------------------*/

			else {
				printf("Server: Unkown command. Please try again\n");
			}
		}
		
		close(listenfdUDP);
		exit(EXIT_SUCCESS);
			
	    
        }
        close(connfdTCP);
    }
    close(listenfdTCP);
	for(int i = 0;i<sizeTXwindow;i++){
		
		free(listaDatagrammiTX[i]); 
		free(listaDatagrammiRX[i]); 
	}
    return 0;
}