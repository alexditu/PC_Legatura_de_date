#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>

#include "lib.h"


	int pow2 (int exp){
		return (1 << exp);
	}

	int getBit (char c, int pos){
		if ((c & pow2 (pos)) == pow2 (pos)){
			return 1;
		}else{
			return 0;
		}
	}

	char clearBit (char *c, int pos){
		*c = *c & (~ pow2 (pos));
		return *c;
	}

	char setBit (char *c, int pos){
		clearBit (c, pos);
		*c = *c | (pow2 (pos));
		return *c;
	}

	int isPow2 (int no){
		if (no == 0){
			return -1;
			printf ("ERRORRRRRRRRRRRRR\n");
		}
		if (no == 1){
			return 1;
		}

		while (no != 1){
			if (no % 2 == 1){
				return 0;
			}
			no = no / 2;
		}
		return 1;
	}

	msg createHam (msg t){
		int pNo = 2, n = t.len;
		while (pNo + n >= pow2(pNo)) {
			pNo ++;
		}

		msg h;
		memset (h.payload, 0, 1400);
		h.len = t.len + pNo;

		int i, j;
		/* copiez in h.payload, t.payload, lasand loc pentru
			bitii de paritate, acestia fi ind initial 0;
		*/
		int bitPos; //deoarece pt codul hamming bitii se numeroteaza de la 1, nu de la 0;

		for (i = 0; i < 8; i++){
			bitPos = 1;
			for (j = 0; j < t.len; j++){
				bitPos ++;
				//daca este putere a lui 2 il sar
				while (isPow2 (bitPos) == 1){
					bitPos ++;
				}
				/* copiez bitul de pe pozitia i, din octetul j, din t.payload,
				 * in h.payload [bitPos], pe pozitia i; adica pregatesc sa fac
				 * hamming de 8 ori: generez un cod hamming pt fiecare din bitii
				 * de la 0 la 7;
				 */
				if (getBit (t.payload [j], i) == 1){
					setBit (&h.payload [bitPos - 1], i);
				}
			}
		}

		int parityBit, s, take, k;
		/* bitul de paritate; ia valori puteri ale lui 2;
		 * sunt pNo biti de paritate
		 */
		//Creare biti de paritate:
		for (i = 0; i < 8; i++){
			for (j = 0; j < pNo; j++){
				parityBit = pow2 (j);
				//printf ("i=%d j=%d parityBit=%d  ", i, j, parityBit);
				take = parityBit; //cati biti trebuie sa iau
				k = parityBit; //incep de pe pozitia bitului de paritate
				k --; //-1, pt ca numeroatrea vectorului e de la 0 nu de la 1
				s = 0;
				/* valoarea bitului de paritate se calculeaza in felul urmator:
				 * (ex pt bitul 2):
				 * P2 = iau 2 biti, sar 2 biti, iau 2 biti, sar 2 biti ...
				 * apoi P2 = P2 % 2;
				*/

				while ( k < h.len ){
					s += getBit (h.payload [k], i);
					take --;
					k ++; //trec la bitul urmator
					if (k >= h.len){//sa nu ies din vector
						break;
					}
					if (take == 0){ //am luat cati biti am avut nevoie, acuma sar tot atatia biti
						while (take != parityBit){
							take ++;
							k ++; //sar peste biti
						}
					}
				}
				s = s % 2;
				if (s == 1){
					setBit (&h.payload[parityBit - 1], i);
				}
			}
		}
		return h;

	}




	/* createCheckSum */
	/*functie ce creeaza suma de control pentru un mesaj, in felul urmator
	 * este mai degraba un octet/byte de control, obtinut astfel:
	 * 		pentru j de la 0..7
	 * 			adun bitii de pe pozitia j, din fiecare octet al mesajului, si
	 * 			fac suma modulo 2. Daca suma e 1, at pun 1, pe bitul j, din
	 * 			variabila checkSum (care este un char, deci e pe un octet).
	 * 	practic calculez paritatea tuturor bitilor de pe pozitiile 0 pana la 7.
	 * 	obtin astfel variabila checkSum, pe care o atasez mesajului.
	 * 	in recv.c refac acest calcul, si vad compar checkSum-ul obtinut cu cel
	 * 	care a fost trimis de sender. Daca sunt identici, atunci e in regula,
	 * 	altfel, pachetul este inutilizabil.
	 *
	 */
	char createCheckSum (msg t){
		int i, j, s;
		char checkSum = 0x00;

		for (j = 0; j < 8; j++){
			s = 0;
			for (i = 0; i < MSGSIZE - 1; i++){//fac pt tot payload-ul - 1, octetul de control.
				s += getBit (t.payload[i], j);
			}
			if ((s % 2) == 1){
				setBit (&checkSum, j);
			}
		}

		return checkSum;

	}

	int max (int a, int b){
		if (a > b){
			return a;
		}else{
			return b;
		}
	}

int main(int argc, char *argv[])
{
	msg t;
	char *filename;
	int task_index, speed, delay;
	int file_dim, no_frames;
	int fld; //file descriptor
	struct stat f_status;

	task_index = atoi(argv[1]);
	filename = argv[2];
	speed = atoi(argv[3]);
	delay = atoi(argv[4]);

	printf("[SENDER] Sender starts.\n");
	printf("[SENDER] Filename=%s, task_index=%d, speed=%d, delay=%d\n", filename, task_index, speed, delay);

	init(HOST, PORT1);

	/* deschid fisierul, si vad ce dimensiune are */
	fld = open (filename, O_RDONLY);
	fstat (fld, &f_status);
	file_dim = (int) f_status.st_size;




	int i;
	//int sum = 0, sum2 = 0;

	/**********************************************
	 * 				TASK 0                        *
	 **********************************************/
	if (task_index == 0){

		no_frames = file_dim / MSGSIZE;
		if (file_dim % MSGSIZE != 0){
			no_frames ++;
		}

		//calculez dimensiunea ferestrei
		int BDP, DIM;
		int count;

		BDP = speed * delay;
		DIM = (BDP * 1000) / (sizeof (msg) * 8);
		printf ("%s %d \n", "DIM=", DIM);


		/* Trimit 2 cadre de initializare: unul cu nr de cadre ce trebuie sa
		 * primeasca recv, si unul cu numele fisierului
		 */
		t.len = sizeof (int);
		memcpy (t.payload, &no_frames, sizeof (int));
		send_message (&t);
		printf ("[S]Sending no_frames=%d\n", no_frames);
		if (recv_message(&t) < 0) {
			perror("[SENDER] receive error");
		}else{
			printf ("[S]Got ack for no_frames\n");
		}

		/* Trimit numele fisierului */
		t.len = strlen (filename);
		memcpy (t.payload, filename, t.len);
		send_message (&t);
		printf ("[S]Sending filename=%s\n", filename);
		if (recv_message(&t) < 0) {
			perror("[SENDER] receive error");
		}else{
			printf ("[S]Got ack for filename\n");
		}

		/* folosesc aceasta variabila pt situatia in care DIM > no_frames
		 * (am mai putin de trimis decat dimensiunea ferestrei)
		 * 1 = true;
		 * 0 = false;
		 */
		int less = 0;

		/* incept sa trimit DIM cadre */
		for (i = 0; i < DIM; i++){
			count =  read (fld, t.payload, MSGSIZE);
			t.len = count;
			if (count <= 0){//nu mai am ce citi din fisier, s-a terminat;
				less = 1;
				break;
			}else{
				send_message (&t);
				printf ("[SENDER]Message sent.\n");
			}
		}

		/* Acum trimit si restul de cadre, inaintea fiecarui cadru trimis
		 * primesc cate un ACK
		 */
		for (i = 0; i < no_frames - DIM; i++){

			if (recv_message(&t) < 0) {
				perror("[SENDER] receive error");
			} else {
				printf("[SENDER] Got reply with ACK.\n");
			}


			count =  read (fld, t.payload, MSGSIZE);
			t.len = count;
			if (count <= 0){//nu mai am ce citi din fisier, s-a terminat;
				printf ("HOPA!!\n");
				break;
			}else{
				send_message (&t);
				printf ("[SENDER]Message sent.\n");
			}
		}
		/* Cazul in care DIM < no_frames */
		if (less == 0){
			for (i = 0; i < DIM; i++){
				if (recv_message(&t) < 0) {
					perror("[SENDER] receive error");
					} else {
						printf("[SENDER] Got reply with ACK #%d.\n", i);
					//	sum2++;
					}
			}
		/* Cazul in care DIM >= no_frames */
		}else{
			int j;
			for (j = 0; j < i; j++){
				if (recv_message(&t) < 0) {
					perror("[SENDER] receive error");
					} else {
						printf("[SENDER] Got reply with ACK #%d.\n", j);
						//sum2++;
					}
			}

		}

		printf ("%s\n", "[Sender] Task 0 - JOB DONE");
	}

	/*************************************************
	 * 			END TASK 0							 *
	 *************************************************/

	/*************************************************
	 * 			TASK 1								 *
	 *************************************************/
	if (task_index == 1){
		int BDP, DIM, i, count;
		int msg_size = MSGSIZE - sizeof (int);

		no_frames = file_dim / msg_size;
		if (file_dim % msg_size != 0){
			no_frames ++;
		}

		BDP = speed * delay;
		DIM = (BDP * 1000) / (sizeof (msg) * 8);

		msg buffered_msg[DIM]; //bufferul in care salvez cadrele trimise
		int no_buffered=0, expected_ack=0, ack;

		/* Trimit mai intai niste pachete de initializare, pt ca recv sa stie:
		 * Numele fisierului de output, si cate cadre tb sa primeasca, ca sa
		 * stie cand sa se opreasca.
		 */
		t.len = no_frames;
		i = -1;
		memcpy (t.payload + msg_size, &i, 4);
		printf ("[S]Sending Init message with no_frames=%d!\n", no_frames);
		send_message (&t);
		while (recv_message_timeout (&t, max (1000, 2 * delay)) == -1){
			send_message (&t);
		}
		printf ("[S]Done sending Init message with no_frames! Got ack\n");

		/* Trimit numele fisierului de intrare */
		t.len = strlen (filename);
		i = -2;
		memcpy (t.payload, filename, t.len);
		memcpy (t.payload + msg_size, &i, sizeof (int));
		printf ("[S]Sending Init msg with filename=%s\n", filename);
		send_message (&t);
		while (recv_message_timeout (&t, max (1000, 2 * delay)) == -1){
			send_message (&t);
		}
		printf ("[S]Got ack with filename\n");


		for (i = 0; i < DIM; i++){
			count =  read (fld, t.payload, msg_size);
			t.len = count;
			printf ("count=%d ", count);

			memcpy (t.payload+msg_size, &i, 4); //nr de ordine
			buffered_msg [no_buffered] = t;
			no_buffered ++;
			send_message (&t);
		}
		printf ("\n");

		while (1){
			/* daca e timeout, trebuie sa retrimit toate pachetele incepand cu
			 * ack_expected (ack_expected % DIM) - circular
			 */
			if (recv_message_timeout (&t, 1000) == -1){
			printf ("[S]Timeout: sending all, from expected_ack=%d\n", expected_ack);
				for (i = expected_ack % DIM; i < DIM; i++){ //sa fac si circular!!!!
					t = buffered_msg [i];
					printf ("[S]Sending message #%d\n", i);
					send_message (&t);
				}
				for (i = 0; i < expected_ack % DIM; i++){
					t = buffered_msg [i];
					printf ("[S]Sending message #%d\n", i);
					send_message (&t);
				}

			}else{
				/* daca am primit ack si este NO_FRAMES (adica ultimul pachet),
				 * at inseamna ca rcv a primit
				 * ultimul pachet, si totul se termina;
				 */
				memcpy (&ack, t.payload+msg_size, 4);
				if (ack == no_frames){
					printf ("[S ack] Gata ack[%d]\n", no_frames);
					break;
				}else{
					/* ack-ul primit e cel asteptat, mai citesc un pahet
					 * si il trimit mai departe
					 * memeorez pachetul, incrementez expected_ack
					 */
					printf ("[S]Got ack=%d\n", ack);
					count =  read (fld, t.payload, msg_size);
					t.len = count;
					/* in cazul in care count=0,adica nu mai am ce citi, trimit
					 * in continuare pachete (fara valoare), pe care recv, oricum
					 * nu le accepta.
					 */

					memcpy (t.payload+msg_size, &no_buffered, 4); //nr de ordine
					no_buffered ++;
					buffered_msg [ack%DIM] = t;
					send_message (&t);

					printf ("[S2]ack[%d]\n", ack);
					expected_ack ++;
				}
			}
		}
	}
	/*************************************************
	 * 			TASK 2								 *
	 *************************************************/


	if (task_index == 2){
		int BDP, DIM, i, count;
		int msg_size = MSGSIZE - sizeof (int);

		no_frames = file_dim / msg_size;
		if (file_dim % msg_size != 0){
			no_frames ++;
		}

		BDP = speed * delay;
		DIM = (BDP * 1000) / (msg_size * 8);

		msg buffered_msg[DIM]; //bufferul in care salvez cadrele trimise
		int flag[DIM]; //flagul corespunzator fiecarui mesaj din buffer
		/* daca flag[i] == -1, nu s-a primit ack pt acest mesaj
		 * daca flag[i] == 1, s-a primit ack pt acest mesaj
		 */
		int no_buffered=0, expected_ack, ack;

		/* Trimit mai intai niste pachete de initializare, pt ca recv sa stie:
		 * Numele fisierului de output, si cate cadre tb sa primeasca, ca sa
		 * stie cand sa se opreasca.
		 */
		t.len = no_frames;
		i = -1;
		memcpy (t.payload + msg_size, &i, 4);
		printf ("[S]Sending Init message with no_frames=%d!\n", t.len);
		send_message (&t);
		while (recv_message_timeout (&t, 1000) == -1){
			memcpy (t.payload + msg_size, &i, 4);
			send_message (&t);
		}
		printf ("[S]Done sending Init with no_frames! Got ack\n");


		t.len = DIM;
		i = -2;
		memcpy (t.payload + msg_size, &i, 4);
		printf ("[S]Sending Init message with DIM!\n");
		send_message (&t);
		while (recv_message_timeout (&t, 1000) == -1){
			memcpy (t.payload + msg_size, &i, 4);
			send_message (&t);
		}
		printf ("[S]Done sending Init with DIM! Got ack\n");


		t.len = strlen (filename);
		memcpy (t.payload, filename, t.len);
		i = -3;
		memcpy (t.payload + msg_size, &i, 4);
		printf ("[S]Sending Init message with filename=%s!\n", filename);
		send_message (&t);
		while (recv_message_timeout (&t, 1000) == -1){
			memcpy (t.payload + msg_size, &i, 4);
			send_message (&t);
		}
		printf ("[S]Done sending Init with filename Got ack\n");

		/* Initializez flag[i] cu 1, pt a nu trimite cadre inutile lui recv:
		 * in cazul in fisierul intra in mai putine cadre decat dimensiunea
		 * ferestrei (DIM).
		 */
		for (i = 0; i < DIM; i++){
			flag[i] = 1;
		}

		/* Trimit DIM cadre */
		for (i = 0; i < DIM; i++){
			count =  read (fld, t.payload, msg_size);
			t.len = count;
			if (count == 0){
				break;
			}
			printf ("count=%d ", count);


			memcpy (t.payload+msg_size, &i, 4); //nr de ordine
			buffered_msg [no_buffered] = t;
			flag[no_buffered] = -1;//mesaj neconfirmat;
			no_buffered ++;
			send_message (&t);
		}
		printf ("\n");
		expected_ack = 0;
		int stop;
		//int j;
		while (1){
			/* Daca am primit timeout, inseamna ca s-au pierdut cadre, deci
			 * trebuie sa retrimit toate cadrele din buffer care nu au fost
			 * confirmate, adica au flagul -1, circular
			 */
			if (recv_message_timeout (&t, 1000) == -1){
				printf ("[S]Timeout\n");
				for (i = expected_ack % DIM; i < DIM; i++){
					if (flag[i] == -1){
						t = buffered_msg[i];
						printf ("[S11]Sending msg #%d\n", expected_ack + i - expected_ack % DIM);
						send_message (&t);
					}
				}
				for (i = 0; i < expected_ack % DIM; i++){
					if (flag[i] == -1){
						t = buffered_msg[i];
						printf ("[S12]Sending msg #%d\n", expected_ack + i - expected_ack % DIM);
						send_message (&t);
					}
				}
			}else{
				memcpy (&ack, t.payload+msg_size, 4);
				/* Cazul de final:
				 * daca am primit ack pt ultimul cadru, si sunt confirmate
				 * toate celelalte, s-a terminat;
				 */
				if ((ack == (no_frames-1)) && (expected_ack == (no_frames-1))){
					printf ("[S]GATA1 \n");
					break;
				}else{
					/* Daca am primit ack-ul asteptat, mai citesc un cadru, il
					 * trimit, si cat timp mai am in continuare cadre cu
					 * flagul 1 (adica au fost confirmate), mai citesc in locul
					 * lor,si le trimit;
					 */
					if (ack == expected_ack){
						printf ("[S]ack=%d\n", ack);
						count =  read (fld, t.payload, msg_size);
						t.len = count;
						if (count != 0){
							memcpy (t.payload+msg_size, &no_buffered, 4);
							printf ("[S1]send %d\n", no_buffered);
							no_buffered ++;
							buffered_msg [ack % DIM] = t;
							flag[ack % DIM] = -1;
							send_message (&t);
						}else{
							flag[ack % DIM] = 1;
						}
						expected_ack ++;

						stop = 0;
						for (i = expected_ack % DIM; i < DIM; i++){
							if (flag[i] == 1 && stop == 0){
								count =  read (fld, t.payload, msg_size);
								t.len = count;

								if (count != 0){

									memcpy (t.payload+msg_size, &no_buffered, 4);
									printf ("[S2]send %d\n", no_buffered);
									no_buffered ++;
									buffered_msg [i] = t;
									flag[i] = -1;

									send_message (&t);

								}else{
									//printf ("[S2]COUNT=0!!!\n");
								}
								if (stop == 0){
									expected_ack ++;
								}
							}else{
								stop = 1;
							}
						}
						//pentru circularitate:
						for (i = 0; i < ack % DIM; i++){
							if (flag[i] == 1 && stop == 0){
								count =  read (fld, t.payload, msg_size);
								t.len = count;
								if (count != 0){
									memcpy (t.payload+msg_size, &no_buffered, 4);
									printf ("[S]send %d\n", no_buffered);
									no_buffered ++;
									buffered_msg [i] = t;
									flag[i] = -1;
									send_message (&t);
								}
								if (stop == 0){
									expected_ack ++;
								}
							}else{
								stop = 1;
							}
						}
						/* Alta cond de final */
						if (expected_ack >= no_frames){
							printf ("[S]Got confirmation for all frames.END\n");
							break;
						}
					}else{
						/* nu am primit ack-ul asteptat, dar e un ack valid,
						 * adica:
						 * (ack <= expected_ack + dim - 1)
						 * marchez cadrul ca si confirmat
						 */
						if (ack <= expected_ack + DIM - 1){
							flag[ack % DIM] = 1;
							printf ("[S]Got other ack=%d\n", ack);
						}
					}

				}

			}
		}






	//end of task 2
	}
	/*************************************************
	 * 			 END TASK 2							 *
	 *************************************************/

	/*************************************************
	 * 			 TASK 3 							 *
	 *************************************************/
	if (task_index == 3){
		int BDP, DIM, i, count;
		int msg_size = MSGSIZE - sizeof (int) - sizeof (char); //MSGSIZE - 4 bytes (nr de ordine) - 1 byte suma de control
		char checkSum;
		int checkPos = MSGSIZE - 1; //pozitia octetului de control: ultimul octet din t.payload

		no_frames = file_dim / (MSGSIZE-5);
		if (file_dim % (MSGSIZE-5) != 0){
			no_frames ++;
		}

		BDP = speed * delay;
		DIM = (BDP * 1000) / (sizeof(msg) * 8);

		msg buffered_msg[DIM]; //bufferul in care salvez cadrele trimise
		int flag[DIM]; //flagul corespunzator fiecarui mesaj din buffer
		/* daca flag[i] == -1, nu s-a primit ack pt acest mesaj
		 * daca flag[i] == 1, s-a primit ack pt acest mesaj
		 */
		int no_buffered=0, expected_ack, ack;

		/* Trimit mai intai niste pachete de initializare, pt ca recv sa stie:
		 * Numele fisierului de output, si cate cadre tb sa primeasca, ca sa
		 * stie cand sa se opreasca.
		 */
		t.len = sizeof (int);
		i = -1;
		memcpy (t.payload, &no_frames, sizeof (int));
		memcpy (t.payload + msg_size, &i, 4);
		checkSum = createCheckSum (t);
		memcpy (t.payload + checkPos, &checkSum, sizeof (char));
		printf ("[S]Sending Init message with no_frames=%d!\n", no_frames);
		send_message (&t);

		while (recv_message_timeout (&t, max (1000, 2 * delay)) == -1){
			send_message (&t);
		}
		printf ("[S]Done sending Init with no_frames! Got ack\n");

		/* trimit dimensiunea ferestrei */
		i = -2;
		t.len = sizeof (int);
		memcpy (t.payload, &DIM, sizeof(int));
		memcpy (t.payload + msg_size, &i, 4);
		checkSum = createCheckSum (t);
		memcpy (t.payload + checkPos, &checkSum, sizeof (char));
		printf ("[S]Sending Init message with DIM!\n");
		send_message (&t);

		while (recv_message_timeout (&t, max (1000, 2 * delay)) == -1){
			send_message (&t);
		}
		printf ("[S]Done sending Init with DIM! Got ack\n");

		/* trimit numele fisierului */
		i = -3;
		t.len = strlen (filename);
		memcpy (t.payload, filename, t.len);
		memcpy (t.payload + msg_size, &i, 4);
		checkSum = createCheckSum (t);
		memcpy (t.payload + checkPos, &checkSum, sizeof (char));
		send_message (&t);

		while (recv_message_timeout (&t, max (1000, 2 * delay)) == -1){
			send_message (&t);
		}
		printf ("[S]Done sending Init with filename=%s! Got ack\n", filename);


		/* Initializez flag[i] cu 1, pt a nu trimite cadre inutile lui recv:
		 * in cazul in fisierul intra in mai putine cadre decat dimensiunea
		 * ferestrei (DIM).
		 */
		for (i = 0; i < DIM; i++){
			flag[i] = 1;
		}

		/* Trimit DIM cadre */
		for (i = 0; i < DIM; i++){
			count =  read (fld, t.payload, msg_size);
			t.len = count;
			if (count == 0){
				break;
			}

			memcpy (t.payload+msg_size, &i, 4); //nr de ordine
			checkSum = createCheckSum (t);
			memcpy (t.payload + checkPos, &checkSum, sizeof (char)); //suma de control

			buffered_msg [no_buffered] = t;//salvez mesajul (il pun in buffer)
			flag[no_buffered] = -1;//mesaj neconfirmat;
			no_buffered ++;
			send_message (&t);
		}

		expected_ack = 0;
		int stop;

		while (1){
			/* Daca am primit timeout, inseamna ca s-au pierdut cadre, deci
			 * trebuie sa retrimit toate cadrele din buffer care nu au fost
			 * confirmate, adica au flagul -1, circular
			 */
			if (recv_message_timeout (&t, 1000) == -1){
				printf ("[S]Timeout\n");
				for (i = expected_ack % DIM; i < DIM; i++){
					if (flag[i] == -1){
						t = buffered_msg[i]; //NU E NEVOIE SA MAI REFAC SUMA DE CONTROL NU?
						printf ("[S11]Sending msg #%d\n", expected_ack + i - expected_ack % DIM);
						send_message (&t);
					}
				}
				for (i = 0; i < expected_ack % DIM; i++){
					if (flag[i] == -1){
						t = buffered_msg[i];
						printf ("[S12]Sending msg #%d\n", expected_ack + i - expected_ack % DIM);
						send_message (&t);
					}
				}
			}else{
				memcpy (&ack, t.payload+msg_size, 4);
				/* Cazul de final:
				 * daca am primit ack pt ultimul cadru, si sunt confirmate
				 * toate celelalte, s-a terminat;
				 */
				if ((ack == (no_frames-1)) && (expected_ack == (no_frames-1))){
					printf ("[S]GATA1 \n");
					break;
				}else{
					/* Daca am primit ack-ul asteptat, mai citesc un cadru, il
					 * trimit, si cat timp mai am in continuare cadre cu
					 * flagul 1 (adica au fost confirmate), mai citesc in locul
					 * lor,si le trimit;
					 */
					if (ack == expected_ack){
						printf ("[S]ack=%d\n", ack);
						count =  read (fld, t.payload, msg_size);
						t.len = count;
						if (count != 0){
							memcpy (t.payload + msg_size, &no_buffered, 4);
							checkSum = createCheckSum (t);
							memcpy (t.payload + checkPos, &checkSum, sizeof (char));

							printf ("[S1]send %d\n", no_buffered);
							no_buffered ++;
							buffered_msg [ack % DIM] = t;
							flag[ack % DIM] = -1;
							send_message (&t);
						}else{
							flag[ack % DIM] = 1;
						}
						expected_ack ++;

						stop = 0;
						for (i = expected_ack % DIM; i < DIM; i++){
							if (flag[i] == 1 && stop == 0){
								count =  read (fld, t.payload, msg_size);
								t.len = count;

								if (count != 0){

									memcpy (t.payload + msg_size, &no_buffered, 4);
									checkSum = createCheckSum (t);
									memcpy (t.payload + checkPos, &checkSum, sizeof (char));

									printf ("[S2]send %d\n", no_buffered);
									no_buffered ++;
									buffered_msg [i] = t;
									flag[i] = -1;
									//expected_ack ++;
									send_message (&t);

								}
								if (stop == 0){
									expected_ack ++;
								}
							}else{
								stop = 1;
							}
						}
						//as putea pune aici cond:
						//daca expected >= no_frames, gata!
						//pentru circularitate:
						for (i = 0; i < ack % DIM; i++){
							if (flag[i] == 1 && stop == 0){
								count =  read (fld, t.payload, msg_size);
								t.len = count;
								if (count != 0){
									memcpy (t.payload+msg_size, &no_buffered, 4);
									checkSum = createCheckSum (t);
									memcpy (t.payload + checkPos, &checkSum, sizeof (char));

									printf ("[S]send %d\n", no_buffered);
									no_buffered ++;
									buffered_msg [i] = t;
									flag[i] = -1;
									send_message (&t);
								}
								if (stop == 0){
									expected_ack ++;
								}
							}else{
								stop = 1;
							}
						}
						/* Alta cond de final */
						if (expected_ack >= no_frames){
							printf ("[S]Got confirmation for all frames.END\n");
							break;
						}
					}else{
						/* nu am primit ack-ul asteptat, dar e un ack valid,
						 * adica:
						 * (ack <= expected_ack + dim - 1)
						 * marchez cadrul ca si confirmat
						 */
						if (ack <= expected_ack + DIM - 1){
							flag[ack % DIM] = 1;
							printf ("[S]Got other ack=%d\n", ack);
						}
					}

				}

			}
		}
	}





	/*************************************************
	 * 			 TASK 4 							 *
	 *************************************************/
	if (task_index == 4){
		int BDP, DIM, i, count;
		int msg_size = MSGSIZE - sizeof (int) - 11; //MSGSIZE - 4 bytes (nr de ordine) - 11 bytes spatiu pt bitii de paritate
		msg h;


		no_frames = file_dim / msg_size;
		if (file_dim % msg_size != 0){
			no_frames ++;
		}

		BDP = speed * delay;
		DIM = (BDP * 1000) / (MSGSIZE * 8);

		msg buffered_msg[DIM]; //bufferul in care salvez cadrele trimise
		int flag[DIM]; //flagul corespunzator fiecarui mesaj din buffer
		/* daca flag[i] == -1, nu s-a primit ack pt acest mesaj
		 * daca flag[i] == 1, s-a primit ack pt acest mesaj
		 */
		int no_buffered=0, expected_ack, ack;

		/* Trimit mai intai niste pachete de initializare, pt ca recv sa stie:
		 * Numele fisierului de output, si cate cadre tb sa primeasca, ca sa
		 * stie cand sa se opreasca, dimensiunea ferestrei DIM.
		 */

		//-1: numele fisierului de output;
		i = -1;
		t.len = strlen (filename);
		memcpy (t.payload, filename, t.len);
		memcpy (t.payload + t.len, &i, 4);
		t.len += 4;
		h = createHam (t);
		t = h;
		printf ("[S]Sending Init message with filename=%s!\n", filename);
		send_message (&t);

		while (recv_message_timeout (&t, 1000) == -1){
			send_message (&t);
		}
		printf ("[S]Done sending Init with filename! Got ack\n");

		/* -2: dimensiunea ferestrei */
		i = -2;
		t.len = sizeof (int);
		memcpy (t.payload, &DIM, t.len);
		memcpy (t.payload + t.len, &i, 4);
		t.len += 4;
		h = createHam (t);
		t = h;
		printf ("[S]Sending Init message with DIM=%d!\n", DIM);
		send_message (&t);
		while (recv_message_timeout (&t, 1000) == -1){
			send_message (&t);
		}
		printf ("[S]Done sending Init with DIM! Got ack\n");

		/* -3: nr de cadre de trimis: no_frames */
		i = -3;
		t.len = sizeof (int);
		memcpy (t.payload, &no_frames, t.len);
		memcpy (t.payload + t.len, &i, sizeof (int));
		t.len += 4;
		h = createHam (t);
		t = h;
		printf ("[S]Sending Init message with no_frames=%d!\n", no_frames);
		send_message (&t);
		while (recv_message_timeout (&t, 1000) == -1){
			send_message (&t);
		}
		printf ("[S]Done sending Init with no_frames! Got ack\n");

		/* Initializez flag[i] cu 1, pt a nu trimite cadre inutile lui recv:
		 * in cazul in fisierul intra in mai putine cadre decat dimensiunea
		 * ferestrei (DIM).
		 */
		for (i = 0; i < DIM; i++){
			flag[i] = 1;
		}

		/* Trimit DIM cadre */
		for (i = 0; i < DIM; i++){
			count =  read (fld, t.payload, msg_size);
			t.len = count;
			if (count == 0){
				break;
			}

			memcpy (t.payload + t.len, &i, 4); //nr de ordine
			t.len += 4;

			h = createHam (t);
			t = h;
			buffered_msg [no_buffered] = t;//salvez mesajul (il pun in buffer)
			flag[no_buffered] = -1;//mesaj neconfirmat;
			no_buffered ++;
			send_message (&t);
		}

		expected_ack = 0;
		int stop;

		while (1){
			/* Daca am primit timeout, inseamna ca s-au pierdut cadre, deci
			 * trebuie sa retrimit toate cadrele din buffer care nu au fost
			 * confirmate, adica au flagul -1, circular
			 */
			if (recv_message_timeout (&t, 1000) == -1){
				printf ("[S]Timeout\n");
				for (i = expected_ack % DIM; i < DIM; i++){
					if (flag[i] == -1){
						t = buffered_msg[i]; //NU E NEVOIE SA MAI REFAC SUMA DE CONTROL NU?
						printf ("[S11]Sending msg #%d\n", expected_ack + i - expected_ack % DIM);
						send_message (&t);
					}
				}
				for (i = 0; i < expected_ack % DIM; i++){
					if (flag[i] == -1){
						t = buffered_msg[i];
						printf ("[S12]Sending msg #%d\n", expected_ack + i - expected_ack % DIM);
						send_message (&t);
					}
				}
			}else{
				memcpy (&ack, t.payload + t.len, 4);
				/* Cazul de final:
				 * daca am primit ack pt ultimul cadru, si sunt confirmate
				 * toate celelalte, s-a terminat;
				 */
				if ((ack == (no_frames-1)) && (expected_ack == (no_frames-1))){
					printf ("[S]GATA1 \n");
					break;
				}else{
					/* Daca am primit ack-ul asteptat, mai citesc un cadru, il
					 * trimit, si cat timp mai am in continuare cadre cu
					 * flagul 1 (adica au fost confirmate), mai citesc in locul
					 * lor,si le trimit;
					 */
					if (ack == expected_ack){
						printf ("[S]ack=%d\n", ack);
						count =  read (fld, t.payload, msg_size);
						t.len = count;
						if (count != 0){
							memcpy (t.payload + t.len, &no_buffered, 4);
							t.len += 4;
							h = createHam (t);
							t = h;
							printf ("[S1]send %d\n", no_buffered);
							no_buffered ++;
							buffered_msg [ack % DIM] = t;
							flag[ack % DIM] = -1;
							send_message (&t);
						}else{
							flag[ack % DIM] = 1;
						}
						expected_ack ++;

						stop = 0;
						for (i = expected_ack % DIM; i < DIM; i++){
							if (flag[i] == 1 && stop == 0){
								count =  read (fld, t.payload, msg_size);
								t.len = count;

								if (count != 0){

									memcpy (t.payload + t.len, &no_buffered, 4);
									t.len += 4;
									h = createHam (t);
									t = h;

									printf ("[S2]send %d\n", no_buffered);
									no_buffered ++;
									buffered_msg [i] = t;
									flag[i] = -1;
									//expected_ack ++;
									send_message (&t);

								}
								if (stop == 0){
									expected_ack ++;
								}
							}else{
								stop = 1;
							}
						}
						//as putea pune aici cond:
						//daca expected >= no_frames, gata!
						//pentru circularitate:
						for (i = 0; i < ack % DIM; i++){
							if (flag[i] == 1 && stop == 0){
								count =  read (fld, t.payload, msg_size);
								t.len = count;
								if (count != 0){
									memcpy (t.payload + t.len, &no_buffered, 4);
									t.len += 4;
									h = createHam (t);
									t = h;

									printf ("[S]send %d\n", no_buffered);
									no_buffered ++;
									buffered_msg [i] = t;
									flag[i] = -1;
									send_message (&t);
								}
								if (stop == 0){
									expected_ack ++;
								}
							}else{
								stop = 1;
							}
						}
						/* Alta cond de final */
						if (expected_ack >= no_frames){
							printf ("[S]Got confirmation for all frames.END\n");
							break;
						}
					}else{
						/* nu am primit ack-ul asteptat, dar e un ack valid,
						 * adica:
						 * (ack <= expected_ack + dim - 1)
						 * marchez cadrul ca si confirmat
						 */
						if (ack <= expected_ack + DIM - 1){
							flag[ack % DIM] = 1;
							printf ("[S]Got other ack=%d\n", ack);
						}
					}

				}

			}
		}
	}


	printf("[SENDER] Job done.\n");

	close (fld);

	return 0;
}





