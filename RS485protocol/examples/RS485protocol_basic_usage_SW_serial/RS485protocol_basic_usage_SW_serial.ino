#include <RS485protocol.h>
#include <SoftwareSerial.h>   //library for software serial communication using


#define ID 1   //id of this device

SoftwareSerial RS(10, 11);   //RX, TX

RS485protocol RS485(2);   //RX/TX control pin
RS485data_struct RS485_data;   //declare structure


unsigned long last_time_of_uptime = 0;   //time of last sending uptime



void setup(){
  Serial.begin(9600);
  Serial.println("RS485protocol!");

  RS.begin(19200);   //begin SW serial communication with baudrate 19200 baud
  RS485.begin(RS, ID);   //Serial, ID of this device

  RS485.sendDataWithFeedback(15, 3, F("Hi, testing data wiiith feedback, from arduino!"));
  delay(100);
  RS485.sendData(15, 8, F("Hello, testing data without feedback, from ardvino!"));
  delay(100);
  RS485.ping(15);
  delay(100);
  RS485.broadcast(6, "Hello, this is broadcast message");
  delay(100);
  RS485.sendUptime(15);
  
  Serial.println(F("Data sent"));
}


void loop(){
  byte returnValue = RS485.check(&RS485_data);   //check data
  
  if(returnValue > 0){    
    if(returnValue == 1){   //ping
      Serial.print(F("Received ping response "));
      Serial.print("Communication type = " + String(RS485_data.sendType));
      Serial.print(", receiver ID = " + String(RS485_data.receiverAddress));
      Serial.print(", sender ID (me) = " + String(RS485_data.senderAddress));
      Serial.print(", data type = " + String(RS485_data.dataType));
      Serial.println(", time=" + RS485_data.data + " ms");
    }
    if(returnValue == 2){   //new data
      Serial.print(F("Received data! "));
      Serial.print("Communication type = " + String(RS485_data.sendType));
      Serial.print(", receiver ID (me) = " + String(RS485_data.receiverAddress));
      Serial.print(", sender ID = " + String(RS485_data.senderAddress));
      Serial.print(", data type = " + String(RS485_data.dataType));
      Serial.println(", data = >" + RS485_data.data + "<");
    }
    if(returnValue == 3){   //feedback
      Serial.print(F("Received correct feedback! "));
      Serial.print("Communication type = " + String(RS485_data.sendType));
      Serial.print(", receiver ID - sent feedback = " + String(RS485_data.receiverAddress));
      Serial.print(", sender ID (me) = " + String(RS485_data.senderAddress));
      Serial.print(", data type = " + String(RS485_data.dataType));
      Serial.println(", data = >" + RS485_data.data + "<");
    }
    if(returnValue == 4){   //no response
      Serial.print(F("No response for sending data with feedback. "));
      Serial.print("Communication type = " + String(RS485_data.sendType));
      Serial.print(", receiver ID = " + String(RS485_data.receiverAddress));
      Serial.print(", sender ID (me) = " + String(RS485_data.senderAddress));
      Serial.print(", data type = " + String(RS485_data.dataType));
      Serial.println(", data = >" + RS485_data.data + "<");
    }
    if(returnValue == 5){   //broadcast
      Serial.print(F("Received broadcast message! "));
      Serial.print("Communication type = " + String(RS485_data.sendType));
      Serial.print(", receiver ID (useless) = " + String(RS485_data.receiverAddress));
      Serial.print(", sender ID = " + String(RS485_data.senderAddress));
      Serial.print(", data type = " + String(RS485_data.dataType));
      Serial.println(", data = >" + RS485_data.data + "<");
    }
    if(returnValue == 6){   //uptime
      Serial.print(F("Received uptime! "));
      Serial.print("Communication type = " + String(RS485_data.sendType));
      Serial.print(", receiver ID = " + String(RS485_data.receiverAddress));
      Serial.print(", sender ID = " + String(RS485_data.senderAddress));
      Serial.print(", data type (useless) = " + String(RS485_data.dataType));
      Serial.println(", uptime >" + RS485_data.data + "< sec");
    }
    Serial.println(F("--------------------------------"));
  }

  if(millis() - last_time_of_uptime > 1000){   //resend
    last_time_of_uptime = millis();
    RS485.sendUptime(15);
  }
}
