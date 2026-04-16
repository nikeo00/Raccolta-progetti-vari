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
#define BUF_SIZE 600	

int port = 0;

struct timeval tv;

//struct for negotiating a ephimeral port
struct __attribute__((packed)) message
{
    uint8_t messtype;
    char port[30];
    char msg[MAXLINE];
};

//struct for the working protocoll
struct __attribute__((packed)) frame_t {
	char lengthFrame[10];
	char type[10]; 
	char ID[20];	
	char data[BUF_SIZE];
	char acked[10];
};

static void print_error(char *msg)
{
	perror(msg);
	exit(EXIT_FAILURE);
}

//used for the adaptive timer
float stimaRTT = 0 ;   
float deviazioneRTT = 0 ;
float campioneN = 0 ;
float stimaRTTpassata = 0 ;
int timeout = 0;

//compute the adaptive timer
int calcola_timer (float *stimaRTT,float *deviazioneRTT,float *campioneN, float *stimaRTTpassata){
	float alpha = 0.125;
	float beta = 0.25;
	*stimaRTT = (1-alpha)*(*stimaRTTpassata)+alpha*(*campioneN);
	*deviazioneRTT = (1-beta)*(*deviazioneRTT)+beta*(*campioneN - *stimaRTT);
	int timeout;
	timeout = *stimaRTT +4*(*deviazioneRTT);
	timeout = round(timeout);
	(timeout == 0) ? 1 : timeout ;
	return timeout;
}
 

int main(int argc, char **argv)
{
	if (argc !=7)
    {
		printf("Client: uso --> ./[%s] [IP Address] [Port Number] [larghezza finestra di invio] [probabilità di perdita pacchetti p in %% ] [durata timer] [timer fisso o adattivo]\n", argv[0]);
        exit(EXIT_FAILURE);
    }
	
	struct sockaddr_in send_addr, from_addr;
	struct stat st;
	struct frame_t frame, framePerso;
	
	int probabilitàPerdita = atoi(argv[4]);
	srand(time(NULL));  
	int numeroRandom = 0;
	int datoRicevuto = 0;
	int datoMandato = 0;
	int timeout = 0;
	timeout = atoi(argv[5]);
	
	char comando_send[50];
	char flname[20];
	char comando[10];
	char ack_send[4] = "ACK";
	
	struct timeval t_out = {0, 0}; 
	
	ssize_t numRead = 0;
	ssize_t length = 0;
	off_t f_size = 0;
	long int ack_num = 0;
	int Client_fd, ack_recv = 0;
	
	FILE *fptr;
	
	int sizeTXwindow = atoi(argv[3]);
	
	struct frame_t *listaDatagrammiTX[sizeTXwindow];
	

	struct frame_t *listaDatagrammiRX[sizeTXwindow];
	

	for(int i = 0;i<sizeTXwindow;i++){
		
		listaDatagrammiTX[i] = (struct frame_t *)malloc(sizeof(struct frame_t));
		listaDatagrammiRX[i] = (struct frame_t *)malloc(sizeof(struct frame_t)); 
		memset(listaDatagrammiTX[i],0,sizeof(struct frame_t));	
		memset(listaDatagrammiRX[i],0,sizeof(struct frame_t));	
		
	}
	
	
	
	memset(ack_send, 0, sizeof(ack_send));
	memset(&send_addr, 0, sizeof(send_addr));
	memset(&from_addr, 0, sizeof(from_addr));
	
	memset(comando_send, 0, sizeof(comando_send));
	memset(comando, 0, sizeof(comando));
	memset(flname, 0, sizeof(flname));
	
	
    int sockfdTCP, sockfdUDP,serv_port = atoi(argv[2]);
	char udp_port[30] ;
    struct sockaddr_in servaddr;
    struct message msg;
	socklen_t servlen;
	
	
    if ((sockfdTCP = socket (AF_INET, SOCK_STREAM, 0)) <0)
    {
        perror("errore nella creazione di una socket TCP \n");
        exit(2);
    }
    memset(&servaddr, 0, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr= inet_addr(argv[1]);
    servaddr.sin_port =  htons(serv_port);
	
    if (connect(sockfdTCP, (struct sockaddr *) &servaddr, sizeof(servaddr))<0)
    {
        perror("Problema nel connettersi al server fase TCP\n");
        exit(3);
    }
    
    
    printf("Server IP\tTCP/UDP\t\tServer port\n");
    printf("---------------------------------------------------------------------------------------------\n");
    

    msg.typemess = 1;

    send(sockfdTCP, &msg, sizeof(msg), 0);
    printf("%s\tTCP\t\t%d\t\tmandato messaggio di type :\t  %d\n", inet_ntoa(servaddr.sin_addr), servaddr.sin_port, msg.typemess);

    printf("port = %d\n",port);

    if (recv(sockfdTCP, &port, sizeof(port),0) == 0)
    {
        perror("server chiuso\n");
        exit(4);
    }

	printf("port ricevuta = %d\n",port);
    
 
    printf("%s\tTCP\t\t%d\t\tricevuto messaggio di type: %d\n", inet_ntoa(servaddr.sin_addr), servaddr.sin_port, msg.typemess);
	
	printf("ho ricevuto la seguente port udp = %d\n",port);
    printf("%s\tTCP\t\t%d\t\tport UDP ricevuta:\t  %d\n", inet_ntoa(servaddr.sin_addr), servaddr.sin_port, port);

    printf("%s\t---\t\t%d\t\chiusa la connessione TCP:\t  %d (0 if success)\n", inet_ntoa(servaddr.sin_addr), servaddr.sin_port, close(sockfdTCP));


    //  UDP start from here
	
	// creo socket UDP
    if ((sockfdUDP = socket (AF_INET, SOCK_DGRAM, 0)) <0)
    {
		perror("Problema nella creazione di socket UDP\n");
		exit(2);
    }
	
	//setto la socket UDP 
    servlen = sizeof(servaddr);
    servaddr.sin_port = port;
    for(;;){
		//da qui posso parlare in UDP,
		
		printf("client connesso al server con successo\n");
		printf("\n Menu \n digitare un qualunque dei seguenti comandi \n 1.) get [file_name] \n 2.) put [file_name]\n 3.) list \n 4.) exit \n");		
		scanf(" %[^\n]%*c", comando_send);


		struct timeval no_timeout = {0, 0};
        setsockopt(sockfdUDP, SOL_SOCKET, SO_RCVTIMEO, &no_timeout, sizeof(no_timeout));
        
			
		sscanf(comando_send, "%s %s", comando, flname);		//parse the user input into command and filename
		

		
		printf("comando = %s\n",comando);
		
		
		/*----------------------------------------------------------------------"get case"-------------------------------------------------------------------------*/

			if ((strcmp(comando, "get") == 0) && (flname[0] != '\0' )) {
				printf("sono nel caso get \n");

				uint32_t totale_datagrammi = 0;
				long int bytes_rec = 0;
				long int i = 0;
				
				datoRicevuto = 0;
				

				while( !datoRicevuto ){
					
					if (sendto(sockfdUDP, comando_send, sizeof(comando_send), 0, (struct sockaddr *) &servaddr, sizeof(servaddr)) == -1) {
						print_error("Client: send");
					}
					printf("sto aspettando di ricevere il numero di datagrammi\n");
				
					recvfrom(sockfdUDP, &(totale_datagrammi), sizeof(totale_datagrammi), 0, (struct sockaddr *) &servaddr, (socklen_t *) &servlen); //Get the total number of frame to recieve

					totale_datagrammi = ntohl(totale_datagrammi);
					printf("ho ricevuto il numero di datagrammi --> %d \n",totale_datagrammi);
					datoRicevuto = 1;
				}

				
				if (totale_datagrammi > 0) {

					
					totale_datagrammi = htonl(totale_datagrammi);
					sendto(sockfdUDP, &(totale_datagrammi), sizeof(totale_datagrammi), 0, (struct sockaddr *) &servaddr, sizeof(servaddr));
					printf("inviato ack con numero di ack----> %d\n", ntohl(totale_datagrammi));
					
					fptr = fopen(flname, "wb");	//open the file in write mode
					printf("ho aperto un file in scrittura\n");
					int i  = 0;
					
					int datagrammi_ricevuti = 0;
					int cont = 0;
					int ackinviati = 0;
					int limitesupfinestra = i+sizeTXwindow;
					int limiteinfwindow = i;
					int finestrascritta = 1;
					int pacchettiscritti = 0;
					
					printf("sto per prendere dati dalla rete\n");
					// repeat all till the file is fully transfered
					totale_datagrammi = ntohl(totale_datagrammi);
					
					
					limiteinfwindow = 0;
					i = 0; 	
					
					for(int k = 0; k < sizeTXwindow; k++) {
						strcpy(listaDatagrammiRX[k]->acked, "0");
						memset(listaDatagrammiRX[k]->data, 0, BUF_SIZE);
					}
					while (i < totale_datagrammi) {
						printf("i = %d\t\tdatagrammi = %d\n",i,totale_datagrammi);
						limitesupfinestra = limiteinfwindow + sizeTXwindow;
						
						tv.tv_sec = timeout;
						setsockopt(sockfdUDP, SOL_SOCKET, SO_RCVTIMEO, (const char*)&tv, sizeof tv);
						
						if (recvfrom(sockfdUDP, &frame, sizeof(frame), 0, (struct sockaddr *)&servaddr, (socklen_t *)&servlen) > 0) {
							int id_rx = atoi(frame.ID);

							if (!strcmp(frame.type, "get")) {

								if (id_rx >= limiteinfwindow && id_rx < limitesupfinestra) {
									int idx = id_rx % sizeTXwindow;
									if (atoi(listaDatagrammiRX[idx]->acked) == 0) {
										printf("ricevuto  ID =  %d\n", id_rx);
										strcpy(listaDatagrammiRX[idx]->lengthFrame, frame.lengthFrame);
										strcpy(listaDatagrammiRX[idx]->ID, frame.ID);
										memcpy(listaDatagrammiRX[idx]->data, frame.data, BUF_SIZE);
										strcpy(listaDatagrammiRX[idx]->acked, "1");

										if (id_rx == limiteinfwindow) {
											while (atoi(listaDatagrammiRX[limiteinfwindow % sizeTXwindow]->acked) == 1) {
												int c_idx = limiteinfwindow % sizeTXwindow;
												printf("  scritto su file pacchetto con ID =  %d\n", limiteinfwindow);
												fwrite(listaDatagrammiRX[c_idx]->data, 1, atoi(listaDatagrammiRX[c_idx]->lengthFrame), fptr);
												strcpy(listaDatagrammiRX[c_idx]->acked, "0"); 
												limiteinfwindow++;
												printf("i prima dell incremento = %d\n",i);
												i++; 
												printf("i dopo l incremento = %d\n",i);
												if (i == totale_datagrammi) break;
											}
										}
									}

									numeroRandom = rand() % 101;
									if (numeroRandom < probabilitàPerdita) {
										printf("ho perso ACK per ID: %d\n", id_rx);
									} else {
										strcpy(frame.type, "ack");
										sendto(sockfdUDP, &frame, sizeof(frame), 0, (struct sockaddr *)&servaddr, servlen);
									}
								}

								else if (id_rx < limiteinfwindow) {
									numeroRandom = rand() % 101;
									if (numeroRandom >= probabilitàPerdita) {
										printf("ricevuto duplicato ID: %d.rimando ACK.\n", id_rx);
										strcpy(frame.type, "ack");
										sendto(sockfdUDP, &frame, sizeof(frame), 0, (struct sockaddr *)&servaddr, servlen);
									}
								}
							}
						}
					}

					printf("ho ricevuto il file, inizio fine trasmissione\n");
					int fine_confermata = 0;
					while(!fine_confermata) {
						strcpy(frame.type, "fineget");
						sprintf(frame.acked, "%d", 0);
						sprintf(frame.ID, "%d", 0); // ID indifferente
						
						printf("[INVIO] fineget...\n");
						sendto(sockfdUDP, &frame, sizeof(frame), 0, (struct sockaddr *)&servaddr, servlen);
						
						// wait for the ok from server to close the connection
						tv.tv_sec = 1;
						setsockopt(sockfdUDP, SOL_SOCKET, SO_RCVTIMEO, (const char*)&tv, sizeof tv);
						
						if (recvfrom(sockfdUDP, &frame, sizeof(frame), 0, NULL, NULL) > 0) {
							if (strcmp(frame.type, "fineget") == 0 && atoi(frame.acked) == 1) {
								printf("[OK] Chiusura confermata dal server.\n");
								fine_confermata = 1;
							}
						}
					}
					fclose(fptr);
				}
				else {
					printf("File is empty\n");
				}
			}	

		
			/*----------------------------------------------------------------------"put case"-------------------------------------------------------------------------*/
		if ((strcmp(comando, "put") == 0) && (flname[0] != '\0')) {
			printf("Inizio caricamento (put) del file: %s\n", flname);

			if (access(flname, F_OK) == 0) {

				char bufferletti[BUF_SIZE];
				int numletti = 0;
				clock_t inizio, fine;
				int adattivo = 0;

				if (strcmp(argv[6], "adattivo") == 0) {
					adattivo = 1;
				}

				stat(flname, &st);
				off_t f_size_put = st.st_size;
				FILE *fptr_put = fopen(flname, "rb");
				
				uint32_t totale_datagrammi = 0;
				totale_datagrammi = (f_size_put % BUF_SIZE != 0) ? (f_size_put / BUF_SIZE) + 1 : (f_size_put / BUF_SIZE);
				uint32_t totale_net = htonl(totale_datagrammi);
				
				printf("Totale pacchetti da inviare: %d\n", totale_datagrammi);
	

				int ack_n = -1;
				printf("Totale pacchetti inviati: %d\n", ntohl(totale_net));
				while (ack_n != totale_datagrammi) {
					if (sendto(sockfdUDP, comando_send, sizeof(comando_send), 0, (struct sockaddr *) &servaddr, sizeof(servaddr)) == -1) {
						print_error("Client: send");
					}
					sendto(sockfdUDP, &totale_net, sizeof(totale_net), 0, (struct sockaddr *)&servaddr, servlen);
					tv.tv_sec = 1;						setsockopt(sockfdUDP, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));
					if (recvfrom(sockfdUDP, &ack_n, sizeof(ack_n), 0, NULL, NULL) > 0) {
						ack_n = ntohl(ack_n);
					}
				}
				printf("Handshake put completato.\n");


				int trasferimento_concluso = 0;
				int base_put = 0;
				int numdatagramma_put = 0;
					
				while (!trasferimento_concluso) {
					int limite = ((base_put + sizeTXwindow) > totale_datagrammi) ? totale_datagrammi : (base_put + sizeTXwindow);


					while (numdatagramma_put < limite) {
						int cont_idx = numdatagramma_put % sizeTXwindow;
						

						fseek(fptr_put, numdatagramma_put * BUF_SIZE, SEEK_SET);
						numletti = fread(bufferletti, 1, BUF_SIZE, fptr_put);

						sprintf(listaDatagrammiTX[cont_idx]->lengthFrame, "%d", numletti);
						strcpy(listaDatagrammiTX[cont_idx]->type, "put");
						sprintf(listaDatagrammiTX[cont_idx]->ID, "%d", numdatagramma_put);
						memcpy(listaDatagrammiTX[cont_idx]->data, bufferletti, numletti);
						strcpy(listaDatagrammiTX[cont_idx]->acked, "0");
						if ((rand() % 101) >= probabilitàPerdita) {
							printf("invio pacchetto con ID = %d\n", numdatagramma_put);
							sendto(sockfdUDP, listaDatagrammiTX[cont_idx], sizeof(struct frame_t), 0, (struct sockaddr *)&servaddr, servlen);
						} else {
							printf("ho perso il pacchetto con  ID: %d\n", numdatagramma_put);
						}
						
						if(adattivo && numdatagramma_put == base_put) inizio = clock();
						numdatagramma_put++;
					}

					tv.tv_sec = timeout;
					setsockopt(sockfdUDP, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));
					if (recvfrom(sockfdUDP, &frame, sizeof(frame), 0, NULL, NULL) > 0) {
						if (strcmp(frame.type, "ack") == 0) {
							int id_ack = atoi(frame.ID);
							int idx = id_ack % sizeTXwindow;
							if (id_ack >= base_put && id_ack < limite && atoi(listaDatagrammiTX[idx]->acked) == 0) {
								printf("ho ricevuto ack con  ID: %d\n", id_ack);
								strcpy(listaDatagrammiTX[idx]->acked, "1");
								if(adattivo && id_ack == base_put) {
									fine = clock();
									campioneN = (float)(fine - inizio) / CLOCKS_PER_SEC;
									timeout = calcola_timer(&stimaRTT, &deviazioneRTT, &campioneN, &stimaRTTpassata);
									if (timeout < 1) timeout = 1;
									}
								while (base_put < numdatagramma_put && atoi(listaDatagrammiTX[base_put % sizeTXwindow]->acked) == 1) {
									base_put++;
								}
							}
						}
					} else {
						for(int r = base_put; r < numdatagramma_put; r++) {
							int r_idx = r % sizeTXwindow;
							if(atoi(listaDatagrammiTX[r_idx]->acked) == 0) {
								sendto(sockfdUDP, listaDatagrammiTX[r_idx], sizeof(struct frame_t), 0, (struct sockaddr *)&servaddr, servlen);
							}
						}
					}
					if (base_put == totale_datagrammi) trasferimento_concluso = 1;
				}

				int fine_confermata = 0;
				while (!fine_confermata) {
					strcpy(frame.type, "fineput");
					sprintf(frame.acked, "0");
					sendto(sockfdUDP, &frame, sizeof(frame), 0, (struct sockaddr *)&servaddr, servlen);
					tv.tv_sec = 1;
					setsockopt(sockfdUDP, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));
					if (recvfrom(sockfdUDP, &frame, sizeof(frame), 0, NULL, NULL) > 0) {
						if (strcmp(frame.type, "fineput") == 0 && atoi(frame.acked) == 1) {
							printf("Caricamento completato con successo.\n");
							fine_confermata = 1;
						}
					}
				}
				fclose(fptr_put);
				alarm(0);          
        

				for(int k = 0; k < sizeTXwindow; k++) {
					strcpy(listaDatagrammiTX[k]->acked, "0");
				}
				printf("[CLIENT] File caricato, stato resettato.\n");
					} else {
				printf("Errore: file locale non trovato.\n");
			}
		}
		/*-----------------------------------------------------------------------"list case"----------------------------------------------------------------------------*/
		
		if (strcmp(comando, "list") == 0) {
			printf("sono nel caso list\n");
			
			if (sendto(sockfdUDP, comando_send, sizeof(comando_send), 0, (struct sockaddr *) &servaddr, sizeof(servaddr)) == -1) {
				printf("errore nel send\nmandato %s\n",comando_send);		
				print_error("Client: send");
			}
			
			char filename[200];
			memset(filename, 0, sizeof(filename));

			length = sizeof(servaddr);
				
			t_out.tv_sec = 2;
			setsockopt(sockfdUDP, SOL_SOCKET, SO_RCVTIMEO, (char *)&t_out, sizeof(struct timeval)); 	

			if ((numRead = recvfrom(sockfdUDP, filename, sizeof(filename), 0,  (struct sockaddr *) &servaddr, (socklen_t *)&servlen)) < 0)
				printf("client list: ricevuto niente dal server\n");

			t_out.tv_sec = 0;
					setsockopt(sockfdUDP, SOL_SOCKET, SO_RCVTIMEO, (char *)&t_out, sizeof(struct timeval)); 	
						
			if (filename[0] != '\0') {
				printf("Number of bytes recieved = %ld\n", numRead);
				printf("\nThis is the List of files and directories --> \n%s \n", filename);
			}
			else {
				printf("Recieved buffer is empty\n");
			}
		}
			
			/*----------------------------------------------------------------------"exit case"-------------------------------------------------------------------------*/

		 if (strcmp(comando, "exit") == 0) {
			
			for(int i = 0;i<sizeTXwindow;i++){
		
			free(listaDatagrammiTX[i]);
			free(listaDatagrammiRX[i]);

			}
			
			exit(EXIT_SUCCESS);

		}
	}
}
