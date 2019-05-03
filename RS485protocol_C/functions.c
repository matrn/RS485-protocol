#include "functions.h"



byte RS485_checkResend(struct RS485data_struct * RS485_data){
	/* 
	 * Return values:
	 * 0 - nothing happened
	 * 1 - error while sending data with feedback due no response - in structure will be sent data
	 */

	int a = 0;   /* variable for everything */

	for(a = 0; a < SENT_DATA_ARRAY_LEN; a ++){   /* gou trough array */
		if(sentData_enable[a] == 1){   /* check if repeating sending is enabled on this array position */
			if(millis() - sentData_time[a] >= SEND_DATA_REPEAT_TIME){   /* check if difference of times is sufficient */
				if(sentData_attempts[a] >= SEND_DATA_MAX_ATTEMPTS){   /* check if attempts of send */
					/* save details about data */
					RS485_data->sendType = sentData[a][1];   /* data with feedback */
					RS485_data->receiverAddress = (int)sentData[a][2] - 5;   /* receiver address */
					RS485_data->senderAddress = (int)sentData[a][3] - 5;   /* ID of this device */
					RS485_data->dataType = (int)sentData[a][4] - 5;    /* data type */
					strncpy(RS485_data->data, sentData[a] + 5, 5 + sentData_length[a]);    /* data */
					RS485_data->data[sentData_length[a]] = 0;   /* end of string */

					sentData_enable[a] = 0;   /* stop sending */

					return 1;   /* return 1 because we want to end */
				}
				else{   /* number of attempts is not sufficient */
					write(fd, sentData[a], strlen(sentData[a]));   /* repeat sending */

					sentData_attempts[a] ++;   /* add attempt */
					sentData_time[a] = millis();   /* save current millis (unix time) */

					#ifdef DEBUG
					printf("DEBUG: Sending repeated - data type = %d\n", (int)sentData[a][4] - 5); 
					#endif
					/*
					puts("||||||||||||||||||||||||||||||||||||||||||||||");
					for(int q = 0; q < strlen(sentData[a]); q ++){
						printf("C%d:%d| ", q, sentData[a][q]);
				}*/
				}
			}
		}
	}
	return 0;   /* return 0 - nothing happened */
}



byte RS485_checkData(struct RS485data_struct * RS485_data){
	/*
	 * Return values:
	 * 0 = no data
	 * 1 = ping response
	 * 2 = new data for this device - communication type 5 or 6 - in structure will be received data
	 * 3 = received correct feedback for data - in structure will be sent data
	 * 4 = 
	 * 5 = received broadcast
	 * 6 = received uptime in seconds from another device
	 */

	byte returnValue = 0;

	char buf[MAX_DATA_SIZE];   /* buffer */
	int recvLen = 0;   /* length of received data */

	memset(buf, 0, sizeof(buf));   /* clear buf */

	recvLen = read(fd, buf, sizeof buf);   /* read serial data */

	if(recvLen > 0){   /* check length */
		int a = 0;   /* variable for everything */
		int fl = 0;   /* variable for going trough data */
		int oldBufferLen = 0;   /* for save length of unchanged buffer */

		byte missByte = 1;   /* if data has start and stop byte, miss byte will be 0, else miss byte will be 1 */
		byte dataNumber = 0;   /* number of received data - counting using start byte */

		int startPosition = 0;   /* position of start byte */
		int stopPosition = -1;   /* position of stop byte */

		char * receivedData;   /* variable for received data */


		#ifdef DEBUG
		puts("----------------------------------------");		
		printf("DEBUG: received length = %d\n", recvLen);
		#endif


		oldBufferLen = (int)strlen(buffer);   /* save length of unchanged buffer */

		sprintf(buffer, "%s%s", buffer, buf);   /* add received data to buffer */
		buffer[recvLen + oldBufferLen] = 0;   /* end of string */

		recvLen = (int)strlen(buffer);   /* save length of composite buffer */


		receivedData = malloc(recvLen + 1);   /* alocate memory */

		strncpy(receivedData, buffer, recvLen);   /* copy data from buffer to receivedData */
		receivedData[recvLen] = '\0';   /* add end char */


		/* count start bytes */
		for(a = 0; a < recvLen; a ++){
			if(receivedData[a] == 0x02){
				dataNumber ++;
			}
		}

		for(fl = 0; fl < dataNumber; fl ++){
			missByte = 1;

			/* let's find start byte position */
			for(a = stopPosition + 1; a < recvLen; a ++){
				if(receivedData[a] == 0x02){
					startPosition = a;   /* save position */
					a = recvLen;   /* leave for loop */
				}
			}

			/* let's find stop byte position */
			for(a = startPosition + 1; a < recvLen; a ++){
				if(receivedData[a] == 0x03){
					stopPosition = a;   /* save position */
					missByte = 0;
					a = recvLen;   /* leave for loop */
				}
				else if(receivedData[a] == 0x02){					
					missByte = 1;
					a = recvLen;   /* leave for loop */
				}
			}

			if(missByte == 1) stopPosition = startPosition;   /* ignore this data and move on */

			#ifdef DEBUG
			printf("DEBUG: Start=%d   Stop=%d\n", startPosition, stopPosition);
			printf("DEBUG: Received length: %d\n", recvLen);
			#endif

			if(missByte == 0){   /* if data has start and stop byte continue in parsing */
				byte sendType = 0;          /* type of send */
				byte receiverAddress = 0;   /* receiver address */
				byte senderAddress = 0;     /* sender address */
				byte dataType = 0;          /* type of data */

				char data[stopPosition - startPosition - 2];   /* final data */

				/* parse data */
				sendType = receivedData[startPosition + 1];
				receiverAddress = receivedData[startPosition + 2];
				senderAddress = receivedData[startPosition + 3];
				dataType = receivedData[startPosition + 4];
		
				if(receiverAddress >= 5){ receiverAddress -= 5;	}else{ receiverAddress = 0; }   /* get substracted receiver address */
				if(senderAddress >= 5){	senderAddress -= 5;	}else{ senderAddress = 0; }   /* get substracted sender address */
				if(dataType >= 5){ dataType -= 5; }else{ dataType = 0; }   /* get substracted data type */


				for(a = startPosition + 5; a < stopPosition; a ++){   /* parse final data */
					data[a - startPosition - 5] = receivedData[a];
				}
				data[stopPosition - startPosition - 5] = '\0';   /* end of string */

				#ifdef DEBUG
				printf("DEBUG: Send type: %d, receiver addres: %d, sender address: %d, data type: %d\n", sendType, receiverAddress, senderAddress, dataType);				
				printf("DEBUG: Data: >%s<\n", data);
				#endif

				if(receiverAddress == ID || sendType == 10){   /* check if these datas is for me, don't steal :) or if this is broadcast*/
					#ifdef DEBUG
					puts("DEBUG: \x1B[32mIt's for me! :) \x1B[0m");
					#endif

					if(sendType == 6){   /* if sender wants feedback */
						RS485_sendFeedback(senderAddress, dataType, strlen(data));   /* send feedback for data */
						#ifdef DEBUG
						puts("DEBUG: Feedback sent");
						#endif
					}

					if(sendType == 7){   /* if this is feedback */
						for(a = 0; a < SENT_DATA_ARRAY_LEN; a ++){   /* try find this data in sent data */
							if(sentData_enable[a] == 1){   /* check if this pos is enabled */
								if((sentData[a][2] - 5) == senderAddress && (sentData[a][3] - 5) == receiverAddress && (sentData[a][4] - 5) == dataType){   /* check for consistency */
									if(atoi(data) == sentData_length[a]){   /* check length of received and sent data */
										returnValue = 3;   /* return = received correct feedback for sendDataWithFeedback */

										/* save details about data */
										RS485_data->sendType = 6;   /* data with feedback */
										RS485_data->receiverAddress = (int)sentData[a][2] - 5;   /* receiver address */
										RS485_data->senderAddress = (int)sentData[a][3] - 5;   /* ID of this device */
										RS485_data->dataType = (int)sentData[a][4] - 5;    /* data type */
										strncpy(RS485_data->data, sentData[a] + 5, 5 + sentData_length[a]);    /* data */
										RS485_data->data[sentData_length[a]] = 0;   /* end of string */

										sentData_enable[a] = 0;   /* stop sending */

										#ifdef DEBUG
										puts("DEBUG: \x1B[36mFeedback is OK\x1B[0m");
										#endif
									}else{
										#ifdef DEBUG
										printf("DEBUG: Wrong length in feedback data - expected length: %d, received length: %d\n", sentData_length[a], atoi(data));
										#endif
									}
								}
							}
						}
					}

					if(sendType == 5 || sendType == 6){
						returnValue = 2;   /* return = new data for this device and communication type is 5 or 6 */

						/* save details about data */
						RS485_data->sendType = sendType;   /* data with feedback or without feedback */
						RS485_data->receiverAddress = receiverAddress;   /* receiver address */
						RS485_data->senderAddress = senderAddress;   /* ID of this device */
						RS485_data->dataType = dataType;    /* data type */
						strcpy(RS485_data->data, data);    /* data */
					}

					if(sendType == 8){   /* if this is ping */
						RS485_send_ping_response(senderAddress);   /* send ping response */
						#ifdef DEBUG
						puts("DEBUG: ping response sent");
						#endif
					}

					if(sendType == 9){   /* if this is ping response */
						unsigned long long current_time = millis();   /* save current millis() (unix time) */

						for(a = 0; a < SENT_DATA_ARRAY_LEN; a ++){   /* try find these datas */
							if(sentData_enable[a] == 1){   /* check if current pos is enabled */
								if(sentData[a][1] == 8 && (sentData[a][2] - 5) == senderAddress && (sentData[a][3] - 5) == receiverAddress){   /* check for consistency */
									returnValue = 1;   /* return = received ping response */

									/* save details about data */
									RS485_data->sendType = 8;   /* ping */
									RS485_data->receiverAddress = (int)sentData[a][2] - 5;   /* receiver address */
									RS485_data->senderAddress = (int)sentData[a][3] - 5;   /* ID of this device */
									RS485_data->dataType = (int)sentData[a][4] - 5;    /* data type - useless info */
									sprintf(RS485_data->data, "%llu", current_time - sentData_time[a]);    /* save ping response time */

									sentData_enable[a] = 0;   /* stop sending */

									#ifdef DEBUG
									printf("DEBUG: Received ping response from ID=%d  time=%llu ms\n", senderAddress, current_time - sentData_time[a]);
									#endif
								}
							}
						}
					}

					if(sendType == 10){   /* broadcast */
						returnValue = 5;   /* return = broadcast */

						/* save details about data */
						RS485_data->sendType = sendType;   /* data with feedback or without feedback */
						RS485_data->receiverAddress = receiverAddress;   /* receiver address - useless*/
						RS485_data->senderAddress = senderAddress;   /* ID of this device */
						RS485_data->dataType = dataType;    /* data type */
						strcpy(RS485_data->data, data);    /* data */
					}

					if(sendType == 11){   /* uptime */
						returnValue = 6;   /* return = uptime in seconds from another device */

						/* save details about data */
						RS485_data->sendType = sendType;   /* data with feedback or without feedback */
						RS485_data->receiverAddress = receiverAddress;   /* receiver address */
						RS485_data->senderAddress = senderAddress;   /* ID of this device */
						RS485_data->dataType = dataType;    /* data type (useless) */
						strcpy(RS485_data->data, data);    /* data */
					}
				}

				//puts("");
			}
			else{
				#ifdef DEBUG
				if(missByte == 1) puts("DEBUG: Missing stop byte, will continue in parsing");
				#endif
			}
		}
		if(missByte == 1){
			strcpy(buffer, buffer + stopPosition);   /* save unparse data */
			buffer[recvLen - stopPosition] = 0;   /* end of string */
		}else{
			memset(buffer, 0, sizeof(buffer));   /* clear buffer */
		}

		free(receivedData);   /* it's polite to free allocated memory */
	}
	else{
		#ifdef DEBUG
		puts("DEBUG: No data");
		#endif
	}

	return returnValue;   /* return return value */
}


void RS485_ping(byte receiverAddress){
	int a = 0;   /* variable for everything */
	char dataForSend[8];   /* data for send */

	dataForSend[0] = 0x02;                  /* start byte */
	dataForSend[1] = 8;                     /* ping */
	dataForSend[2] = receiverAddress + 5;   /* receiver address */
	dataForSend[3] = ID + 5;                /* ID of this device */ 
	dataForSend[4] = 5;                     /* data type - it's for nothing */
	dataForSend[5] = 5;                     /* data  - it's also trash */
	dataForSend[6] = 0x03;                  /* stop byte */
	dataForSend[7] = 0;                     /* end of string */

	write(fd, dataForSend, strlen(dataForSend));   /* send data */

	#ifdef DEBUG
	printf("Sending>%s<\n", dataForSend);
	#endif

	sentData_position ++;   /* plus position */
	if(sentData_position >= SENT_DATA_ARRAY_LEN) sentData_position = 0;   /* check max position */

	for(a = 0; a < SENT_DATA_ARRAY_LEN; a ++){   /* check, if current data is not same type as some previous data, if yes, remove previous data */
		if(sentData[a][1] == 8 && sentData[a][2] == receiverAddress && sentData[a][3] == ID){   /* check for consistency */
			sentData_enable[a] = 0;   /* disable this position */
			#ifdef DEBUG
			puts("Removed data in data with feedback array because it's same type like this data");
			#endif
		}
	}

	/* let's save all important datas */
	sentData_enable[sentData_position] = 1;             /* enable repeating sending data */
	sentData_attempts[sentData_position] = 0;           /* zero send attemps */
	sentData_time[sentData_position] = millis();        /* save current millis (unix time) */
	sentData_length[sentData_position] = 6;             /* save length of input data */
	
	strcpy(sentData[sentData_position], dataForSend);   /* save sent data */

	#ifdef DEBUG
	printf("position = %d\n", sentData_position);
	#endif
}



void RS485_sendData(byte receiverAddress, byte dataType, char * data){
	int a = 0;   /* variable for everything */
	int inLen = strlen(data);   /* length of input data */

	char dataForSend[inLen + 8];   /* data for send */


	dataForSend[0] = 0x02;                  /* start byte */
	dataForSend[1] = 5;                     /* data without feedback */
	dataForSend[2] = receiverAddress + 5;   /* receiver address */
	dataForSend[3] = ID + 5;                /* ID of this device */
	dataForSend[4] = dataType + 5;          /* type of data */

	for(a = 5; a < inLen + 5; a ++){   /* copy input data to dataForSend */
		dataForSend[a] = data[a - 5];
	}

	dataForSend[inLen + 5] = 0x03;   /* stop byte */
	dataForSend[inLen + 6] = 0;   /* end of string */

	write(fd, dataForSend, strlen(dataForSend));   /* send data */

	#ifdef DEBUG
	printf("Sending>%s<\n", dataForSend);
	#endif
}


byte RS485_sendDataWithFeedback(byte receiverAddress, byte dataType, char * data){
	/*
	 * Return values:
	 * 0 - all is all right
	 * 1 - input data is too log to save to the array but data sent
	 */

	byte returnValue = 0;

	int a = 0;   /* variable for everything */
	int inLen = strlen(data);   /* length of input data */

	char dataForSend[inLen + 8];   /* data for send */


	dataForSend[0] = 0x02;                  /* start byte */
	dataForSend[1] = 6;                     /* data with feedback */
	dataForSend[2] = receiverAddress + 5;   /* receiver address */
	dataForSend[3] = ID + 5;                /* ID of this device */
	dataForSend[4] = dataType + 5;          /* type of data */

	for(a = 5; a < inLen + 5; a ++){   /* copy input data to dataForSend */
		dataForSend[a] = data[a - 5];
	}

	dataForSend[inLen + 5] = 0x03;   /* stop byte */
	dataForSend[inLen + 6] = 0;   /* end of string */

	write(fd, dataForSend, strlen(dataForSend));   /* send data */

	#ifdef DEBUG
	printf("Sending>%s<\n", dataForSend);
	#endif

	if(strlen(dataForSend) < MAX_DATA_SIZE){   /* check for data size */
		sentData_position ++;
		if(sentData_position >= SENT_DATA_ARRAY_LEN) sentData_position = 0;

		for(a = 0; a < SENT_DATA_ARRAY_LEN; a ++){   /* check, if current data is not same type as some previous data, if yes, remove previous data */
			if(sentData[a][2] == receiverAddress && sentData[a][3] == ID && sentData[a][4] == dataType){
				sentData_enable[a] = 0;
				puts("Removed data in data with feedback array because it's same type like this data");
			}
		}

		/* let's save all important datas */
		sentData_enable[sentData_position] = 1;             /* enable repeating sending data */
		sentData_attempts[sentData_position] = 0;           /* zero send attemps */
		sentData_time[sentData_position] = millis();        /* save current millis (unix time) */
		sentData_length[sentData_position] = inLen;         /* save length of input data */

		strcpy(sentData[sentData_position], dataForSend);   /* save sent data */

		#ifdef DEBUG
		printf("position = %d\n", sentData_position);
		#endif
	}else{
		returnValue = 1;   /* return = too log to save to the array but data sent */

		#ifdef DEBUG
		fprintf(stderr, "Length of data is too big for save but data sent\n");
		#endif
	}

	return returnValue;
}


void RS485_broadcast(byte dataType, char * data){   /* broadcast */
	int a = 0;   /* variable for everything */
	int inLen = strlen(data);   /* length of input data */

	char dataForSend[inLen + 8];   /* data for send */


	dataForSend[0] = 0x02;                  /* start byte */
	dataForSend[1] = 10;                    /* broadcast */
	dataForSend[2] = 5;   		            /* receiver address (useless) */
	dataForSend[3] = ID + 5;                /* ID of this device */
	dataForSend[4] = dataType + 5;          /* type of data */

	for(a = 5; a < inLen + 5; a ++){   /* copy input data to dataForSend */
		dataForSend[a] = data[a - 5];
	}

	dataForSend[inLen + 5] = 0x03;   /* stop byte */
	dataForSend[inLen + 6] = 0;   /* end of string */

	write(fd, dataForSend, strlen(dataForSend));   /* send data */

	#ifdef DEBUG
	printf("Sending broadcast>%s<\n", dataForSend);
	#endif
}


void RS485_sendUptime(byte receiverAddress){   /* send uptime */
	struct sysinfo si;   /* structure for sysinfo*/
	int a = 0;   /* variable for everything */
	char uptime[10];   /* variable for string uptime */

	sysinfo(&si);   /* get sysinfo */
	sprintf(uptime, "%li", si.uptime);   /* save uptime to string varaible uptime */

	int inLen = strlen(uptime);   /* length of input data */

	char dataForSend[inLen + 8];   /* data for send */


	dataForSend[0] = 0x02;                  /* start byte */
	dataForSend[1] = 11;                    /* broadcast */
	dataForSend[2] = receiverAddress + 5;   /* receiver address */
	dataForSend[3] = ID + 5;                /* ID of this device */
	dataForSend[4] = 5;                     /* type of data (useless) */
 
	for(a = 5; a < inLen + 5; a ++){   /* copy input data to dataForSend */
		dataForSend[a] = uptime[a - 5];
	}

	dataForSend[inLen + 5] = 0x03;   /* stop byte */
	dataForSend[inLen + 6] = 0;   /* end of string */

	write(fd, dataForSend, strlen(dataForSend));   /* send data */

	#ifdef DEBUG
	printf("Uptime: %li sec\n", si.uptime);
	printf("Sending uptime>%s<\n", dataForSend);
	#endif
}


void RS485_sendFeedback(byte receiverAddress, byte dataType, int length){
	unsigned int a = 0;   /* variable for everything */
	char numChar[6];   /* variable for save number in string format */
	char dataForSend[15];   /* data for send */

	sprintf(numChar, "%d", length);   /* convert int to string */

	dataForSend[0] = 0x02;                  /* start byte */
	dataForSend[1] = 7;                     /* feedback */
	dataForSend[2] = receiverAddress + 5;   /* receiver address */
	dataForSend[3] = ID + 5;                /* ID of this device */
	dataForSend[4] = dataType + 5;          /* type of data */

	for(a = 5; a < strlen(numChar) + 5; a ++){   /* copy input data to dataForSend */
		dataForSend[a] = numChar[a - 5];
	}

	dataForSend[strlen(numChar) + 5] = 0x03;   /* stop byte */

	write(fd, dataForSend, strlen(dataForSend));   /* send data */
}


void RS485_send_ping_response(byte receiverAddress){
	char dataForSend[8];


	dataForSend[0] = 0x02;                  /* start byte */
	dataForSend[1] = 9;                     /* ping response */
	dataForSend[2] = receiverAddress + 5;   /* receiver address */
	dataForSend[3] = ID + 5;                /* ID of this device */ 
	dataForSend[4] = 5;                     /* data type - it's for nothing */
	dataForSend[5] = 5;                     /* data  - it's also trash */
	dataForSend[6] = 0x03;                  /* stop byte */
	dataForSend[7] = 0;                     /* end of string */

	write(fd, dataForSend, strlen(dataForSend));   /* send data */
}




unsigned long long millis(){   /* returns current millis (unix time) */
	struct timeval tm;   /* declare structure */

	gettimeofday(&tm, NULL);   /* get time */

	unsigned long long millisecondsSinceEpoch = (unsigned long long)(tm.tv_sec) * 1000 + (unsigned long long)(tm.tv_usec)/1000;   /* calculate millis (unix time) */

	return millisecondsSinceEpoch;   /* return millis time (unix time) */
}




int set_interface_attribs(int fd, int speed, int parity){
	struct termios tty;   /* termios structure for serial attributes */

	memset (&tty, 0, sizeof tty);   /* clear structure */

	if(tcgetattr(fd, &tty) != 0){   /* get serial attributes */
		perror("error %d from tcgetattr");
		return -1;
	}

	cfsetospeed(&tty, speed);   /* set output speed */
	cfsetispeed(&tty, speed);   /* set input speed */
	cfmakeraw(&tty);            /* set RAW mode */

	tty.c_cflag = (tty.c_cflag & ~CSIZE) | CS8;	 /* 8-bit chars */

	tty.c_iflag &= ~IGNBRK;   /* disable break processing - disable IGNBRK for mismatched speed tests; otherwise receive break as \000 chars */
	tty.c_lflag = 0;          /* no signaling chars, no echo, no canonical processing */
	tty.c_oflag = 0;          /* no remapping, no delays */
	tty.c_cc[VMIN]  = 0;      /*read doesn't block */
	tty.c_cc[VTIME] = 5;      /* 0.5 seconds read timeout */

	tty.c_iflag &= ~(IXON | IXOFF | IXANY);   /* shut off xon/xoff ctrl */

	tty.c_cflag |= (CLOCAL | CREAD);     /* ignore modem controls, enable reading */
	tty.c_cflag &= ~(PARENB | PARODD);   /* shut off parity */
	tty.c_cflag |= parity;
	tty.c_cflag &= ~CSTOPB;
	tty.c_cflag &= ~CRTSCTS;

	if(tcsetattr(fd, TCSANOW, &tty) != 0){   /* set serial attributes */
		perror("error while setting attributes");
		return -1;
	}
	return 0;
}


void set_blocking(int fd, int should_block){
	struct termios tty;   /* termios structure for serial attributes */

	memset(&tty, 0, sizeof tty);   /* clear structure */

	if(tcgetattr(fd, &tty) != 0){   /* set actual serial attributes */
		perror("error while getting attributes using tcgetattr");
	}

	tty.c_cc[VMIN]  = should_block ? 1 : 0;
	tty.c_cc[VTIME] = 5;   /* 0.5 seconds read timeout */

	if(tcsetattr (fd, TCSANOW, &tty) != 0){   /* set serial attributes */
		perror("error setting attributes");
	}
}