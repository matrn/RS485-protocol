/* linux compile:
	gcc RS485protocol.c functions.c -o RS485protocol
 */

/* 
 * Name: RS485protocol
 * Created by: matrn
 * Finished: 1.  19. 7. 2018
 			 2.  17. 10 2018
 *
 * Source code for open, read and write from serial port in C: https://stackoverflow.com/questions/6947413/how-to-open-read-and-write-from-serial-port-in-c
 */


#include <errno.h>
#include <stdio.h>
#include "functions.h"


#define VERSION 11



int main(){
	char *portname = "/dev/ttyUSB0";   /* serial port name */
	ID = 15;
	
	struct RS485data_struct RS485_data;


	/* -----VERSION-----*/
	printf("#####VERSION V%d#####\n", VERSION);
	/* -----VERSION-----*/


	memset(buffer, 0, sizeof(buffer));   /* setup buffer */

	fd = open(portname, O_RDWR | O_NOCTTY | O_SYNC);   /* open serial port */

	if(fd < 0){
		perror("error opening serial port");
		return -1;
	}

	if(set_interface_attribs(fd, B19200, 0) == -1){   /* set speed to 19,200 bps, 8n1 (no parity) */
		perror("error while settings serial port attributes");
	}

	set_blocking(fd, 0);   /* set no blocking */

	
	if(RS485_sendDataWithFeedback(1, 1, "Hey, testing data with feedback") == 1){
		puts("input data is too log to save to the array but data sent");
	}

	usleep(100000);
	RS485_sendData(1, 13, "Hello, testing data without feedback");	
	usleep(100000);
	RS485_ping(1);
	usleep(100000);
	RS485_broadcast(81, "Hey, broadcasting from C code");
	usleep(100000);
	RS485_sendUptime(1);

	while(1){   /* endless loop */
		byte returnValue = RS485_checkData(&RS485_data);
		if(returnValue != 0){
			if(returnValue == 1){
				printf("[Received ping response] - send type = %d, to %d, from(me) %d, (data type = %d), time=%s ms\n", RS485_data.sendType, RS485_data.receiverAddress, RS485_data.senderAddress, RS485_data.dataType, RS485_data.data);
			}

			if(returnValue == 2){
				printf("[Received data] - send type = %d, receiver ID(me): %d, sender ID: %d, data type = %d, data >%s<\n", RS485_data.sendType, RS485_data.receiverAddress, RS485_data.senderAddress, RS485_data.dataType, RS485_data.data);
			}

			if(returnValue == 3){
				printf("[Received correct feedback] - send type = %d, receiver ID: %d, sender ID(me): %d, data type = %d, data >%s<\n", RS485_data.sendType, RS485_data.receiverAddress, RS485_data.senderAddress, RS485_data.dataType, RS485_data.data);
			}

			if(returnValue == 5){
				printf("[Received broadcast message] - send type = %d, receiver ID (useless): %d, sender ID: %d, data type = %d, data >%s<\n", RS485_data.sendType, RS485_data.receiverAddress, RS485_data.senderAddress, RS485_data.dataType, RS485_data.data);
			}

			if(returnValue == 6){
				printf("[Received uptime] - send type = %d, receiver ID (me): %d, sender ID: %d, data type (useless) = %d, uptime >%s< sec\n", RS485_data.sendType, RS485_data.receiverAddress, RS485_data.senderAddress, RS485_data.dataType, RS485_data.data);
			}

			puts("------------------------------------------");
		}
		
		if(RS485_checkResend(&RS485_data) != 0){   /* something new happened */
			fprintf(stderr, "No feedback for data with send type %d from %d to %d with data type %d and data>%s<\n", RS485_data.sendType, RS485_data.senderAddress, RS485_data.receiverAddress, RS485_data.dataType, RS485_data.data);

			puts("------------------------------------------");
		}

	}

	close(fd);   /* close serial port */

	return 0;
}