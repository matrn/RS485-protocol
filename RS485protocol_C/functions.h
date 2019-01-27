/*
 * SEND TYPES:
 *   5 - data without feedback
 *   6 - data with feedback
 *   7 - feedback
 *   8 - ping
 *   9 - ping response
 *   10 - broadcast
 *   11 - uptime
 */

/* 
 * Name: RS485protocol
 * Created by: matrn
 * Finished: 1.  19. 7. 2018
 			 2.  17. 10 2018
 */


#ifndef FUNCTIONS_H_
#define FUNCTIONS_H_

#include <errno.h>
#include <fcntl.h> 
#include <string.h>
#include <termios.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include <sys/sysinfo.h>   /* uptime */

//#define DEBUG

/* Usage of DEBUG:
#ifdef DEBUG
puts("DEBUG");
#endif
*/

typedef unsigned char byte;   /* define data type byte */



/*----------USER DEFINED----------*/
byte ID;   /* ID of this device */
/*----------USER DEFINED----------*/


#define SENT_DATA_ARRAY_LEN 10   /* length of array */
#define MAX_DATA_SIZE 256   /* maximum size of data */
#define SEND_DATA_MAX_ATTEMPTS 5   /* maximum attempts for resending */
#define SEND_DATA_REPEAT_TIME 500   /* time in milliseconds for repeat sending */


struct RS485data_struct{
	byte sendType;
	byte receiverAddress;
	byte senderAddress;	
	byte dataType;
	char data[MAX_DATA_SIZE];
};



int fd;   /* file descriptor */

char buffer[2 * MAX_DATA_SIZE];   /* buffer */

/* -----variables for sending data with feedback-----*/
char sentData[SENT_DATA_ARRAY_LEN][MAX_DATA_SIZE];       /* whole sent data (type, data, sender....) */
int sentData_enable[SENT_DATA_ARRAY_LEN];                /* for allow resending */
unsigned long long sentData_time[SENT_DATA_ARRAY_LEN];   /* millis (unix time) of last send */
int sentData_attempts[SENT_DATA_ARRAY_LEN];              /* number of attempts */
int sentData_length[SENT_DATA_ARRAY_LEN];                /* length of sent data */

int sentData_position;   /* position of array */
/* -----variables for sending data with feedback-----*/

/* -----serial settings functions----- */
int set_interface_attribs (int, int, int);
void set_blocking(int, int);
/* -----serial settings functions----- */

/* -----RS485 functions----- */
byte RS485_checkResend(struct RS485data_struct *);
byte RS485_checkData(struct RS485data_struct *);

void RS485_ping(byte);

void RS485_sendData(byte, byte, char *);
byte RS485_sendDataWithFeedback(byte, byte, char *);

void RS485_broadcast(byte, char *);
void RS485_sendUptime(byte);

void RS485_sendFeedback(byte, byte, int);
void RS485_send_ping_response(byte);
/* -----RS485 functions----- */

/* -----other----- */
unsigned long long millis();
/* -----other----- */

#endif