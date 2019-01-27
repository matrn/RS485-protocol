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


#ifndef RS485protocol_h
#define RS485protocol_h


#include "Arduino.h"
//#include <SoftwareSerial.h>   // library for serial communication on different pins


//#define DEBUG

#ifdef DEBUG
 #define debug(...) Serial.print(F("DEBUG: ")); Serial.println(__VA_ARGS__)
#else
 #define debug(...)
#endif



#define MAX_DATA_SIZE 256   //maximu size of data to save to sentData array
#define SENT_DATA_ARRAY_LEN 3       //length of array for sent data (used for send data with feedback)
#define SEND_DATA_MAX_ATTEMPTS 5    //maximunm attempts of resending data
#define SEND_DATA_REPEAT_TIME 1000   // time in milliseconds for repeat sending


struct RS485data_struct{    //structure for returning data  
	byte sendType;          //type of send data
	byte receiverAddress;   //ID of receiver
	byte senderAddress;	    //ID of sender
	byte dataType;          //data type
	String data;            //data
};


class RS485protocol{
	public:
		RS485protocol(byte RXTXcontrol);   //main function

		void begin(Stream &Sserial,byte ID);   //begin communication
		byte check(RS485data_struct *);   //check for new data

		void ping(byte receiverAddress);   //ping to specific ID
		void sendData(byte receiverAddress, byte dataType, String data);   //send data without feedback
		byte sendDataWithFeedback(byte receiverAddress, byte dataType, String data);   //send data with feedback
		void broadcast(byte dataType, String data);   //broadcast message to every device
		void sendUptime(byte receiverAddress);   //send uptime of this device in seconds
	private:
		void sendFeedback(byte receiverAddress, byte dataType, int dataLength);   //private function for sending feedback
		void sendPingResponse(byte receiverAddress);   //private functions for sending ping response
		
		byte _RTXCpin;   //pin of RX and TX RS485 control
		byte _ID;   //ID of this device

		String _RSdata = "";   //variable for received data
		byte _RSstartReceiving = 0;   //for enabling reading new data

		Stream * _RS485;   //Software serial declaration

		/* -----variables for sending data with feedback-----*/
		String _sentData[SENT_DATA_ARRAY_LEN];       /* whole sent data (type, data, sender....) */
		byte _sentData_enable[SENT_DATA_ARRAY_LEN];                /* for allow resending */
		unsigned long _sentData_time[SENT_DATA_ARRAY_LEN];   /* millis of last send */
		byte _sentData_attempts[SENT_DATA_ARRAY_LEN];              /* number of attempts */
		int _sentData_length[SENT_DATA_ARRAY_LEN];                /* length of sent data */

		int _sentData_position = 0;   /* position of array */
		/* -----variables for sending data with feedback-----*/
};

#endif