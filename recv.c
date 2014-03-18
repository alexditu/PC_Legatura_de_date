#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>

#include "lib.h"

	int isPow2 (int no){
		if (no == 0){
			return -1;
			printf ("ERROR\n");
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
			for (i = 0; i < MSGSIZE - 1; i++){
				s += getBit (t.payload[i], j);
			}
			if ((s % 2) == 1){
				setBit (&checkSum, j);
			}
		}

		return checkSum;

	}

	/* functie care verifica daca un cod hamming este corect */
	msg correctHam (msg t){
		int pNo = 0; //nr de biti de paritate ce trebuie verificati
		int parityBit, s, take, k, i, j;
		while (pow2 (pNo) < t.len){
			pNo ++;
		}

		int **pBits;
		pBits =(int **) malloc (8 * sizeof(int*));
		for (i = 0; i < 8; i++){
			pBits[i] =(int *) calloc (pNo, sizeof (int));
		}
		//pBits[i][j] = al j-lea bit de paritate corespunzator bitilor de pe pozitia i

		/* h = mesajul ce trebuie corectat
		 * t = un mesaj ajutator
		 */
		msg h;
		h = t;
		//pun toti bitii de paritate pe 0;
		for (i = 0; i < pNo; i++){
			t.payload [pow2(i)-1] = 0;
		}

		//Creare biti de paritate:
		for (i = 0; i < 8; i++){
			for (j = 0; j < pNo; j++){
				parityBit = pow2 (j);

				take = parityBit; //cati biti trebuie sa iau
				k = parityBit; //incep de pe pozitia bitului de paritate
				k --; //-1, pt ca numeroatrea vectorului e de la 0 nu de la 1
				s = 0;
				/* valoarea bitului de paritate se calculeaza in felul urmator:
				 * (ex pt bitul 2):
				 * P2 = iau 2 biti, sar 2 biti, iau 2 biti, sar 2 biti ...
			 	 * apoi P2 = P2 % 2;
				*/

				while ( k < t.len ){
					s += getBit (t.payload [k], i);

					take --;
					k ++; //trec la bitul urmator
					if (k >= t.len){//sa nu ies din vector
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

				if (getBit (h.payload [parityBit - 1], i) != s){ //daca e gresit, il marchez
					pBits[i][j] = 1;
				}
			}
		}

		/* Pt fiecare coloana (de la 0 la 7) calculez suma bitilor de paritate
		 * gresiti pentru a afla care bit e eronat
		 */

		int bVal;
		for (i = 0; i < 8; i++){
			s = 0;
			for (j = 0; j < pNo; j++){
				if (pBits[i][j] == 1){
					s += pow2(j);
				}
			}

			//coretie bit gresit:
			s--;;
			if (s >= 0){
				bVal = getBit (h.payload[s], i);
				if (bVal == 1){
					clearBit (&h.payload[s], i);
				}else{
					setBit (&h.payload[s], i);
				}
			}
		}

	return h;
	}

	msg getCodeFromHam (msg t) {
		msg h;//unde o sa extrag codul initial
		int i, j, fromPos;
		int pNo = 0;

		while (pow2 (pNo) < t.len){
			pNo ++;
		}
		h.len = t.len - pNo; //nr de biti de paritate
		for (i = 0; i < h.len; i++){
			h.payload[i] = 0;
		}



		for (i = 0; i < 8; i++){
			fromPos = 1;
			for (j = 0; j < h.len; j++){
				fromPos ++;
				while (isPow2(fromPos) == 1){
					fromPos ++;
				}
				if (getBit (t.payload [fromPos - 1], i) == 1){
					setBit (&h.payload [j], i);
				}
			}
		}

		return h;


	}

int main(int argc, char *argv[])
{
	msg r;
	int task_index;
	int fld; //file descriptor

	task_index = atoi(argv[1]);
	printf("[RECEIVER] Receiver starts.\n");
	printf("[RECEIVER] Task index=%d\n", task_index);

	init(HOST, PORT2);

	/* Deschid fisierul de iesire */
	//fld = open ("recv_fileX", O_WRONLY | O_TRUNC | O_CREAT,  0666);

	/**********************************************
	 * 				TASK 0                        *
	 **********************************************/
	if (task_index == 0){
		int no_frames;
		char *filename;

		/* Mai intai primesc un mesaj gol, in care mi se spune cate cadre se trimit */
		if (recv_message(&r) < 0) {
			perror("[RECEIVER] receive message");
			return -1;
		}
		printf("[RECEIVER] Got msg with Init\n");
		memcpy (&no_frames, r.payload, r.len);
		send_message(&r);

		/* Primesc numele fisierului, apoi il deschid */
		if (recv_message(&r) < 0) {
			perror("[RECEIVER] receive message");
			return -1;
		}

		filename = (char *) malloc (r.len + 6);
		strcpy (filename, "recv_");
		memcpy (filename + 5, r.payload, r.len);
		printf("[RECEIVER] Got msg with filename=%s\n", filename);
		send_message (&r);

		/* Deschid fisierul */
		fld = open (filename, O_WRONLY | O_TRUNC | O_CREAT,  0666);



		/* Acum urmeaza sa primesc no_frames cadre */
		int i;
		for (i = 0; i < no_frames; i++){

			if (recv_message(&r) < 0) {
				perror("[RECEIVER] receive message");
				return -1;
			}
			//t.payload [t.len] = '\0';
			printf("[RECEIVER] Got msg #%d.\n", i);
			send_message (&r);

			/* scriu in fisier continutul mesajului */
			write (fld, r.payload, r.len);

		}
	}
	/*************************************************
	 * 			END TASK 0							 *
	 *************************************************/

	/*************************************************
	 * 			TASK 1								 *
	 *************************************************/

	int crt = 0;

	if (task_index == 1){
		int expected_frame = 0, crt_frame;
		int msg_size = MSGSIZE - sizeof (int);
		int no_frames_to_recv;
		char *filename;

		if (recv_message (&r) < 0) {
					perror("[RECEIVER] receive message");
					return -1;
		}
		memcpy (&crt_frame, r.payload + msg_size, 4);

		if (crt_frame == -1){
			no_frames_to_recv = r.len;
			printf ("[R]Got Init message with MAX=%d\n", no_frames_to_recv);
			send_message (&r);

		}else{
			printf ("[R]Error: Something went worng!\n");
		}
		if (recv_message(&r) < 0) {
			perror("[RECEIVER] receive message");
			return -1;
		}
		memcpy (&crt_frame, r.payload+msg_size, 4);

		/* Citesc numele fisierului de output, si apoi deschid fisierul */
		filename = (char *) malloc (r.len + 6);
		strcpy (filename, "recv_");
		if (crt_frame == -2){
			strncpy (filename + 5, r.payload, r.len);
			printf ("[R]Got Init message with filename=%s\n", filename);
			send_message (&r);
		}else{
			printf ("[S] need frame = -2\n");
		}

		fld = open (filename, O_WRONLY | O_TRUNC | O_CREAT,  0666);



		while (1){
			recv_message (&r);
			memcpy (&crt_frame, r.payload+msg_size, 4);
			if (crt_frame == no_frames_to_recv && expected_frame == no_frames_to_recv){
				printf ("[R]Gata\n");
				send_message (&r);

				break;

			}else{
				if (crt_frame == expected_frame){
					printf ("[R]recv %d\n", crt_frame);
					write (fld, r.payload, r.len);
					expected_frame ++;
					send_message (&r);

				}
			}
		}

	}
	/*************************************************
	 * 			END TASK 1							 *
	 *************************************************/
	/*************************************************
	 * 			 TASK 2 							 *
	 *************************************************/
	if (task_index == 2){
		int expected_frame, crt_frame;
		int msg_size = MSGSIZE - sizeof (int);
		int no_frames_to_recv;
		int DIM;

		recv_message (&r);
		memcpy (&crt_frame, r.payload+msg_size, 4);
		if (crt_frame == -1){
			no_frames_to_recv = r.len;
			printf ("[R]Got Init message with MAX=%d\n", no_frames_to_recv);
			send_message (&r);
		}

		recv_message(&r);
		memcpy (&crt_frame, r.payload+msg_size, 4);
		if (crt_frame == -2){
			DIM = r.len;
			printf ("[R]Got Init message with DIM=%d\n", DIM);
			send_message (&r);
		}

		char *filename;
		recv_message(&r);
		filename = (char *) malloc (r.len + 6);
		strcpy (filename, "recv_");
		strncpy (filename+5, r.payload, r.len);

		fld = open (filename, O_WRONLY | O_TRUNC | O_CREAT,  0666);


		memcpy (&crt_frame, r.payload+msg_size, 4);
		if (crt_frame == -3){
			printf ("[R]Got Init message with filename=%s\n", filename);
			send_message (&r);
		}

		msg buffered_msg[DIM];
		int flag[DIM];
		//int no_buffered;
		int i;

		/* Initializez flag[i] cu -1 */
		for (i = 0; i < DIM; i++){
			flag[i] = -1;
		}

		expected_frame = 0;
		int stop;

		int crt;
		crt = 0;

		no_frames_to_recv --;
		while (1){
			recv_message (&r);
			memcpy (&crt_frame, r.payload+msg_size, 4);

			if (crt_frame == no_frames_to_recv && expected_frame == no_frames_to_recv){
				printf ("[R]Got last frame %d.END0\n", crt_frame);
				send_message (&r);
				write (fld, r.payload, r.len);
				crt++;
				break;
			}else{

				if (crt_frame == expected_frame){
					printf ("[R]recv %d\n", crt_frame);
					write (fld, r.payload, r.len);

					crt++;
					if (crt_frame == no_frames_to_recv){
						printf("[R]Got last frame=%d. END1\n", expected_frame);
						break;
					}
					flag[crt_frame % DIM] = -1;

					send_message (&r);
					expected_frame ++;


					/* cat timp am in buffer, in continuarea lui expected,
					 * cadre ce au flagul 1, le scriu si pe ele in fisier
					 * si cresc expected_frame;
					 */
					stop = 0;
					for (i = expected_frame % DIM; i < DIM; i++){
						if (flag[i] == 1){
							r = buffered_msg [i];
							write (fld, r.payload, r.len);

							crt++;


							expected_frame ++;
							flag[i] = -1;
						}else{
							stop = 1;
							break;
						}
					}
					//circularitate
					if (stop == 0){
						for (i = 0; i < crt_frame % DIM; i++){
							if (flag[i] == 1){
								r = buffered_msg [i];
								write (fld, r.payload, r.len);

								crt++;

								expected_frame ++;
								flag[i] = -1;
							}else{
								stop = 1;
								break;
							}
						}
					}
					if (expected_frame > no_frames_to_recv){
						printf ("[R]END!\n");
						break;
					}
				}else{
					/* daca primesc alt cadru decat cel asteptat, il retin in
					 * buffer si il marchez ca primit;
					 */
					if ((crt_frame <= expected_frame + DIM - 1) && (crt_frame <= no_frames_to_recv)){
						printf ("[R]Recieved %d\n", crt_frame);
						buffered_msg [crt_frame % DIM] = r;
						flag [crt_frame % DIM] = 1;
						send_message (&r);
					}
				}

			}

		}
	}
	/*************************************************
	 * 			 END TASK 2							 *
	 *************************************************/

	/*************************************************
	 * 			 TASK 3 							 *
	 *************************************************/
	if (task_index == 3){
		int expected_frame, crt_frame;
		int msg_size = MSGSIZE - sizeof (int) - 1;//dimensiunea mesajului efectiv (a datelor citite din fisier)
		int no_frames_to_recv;
		int DIM;
		char recvCheckSum;//suma de control trimisa de la sender
		char actualCheckSum;//suma de control calculata de reciever
		//NOTA: daca cele 2 sume de control difera, atunci cadrul este corup => discard!
		int checkPos = MSGSIZE - 1;//pozitia sumei de control in cadrul t.payload
		char *filename;


		/* Testez suma de control a cadrului primit. In cazul in care nu trece
		 * testul, inseamna ca e corupt, asa ca nu fac nimic cu el. Deoarece
		 * nu-i trimit confirmare senderului, acesta va intra in timeout, si va
		 * trimite alt pachet.
		 */

		/* Primesc cadrul cu no_frames_to_recv */
		recvCheckSum = '1';
		actualCheckSum = '0';
		while (recvCheckSum != actualCheckSum){
			recv_message (&r);
			memcpy (&crt_frame, r.payload+msg_size, 4);
			memcpy (&recvCheckSum, r.payload + checkPos, 1);
			actualCheckSum = createCheckSum (r);
		}

		if (crt_frame == -1){
			//no_frames_to_recv = r.len;
			memcpy (&no_frames_to_recv, r.payload, r.len);
			printf ("[R]Got Init message with MAX=%d\n", no_frames_to_recv);
			send_message (&r);
		}else{
			printf ("[R] Error Init msg -1\n");
		}

		/* Primesc cadrul cu no_frames_to_recv */
		recvCheckSum = '1';
		actualCheckSum = '0';

		while (recvCheckSum != actualCheckSum){
			recv_message (&r);
			memcpy (&crt_frame, r.payload+msg_size, sizeof(int));
			memcpy (&recvCheckSum, r.payload + checkPos, sizeof (char));
			actualCheckSum = createCheckSum (r);
		}

		if (crt_frame == -2){
			memcpy (&DIM, r.payload, r.len);
			printf ("[R]Got Init message with DIM=%d\n", DIM);
			send_message (&r);
		}else{
			printf ("[R] Error Init msg -2\n");
		}

		/* Primesc cadrul cu numele fisierului */
		recvCheckSum = '1';
		actualCheckSum = '0';

		while (recvCheckSum != actualCheckSum){
			recv_message (&r);
			memcpy (&crt_frame, r.payload+msg_size, sizeof(int));
			memcpy (&recvCheckSum, r.payload + checkPos, sizeof (char));
			actualCheckSum = createCheckSum (r);
		}

		recv_message(&r);
		filename = (char *) malloc (r.len + 6);
		strcpy (filename, "recv_");
		if (crt_frame == -3){
			memcpy (filename + 5, r.payload, r.len);
			printf ("[R]Got Init message with filename=%s\n", filename);
			send_message (&r);
		}else{
			printf ("[R] Error Init msg -2\n");
		}

		/* Deschid fisierul de output */
		fld = open (filename, O_WRONLY | O_TRUNC | O_CREAT,  0666);

		msg buffered_msg[DIM];
		int flag[DIM];
		//int no_buffered;
		int i;

		/* Initializez flag[i] cu -1 */
		for (i = 0; i < DIM; i++){
			flag[i] = -1;
		}

		expected_frame = 0;
		int stop;

		int crt;
		crt = 0;

		no_frames_to_recv --;
		while (1){
		//	recv_message (&r);
			//memcpy (&crt_frame, r.payload+msg_size, 4);
			recvCheckSum = '1';
			actualCheckSum = '0';

			while (recvCheckSum != actualCheckSum){
				recv_message (&r);
				memcpy (&crt_frame, r.payload+msg_size, sizeof(int));
				memcpy (&recvCheckSum, r.payload + checkPos, sizeof (char));
				actualCheckSum = createCheckSum (r);
			}

			if (crt_frame == no_frames_to_recv && expected_frame == no_frames_to_recv){
				printf ("[R]Got last frame %d.END0\n", crt_frame);
				send_message (&r);
				write (fld, r.payload, r.len);
				break;
			}else{

				if (crt_frame == expected_frame){
					printf ("[R]recv %d\n", crt_frame);
					write (fld, r.payload, r.len);
					if (crt_frame == no_frames_to_recv){
						printf("[R]Got last frame=%d. END1\n", expected_frame);
						break;
					}
					flag[crt_frame % DIM] = -1;
					send_message (&r);
					expected_frame ++;

					stop = 0;
					for (i = expected_frame % DIM; i < DIM; i++){
						if (flag[i] == 1){
							r = buffered_msg [i];
							write (fld, r.payload, r.len);
							expected_frame ++;
							flag[i] = -1;
						}else{
							stop = 1;
							break;
						}
					}
					//circularitate
					if (stop == 0){
						for (i = 0; i < crt_frame % DIM; i++){
							if (flag[i] == 1){
								r = buffered_msg [i];
								write (fld, r.payload, r.len);

								crt++;

								expected_frame ++;
								flag[i] = -1;
							}else{
								stop = 1;
								break;
							}
						}
					}
					if (expected_frame > no_frames_to_recv){
						printf ("[R]END\n");
						break;
					}
				}else{
					/* daca primesc alt cadru decat cel asteptat, il retin in
					 * buffer si il marchez ca primit;
					 */
					if ((crt_frame <= expected_frame + DIM - 1) && (crt_frame <= no_frames_to_recv)){
						printf ("[R]Recieved %d\n", crt_frame);
						buffered_msg [crt_frame % DIM] = r;
						flag [crt_frame % DIM] = 1;
						send_message (&r);
					}
				}

			}

		}
	}


	/*************************************************
	 * 			 TASK 4							 *
	 *************************************************/
	if (task_index == 4){
		int expected_frame, crt_frame;
		//int msg_size = MSGSIZE - sizeof (int) - 11;//dimensiunea mesajului efectiv (a datelor citite din fisier)
		int no_frames_to_recv;
		int DIM;
		msg correctMsg;

		char *filename;


		recv_message (&r);
		correctMsg = correctHam (r);
		r = getCodeFromHam (correctMsg);
		r.len -= 4;
		filename = (char *) malloc (r.len + 6);
		strcpy (filename, "recv_");
		strncpy (filename + 5, r.payload, r.len);
		memcpy (&crt_frame, r.payload + r.len, sizeof (int));
		if (crt_frame == -1){
			printf ("[R]Got Init message with filename=%s\n", filename);
			send_message (&r);
		}
		fld = open (filename, O_WRONLY | O_TRUNC | O_CREAT,  0666);

		recv_message (&r);
		correctMsg = correctHam (r);
		r = getCodeFromHam (correctMsg);
		r.len -= 4;
		memcpy (&DIM, r.payload, r.len);
		memcpy (&crt_frame, r.payload + r.len, sizeof (int));
		if (crt_frame == -2){
			printf ("[R]Got Init message with DIM=%d\n", DIM);
			send_message (&r);
		}

		recv_message (&r);
		correctMsg = correctHam (r);
		r = getCodeFromHam (correctMsg);
		r.len -= 4;
		memcpy (&no_frames_to_recv, r.payload, r.len);
		memcpy (&crt_frame, r.payload + r.len, sizeof (int));
		if (crt_frame == -3){
			printf ("[R]Got Init message with no_frames_to_recv=%d\n", no_frames_to_recv);
			send_message (&r);
		}


		msg buffered_msg[DIM];
		int flag[DIM];
		int i;

		/* Initializez flag[i] cu -1 */
		for (i = 0; i < DIM; i++){
			flag[i] = -1;
		}

		expected_frame = 0;
		int stop;

		int crt;
		crt = 0;

		no_frames_to_recv --;


		while (1){
			recv_message (&r);
			correctMsg = correctHam (r);
			r = getCodeFromHam (correctMsg);
			r.len -= 4;
			memcpy (&crt_frame, r.payload + r.len, sizeof(int));


			if (crt_frame == no_frames_to_recv && expected_frame == no_frames_to_recv){
				printf ("[R]Got last frame %d.END0\n", crt_frame);
				send_message (&r);
				write (fld, r.payload, r.len);
				break;
			}else{

				if (crt_frame == expected_frame){
					printf ("[R]recv %d\n", crt_frame);
					write (fld, r.payload, r.len);
					if (crt_frame == no_frames_to_recv){
						printf("[R]Got last frame=%d. END1\n", expected_frame);
						break;
					}
					flag[crt_frame % DIM] = -1;
					send_message (&r);
					expected_frame ++;

					stop = 0;
					for (i = expected_frame % DIM; i < DIM; i++){
						if (flag[i] == 1){
							r = buffered_msg [i];
							write (fld, r.payload, r.len);
							expected_frame ++;
							flag[i] = -1;
						}else{
							stop = 1;
							break;
						}
					}
					//circularitate
					if (stop == 0){
						for (i = 0; i < crt_frame % DIM; i++){
							if (flag[i] == 1){
								r = buffered_msg [i];
								write (fld, r.payload, r.len);
								expected_frame ++;
								flag[i] = -1;
							}else{
								stop = 1;
								break;
							}
						}
					}
					if (expected_frame > no_frames_to_recv){
						printf ("[R]END\n");
						break;
					}
				}else{
					/* daca primesc alt cadru decat cel asteptat, il retin in
					 * buffer si il marchez ca primit;
					 */
					if ((crt_frame <= expected_frame + DIM - 1) && (crt_frame <= no_frames_to_recv)){
						printf ("[R]Recieved %d\n", crt_frame);
						buffered_msg [crt_frame % DIM] = r;
						flag [crt_frame % DIM] = 1;
						send_message (&r);
					}
				}

			}

		}
		printf ("[R] GOT CRT=%d messages!\n", crt);
	}



	close (fld);

	printf("[RECEIVER] All done.\n");
	return 0;
}
