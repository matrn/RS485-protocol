#include "RS485protocol.h"   //import header file



RS485protocol::RS485protocol(byte RXTXcontrol){
	_RTXCpin = RXTXcontrol;   //save RX TX control pin
}


void RS485protocol::begin(Stream &Sserial, byte ID){
	//_RS485->begin(speed);   //begin software serial communication
	_RS485 = &Sserial;   //save pointer to serial communcation

	pinMode(_RTXCpin, OUTPUT);   //set RX TX control pin as output
	digitalWrite(_RTXCpin, 0);   //enable RS485 receiving 

	_ID = ID;   //save ID of this device
}


byte RS485protocol::check(RS485data_struct * RS485_data){   //function for check new data
	byte returnValue = 0;

	/*
	 * Return values:
	 * 0 = no data
	 * 1 = ping response
	 * 2 = new data for this device - communication type 5 or 6 - in structure will be received data
	 * 3 = received correct feedback for data - in structure will be sent data
	 * 4 = error while sending data with feedback due no response - in structure will be sent data
	 * 5 = broadcast message
	 * 6 = uptime in seconds from another device
	 */


	if(_RS485->available()){
		char receivedChar = _RS485->read();   //read received char

		if(receivedChar == 0x02){   //check for start byte
			if(_RSstartReceiving == 0) _RSstartReceiving = 1;   //if not started yet, start receiving new data
			if(_RSstartReceiving == 1){   //clear received data if received new start byte (because no stop byte received)
				_RSdata = "";
			}
		}
		if(_RSstartReceiving == 1){   //if receiving new data already started
			if(receivedChar == 0x03){   //if receivedChar is 0x03 it is end of transmit, so let's parse data
				//----------parse data---------		 
				if(_RSdata[0] == 0x02){   //check start byte
					byte sendType = _RSdata[1];            //get communication type
					byte receiverAddress = _RSdata[2];     //get receiver adress 
					byte senderAddress = _RSdata[3];       //save sender ID
					byte dataType = _RSdata[4];            //save data type
					

					if(receiverAddress >= 5){ receiverAddress -= 5; }else{ receiverAddress = 0; }   //get subtracted receiver address
					if(senderAddress >= 5){ senderAddress -= 5; }else{ senderAddress = 0; }   //get subtracted send address
					if(dataType >= 5){ dataType -= 5; }else{ dataType = 0; }   //get subtracted data type


					if(receiverAddress ==_ID || sendType == 10){   //if receiver adress is same as ID of this arduino continue or if this message is broadcast 
						String data = (_RSdata.substring(5, _RSdata.length()));   //save data

						debug("Communication type: " + String(sendType) + " Data from: " + String(senderAddress) + " Data type: " + String(dataType) + " Data:>" + data + "<");

						if(sendType == 6){   //if sender wants feedback
							sendFeedback(senderAddress, dataType, data.length());   //send feedback 
							debug(F("Feedback sent"));
						}
					
						if(sendType == 7){   //if this is feedback
							for(int a = 0; a < SENT_DATA_ARRAY_LEN; a ++){   //try find this in _sentData array
								if(_sentData_enable[a] == 1){   //check if actual pos is enabled
									if((_sentData[a][2] - 5) == senderAddress && (_sentData[a][3] - 5) == receiverAddress && (_sentData[a][4] - 5) == dataType){   //check for consistency
										if((data).toInt() == _sentData_length[a]){   //check length
											returnValue = 3;   //return = received correct feedback for sendDataWithFeedback

											//save details about data to structure
											RS485_data->sendType = 6;   //data with feedback
											RS485_data->receiverAddress = (int)_sentData[a][2] - 5;   //receiver address
											RS485_data->senderAddress = (int)_sentData[a][3] - 5;   //ID of this device
											RS485_data->dataType = (int)_sentData[a][4] - 5;    //data type
											RS485_data->data = _sentData[a].substring(5, 5 + _sentData_length[a]);    //data

											_sentData_enable[a] = 0;   //stop sending

											debug(F("Feedback is OK"));
							 			}else{   //wrong length of data
											debug("Wrong length in feedback data - expected length: " + String(_sentData_length[a]) + ", received length: " + String((RS485_data->data).toInt()));
										}
									}
								}
							} 
						}

						if(sendType == 5 || sendType == 6){   //if this is new data (normal or with feedback)
							returnValue = 2;   //return = new data for this device and communication type is 5 or 6

							//save data to structure 
							RS485_data->sendType = sendType;   //data with feedback or without feedback
							RS485_data->receiverAddress = receiverAddress;   //receiver address
							RS485_data->senderAddress = senderAddress;   //ID of this device
							RS485_data->dataType = dataType;    //data type
							RS485_data->data = data;    //data
							//sendData(RS485_data->senderAddress, RS485_data->dataType, data);
						}

						if(sendType == 8){   //if this is ping
							sendPingResponse(senderAddress);   //send ping response
							debug(F("ping reponse sent"));
						}

						if(sendType == 9){   //if this is ping response
							debug(F("Received ping response, will parse it :)"));

							unsigned long current_time = millis();   //save current time (millis)

							for(int a = 0; a < SENT_DATA_ARRAY_LEN; a ++){   //try find position in array
								if(_sentData_enable[a] == 1){   //check if actual pos is enabled
									if(_sentData[a][1] == 8 && (_sentData[a][2] - 5) == senderAddress && (_sentData[a][3] - 5) == receiverAddress){   //check for consistency
										returnValue = 1;   //return = received ping response

										//save details about data to structure
										RS485_data->sendType = 8;   //ping
										RS485_data->receiverAddress = (int)_sentData[a][2] - 5;   //receiver address
										RS485_data->senderAddress = (int)_sentData[a][3] - 5;   //ID of this device
										RS485_data->dataType = (int)_sentData[a][4] - 5;    //data type - useless info
										RS485_data->data = String(current_time - _sentData_time[a]);    //time of ping response

										_sentData_enable[a] = 0;   //stop sending

										debug(F("Received ping response"));
									}
								}
							} 
						}

						if(sendType == 10){   //broadcast message
							returnValue = 5;   //return = broadcast message

							//save data to structure 
							RS485_data->sendType = sendType;   //data with feedback or without feedback
							RS485_data->receiverAddress = receiverAddress;   //receiver address (useless)
							RS485_data->senderAddress = senderAddress;   //ID of this device
							RS485_data->dataType = dataType;    //data type
							RS485_data->data = data;    //data
						}

						if(sendType == 11){   //uptime
							returnValue = 6;   //return = uptime

							//save data to structure 
							RS485_data->sendType = sendType;   //data with feedback or without feedback
							RS485_data->receiverAddress = receiverAddress;   //receiver address
							RS485_data->senderAddress = senderAddress;   //ID of this device
							RS485_data->dataType = dataType;    //data type (useless)
							RS485_data->data = data;    //data
						}
					}
				}
				//----------parse data---------

				_RSdata = "";   //clear RS485 variable
	 			_RSstartReceiving = 0;   //disable receiving new data
			}
			else{
				_RSdata += String(receivedChar);   //add data to string
			}  
		}	   
	} 

	if(returnValue == 0){   //if no new data here, we can check for resending data
		for(int a = 0; a < SENT_DATA_ARRAY_LEN; a ++){   //go trough _sentData
			if(_sentData_enable[a] == 1){   //check if repeating sending is enabled on this array position 
				if((millis() - _sentData_time[a]) >= SEND_DATA_REPEAT_TIME){   //check if difference of times is sufficient 
					if(_sentData_attempts[a] >= SEND_DATA_MAX_ATTEMPTS){   //check if attempts of send 
						returnValue = 4;   //return = error while sending data with feedback - no response
					
						//save details about data
						RS485_data->sendType = _sentData[a][1];   //data with feedback
						RS485_data->receiverAddress = (int)_sentData[a][2] - 5;   //receiver address
						RS485_data->senderAddress = (int)_sentData[a][3] - 5;   //ID of this device
						RS485_data->dataType = (int)_sentData[a][4] - 5;    //data type
						RS485_data->data = _sentData[a].substring(5, 5 + _sentData_length[a]);    //data

						_sentData_enable[a] = 0;   //stop sending 

						return returnValue;   //return returnValue because we want to end
					}
					else{   //number of attempts is not sufficient 
						char buff[_sentData[a].length() + 2];
						_sentData[a].toCharArray(buff, _sentData[a].length()+1);
			
						digitalWrite(_RTXCpin, 1);   //enable sending			
						_RS485->write(buff);		  //send data again
						digitalWrite(_RTXCpin, 0);   //disable sending and enable receiving
			
						_sentData_attempts[a] ++;   //add attempt 
						_sentData_time[a] = millis();   //save current millis
						debug("Sending repeated for data type: " + String((int)_sentData[a][4] - 5));
					}
				}
			}
		}
	}
    

	return returnValue;   //return return value
}


void RS485protocol::ping(byte receiverAddress){
	char dataForSend[8];   //data for send

	dataForSend[0] = 0x02;                   //start byte
	dataForSend[1] = 8;                      //ping
	dataForSend[2] = receiverAddress + 5;    //receiver address 
	dataForSend[3] = _ID + 5;                //ID of this device
 	dataForSend[4] = 5;                      //data type - it's for nothing
	dataForSend[5] = 5;                      //data  - it's also trash
	dataForSend[6] = 0x03;                   //stop byte 
	dataForSend[7] = 0;
  
	digitalWrite(_RTXCpin, 1);   //enable sending
	_RS485->write(dataForSend);          //send data
	digitalWrite(_RTXCpin, 0);   //disable sending and enable receiving

	debug("Send array data: revAddr=" + String((int)dataForSend[2]) + ", myID=" + String((int)dataForSend[3]) + ", dataType=" + String((int)dataForSend[4]) + ", and=" +  String((int)dataForSend[5]));     

 	_sentData_position ++;
    if(_sentData_position >= SENT_DATA_ARRAY_LEN) _sentData_position = 0;

    for(int a = 0; a < SENT_DATA_ARRAY_LEN; a ++){   //check, if current data is not same type as some previous data, if yes, remove previous data 
		if(_sentData[a][1] == 8 && _sentData[a][2] == receiverAddress && _sentData[a][3] == _ID){
			_sentData_enable[a] = 0;
 			debug(F("Removed data in data with feedback array because it's same type like this data"));
		}
    }

    debug("Position=" + String(_sentData_position));
    
    // let's save all important datas 
    _sentData_enable[_sentData_position] = 1;             //enable repeating sending data 
 	_sentData_attempts[_sentData_position] = 0;           //zero send attemps 
    _sentData_time[_sentData_position] = millis();        //save current millis
    _sentData_length[_sentData_position] = 6;         //save length of input data 
   
    _sentData[_sentData_position] = dataForSend;
}


void RS485protocol::sendData(byte receiverAddress, byte dataType, String data){
	int inLen = data.length();   //length of input data	

	char dataForSend[inLen + 8];   //data for send

	dataForSend[0] = 0x02;				    //start byte
	dataForSend[1] = 5;					    //data without feedback 
	dataForSend[2] = receiverAddress + 5;	//receiver address 
	dataForSend[3] = _ID + 5;				//ID of this device
	dataForSend[4] = dataType + 5;		    //type of data 

	for(int a = 5; a < inLen + 5; a ++){   //copy input data to dataForSend 
		dataForSend[a] = data[a - 5];
	}
  
	dataForSend[inLen + 5] = 0x03;   //stop byte 
	dataForSend[inLen + 6] = 0;   //null - end of string
  
	digitalWrite(_RTXCpin, 1);   //enable sending
	_RS485->write(dataForSend);		  //send data
	digitalWrite(_RTXCpin, 0);   //disable sending and enable receiving
}


byte RS485protocol::sendDataWithFeedback(byte receiverAddress, byte dataType, String data){   //function for send data with feedback   
	/*
	 * Return values:
	 * 0 - all is all right
	 * 1 - input data is too log to save to the array but data sent
	 */

	byte returnValue = 0;

	int inLen = data.length();   //length of input data    
 
	char dataForSend[inLen + 8];   //data for send

	dataForSend[0] = 0x02;                   //start byte
	dataForSend[1] = 6;                      //data with feedback 
	dataForSend[2] = receiverAddress + 5;    //receiver address 
	dataForSend[3] = _ID + 5;                //ID of this device
 	dataForSend[4] = dataType + 5;           //type of data 

	for(int a = 5; a < inLen + 5; a ++){   //copy input data to dataForSend 
		dataForSend[a] = data[a - 5];
	}
  
	dataForSend[inLen + 5] = 0x03;   //stop byte 
	dataForSend[inLen + 6] = 0;   //end of string
  
	digitalWrite(_RTXCpin, 1);   //enable sending
	_RS485->write(dataForSend);          //send data
	digitalWrite(_RTXCpin, 0);   //disable sending and enable receiving


	if(inLen <= MAX_DATA_SIZE){
 		_sentData_position ++;   //plus position
		if(_sentData_position >= SENT_DATA_ARRAY_LEN) _sentData_position = 0;   //check for maximum pos

		for(int a = 0; a < SENT_DATA_ARRAY_LEN; a ++){   //check, if current data type is not same type as some previous data type, if yes, remove previous data 
	  		if(_sentData[a][2] == receiverAddress && _sentData[a][3] == _ID && _sentData[a][4] == dataType){   //check for consistency
	   			_sentData_enable[a] = 0;
	   	 		debug(F("Removed data in data with feedback array because it's same type like this data"));
	  		}
		}

		debug("Send array data: revAddr=" + String((int)dataForSend[2]) + ", myID=" + String((int)dataForSend[3]) + ", dataType=" + String((int)dataForSend[4]) + ", and=" +  String((int)dataForSend[5])); 
		debug("Position=" + String(_sentData_position));

		// let's save all important datas 
		_sentData_enable[_sentData_position] = 1;             //enable repeating sending data 
	 	_sentData_attempts[_sentData_position] = 0;           //zero send attemps 
		_sentData_time[_sentData_position] = millis();        //save current millis
		_sentData_length[_sentData_position] = inLen;         //save length of input data 
	   
		_sentData[_sentData_position] = dataForSend;
   
	}else{
		returnValue = 1;   //return = too log to save to the array but data sent
		debug(F("too long to save to the array"));
	//error("Length of data is too big for save sent data");
	}
  
	debug(F("Sending data with feedback"));

	return returnValue;   //return retunr value
}


void RS485protocol::broadcast(byte dataType, String data){   //send broadcast message
	int inLen = data.length();   //length of input data	

	char dataForSend[inLen + 8];   //data for send

	dataForSend[0] = 0x02;				    //start byte
	dataForSend[1] = 10;				    //broadcast message
	dataForSend[2] = 5;                  	//receiver address - useless
	dataForSend[3] = _ID + 5;				//ID of this device
	dataForSend[4] = dataType + 5;		    //type of data 

	for(int a = 5; a < inLen + 5; a ++){   //copy input data to dataForSend 
		dataForSend[a] = data[a - 5];
	}
  
	dataForSend[inLen + 5] = 0x03;   //stop byte 
	dataForSend[inLen + 6] = 0;   //null - end of string
  
	digitalWrite(_RTXCpin, 1);   //enable sending
	_RS485->write(dataForSend);		  //send data
	digitalWrite(_RTXCpin, 0);   //disable sending and enable receiving
}


void RS485protocol::sendUptime(byte receiverAddress){   //send uptime in seconds
	char uptime[10];

	sprintf(uptime, "%lu", millis()/1000);
	byte inLen = strlen(uptime);   //length of uptime in string

	char dataForSend[inLen + 8];   //data for send

	dataForSend[0] = 0x02;				    //start byte
	dataForSend[1] = 11;				    //uptime
	dataForSend[2] = receiverAddress + 5;   //receiver address 
	dataForSend[3] = _ID + 5;				//ID of this device
	dataForSend[4] = 5;		    //type of data (useless)

	for(int a = 5; a < inLen + 5; a ++){   //copy input data to dataForSend 
		dataForSend[a] = uptime[a - 5];
	}
  
	dataForSend[inLen + 5] = 0x03;   //stop byte 
	dataForSend[inLen + 6] = 0;   //null - end of string
  
	digitalWrite(_RTXCpin, 1);   //enable sending
	_RS485->write(dataForSend);		  //send data
	digitalWrite(_RTXCpin, 0);   //disable sending and enable receiving
}


void RS485protocol::sendFeedback(byte receiverAddress, byte dataType, int dataLength){   //send RS485 feedback
	char buff[7];   //buffer for save input length in string format

	sprintf(buff, "%d\0", dataLength);   //get received value

	digitalWrite(_RTXCpin, 1);   //enable sending
	_RS485->write(0x02);                   //send start byte
	_RS485->write(7);                      //feedback
	_RS485->write(receiverAddress + 5);    //send adress of receiver
	_RS485->write(_ID + 5);                //send my address
	_RS485->write(dataType + 5);           //send dataType
	_RS485->write(buff);                   //send data
	_RS485->write(0x03);                   //send stop byte
	digitalWrite(_RTXCpin, 0);   //disable sending and enable receiving
}


void RS485protocol::sendPingResponse(byte receiverAddress){   //send RS485 ping response
	digitalWrite(_RTXCpin, 1);   //enable sending
	_RS485->write(0x02);                   //send start byte
	_RS485->write(9);                      //ping response
	_RS485->write(receiverAddress + 5);    //send adress of receiver
	_RS485->write(_ID + 5);                //send my address
	_RS485->write(5);                      //data type - it's for nothing
	_RS485->write(5);					   //data - it'đ also trash
	_RS485->write(0x03);                   //send stop byte
	digitalWrite(_RTXCpin, 0);   //disable sending and enable receiving
}