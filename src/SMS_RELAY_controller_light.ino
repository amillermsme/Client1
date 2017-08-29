#include <Arduino.h>

/*
​SMS_RELAY_controller_light.ino v 0.3/20160205 - c-uGSM shield v 1.13 /d-u3G shield 1.13 SOFTAWARE EXAMPLE
COPYRIGHT (c) 2015-2016 Dragos Iosub / R&D Software Solutions srl

You are legaly entitled to use this SOFTWARE ONLY IN CONJUNCTION WITH c-uGSM or d-u3G DEVICES USAGE. Modifications, derivates and redistribution 
of this software must include unmodified this COPYRIGHT NOTICE. You can redistribute this SOFTWARE and/or modify it under the terms 
of this COPYRIGHT NOTICE. Any other usage may be permited only after written notice of Dragos Iosub / R&D Software Solutions srl.

This SOFTWARE is distributed is provide "AS IS" in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied 
warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

Dragos Iosub, Bucharest 2016.
http://itbrainpower.net
*/


#define du3GShieldUsage							//un-comment this if you use the d-u3G shield

char phoneNumber[16] = "+18319053945";			//destination number
static char AndrewNumber[16] = "+18319053945";      //destination number

#define FourRelays									//un-comment this to use 2 relays output (1 2xRELAY BOARD)
#define ExtraIO
#define ButtonArray

#define OutPort0   8								//Arduino digital port used to drive Relay0
#define OutPort1   9								//Arduino digital port used to drive Relay1

#if defined(FourRelays)
	#define OutPort2   10								//Arduino digital port used to drive Relay2
	#define OutPort3   11								//Arduino digital port used to drive Relay3
#endif

#if defined(ExtraIO)
  #define OutPort4   12               //Arduino digital port used to drive Strobe Board micro
  #define OutPort5   13               //Arduino digital port used to drive Strobe Board micro
#endif

#if defined(ButtonArray)
  #define ButtonGnd  4               
  #define Button1    A0
  #define Button2    A1
  #define Button3    A2
  #define Button4    A3
#endif
 

static char nunderstand[95] = "BAD COMMAND! digital0,digital1,rel0,rel1,rel2,rel3 >> 1 ->activate relay, 0 ->release relay";
static char executed[40] = " cmd executed";
static char boot[13] = "BOOT EVENT ";


#if defined(du3GShieldUsage)							//for d-u3G hardware only!
	//#define HARDWARERELEASE	1						//un-comment this only if module SN bw. 031000 032999 and comment the next one 
	#define HARDWARERELEASE 	2					//default mode! SN > 033000
#endif

/*#####NO PARAMS TO CHANGE BELLOW#####*/

#define powerPIN         7		//Arduino Digital pin used to power up / power down the modem
#define resetPIN         6		//Arduino Digital pin used to reset the modem 
#define statusPIN        5		//Arduino Digital pin used to monitor if modem is powered 

//int supervisorMask = 1;			//mask monitoring 1 - monitor enabled, 0 - monitor disabled

#if defined (du3GShieldUsage)
	#include "du3G_basic_lbr.h"
	#include "du3G_SMS_lbr.h"
#else
	#include "cuGSM_basic_lbr.h"
	#include "cuGSM_SMS_lbr.h"
#endif

#if (ARDUINO >= 100)
	#include "Arduino.h"
	#include <SoftwareSerial.h>
#else
	#include "WProgram.h"
	#include <NewSoftSerial.h>
#endif

#define UNO_MODE                   //used by modem library
#define BUFFDSIZE 162              //used by modem library

SoftwareSerial agsmSerial(2, 3);   //RX==>2 ,TX soft==>3

char ch;
char buffd[BUFFDSIZE];
char readBuffer[162];
//static char mBuffer[162];
int state=0, i=0, powerState = 0; 

//unsigned long startTime = 0;

void procRespose(char* msg){
	memset(readBuffer,0x00,sizeof(readBuffer));
	sprintf(readBuffer, "%s", msg);
	Serial.println(readBuffer);
	Serial.flush();
	delay(150);
	sendSMS(phoneNumber, readBuffer);			//confirm command 
	delay(100);
}

void setup(){
	Serial.begin(19200);					//start serial connection

	pinMode(OutPort0, OUTPUT);				//configure the UNO Dxx as output
	pinMode(OutPort1, OUTPUT);				//configure the UNO Dxx as output
	digitalWrite(OutPort0, 0);
	digitalWrite(OutPort1, 0);
	#if defined(FourRelays)
		pinMode(OutPort2, OUTPUT);				//configure the UNO Dxx as output
		pinMode(OutPort3, OUTPUT);				//configure the UNO Dxx as output
		digitalWrite(OutPort2, 0);
		digitalWrite(OutPort3, 0);
	#endif
  #if defined(ExtraIO)
    pinMode(OutPort4, OUTPUT);        //configure the UNO Dxx as output
    pinMode(OutPort5, OUTPUT);        //configure the UNO Dxx as output
    digitalWrite(OutPort4, 1);
    digitalWrite(OutPort5, 1);
  #endif

  #if defined(ButtonArray)
    pinMode(ButtonGnd, OUTPUT);       //configure the UNO Dxx Button Array GND
    pinMode(Button1, INPUT);          //configure the UNO Axx as Inputs
    pinMode(Button2, INPUT);
    pinMode(Button3, INPUT);
    pinMode(Button4, INPUT);
    digitalWrite(ButtonGnd, LOW);
    digitalWrite(Button1, INPUT_PULLUP);    //set button inputs to use pullups
    digitalWrite(Button2, INPUT_PULLUP);
    digitalWrite(Button3, INPUT_PULLUP);
    digitalWrite(Button4, INPUT_PULLUP);
  #endif

  

	agsmSerial.begin(9600);					//next, modem setup
	clearagsmSerial();
	clearSerial();
	delay(10);

	modemHWSetup();							//configure UNO IN and OUT to be used with modem

	Serial.flush();
	agsmSerial.flush();
	delay(500);

	Serial.println(F("wait until c-uGSM/d-u3G is ready"));
  Serial.println(F("Return Phone Number:"));
  Serial.println(phoneNumber);
  Serial.println(F("Phone Number Printed."));
	Serial.flush();

	powerOnModem();
  
	clearBUFFD();
	while(strlen(buffd)<1){
		getIMEI();
		delay(500);
	}

	ready4SMS = 0; 
	ready4Voice = 0;

	setupMODEMforSMSusage();

	Serial.println(F("modem ready.. let's proceed"));
	Serial.flush();
	delay(100);

	procRespose((char*)boot);

}

int checkSMSAuthNo(int SMSindex){
  Serial.println(F("A Look at buffd throughout function checkSMSAuthNo..."));
  Serial.println(buffd);
	int ret = 0, isPhNum = 0, i = 0, n = 0;
  clearBUFFD();
	if(ready4SMS != 1) setupMODEMforSMSusage();
	if(totSMS<1) listSMS();
	if(SMSindex > noSMS || SMSindex < 1) return -1;
	int cnt=0;
	int j=0;
	int run=1;
	char c;
	unsigned long startTime = 0;
	clearagsmSerial();
	agsmSerial.print("AT+CMGR=");//send command to modem
  agsmSerial.println(SMSindex);//send command to modem
	//agsmSerial.print(SMSindex);//send command to modem  (FreeTalk SIM not compatible with 2 arg CMGR?)
	//agsmSerial.println(",0");//send command to modem    (FreeTalk SIM not compatible with 2 arg CMGR?)
  delay(20);
	Serial.println(buffd);
	delay(1);
	readline();//just 2 remove modem cmd echo
	Serial.println(buffd);
  delay(1);
  readline();//just 2 remove modem cmd echo
  Serial.println(buffd);
  delay(1);
  if(strstr(buffd, phoneNumber)){
    ret = 1;
  }
  else{
    while (n == 0){
      if (buffd[i++] == 34){
        isPhNum++;
      }
      if (isPhNum == 3){
        while (buffd[i + n] != 34){
          phoneNumber[n] = buffd[i + n]; //parse buffd for ph #
          Serial.println(buffd[i + n]);
          n++;
        }
      }
      
    }
    Serial.println(F("New Phone Number detected!"));
    Serial.println(phoneNumber);
    fATcmd(F("+CPBR=1"));             //Read Position Zero
    delay(200);
    agsmSerial.print("AT+CPBW=1,\"");//send command to modem
    agsmSerial.print(phoneNumber);//send command to modem
    agsmSerial.println("\",145,\"1\"");
    delay(20);
    Serial.println(F("New Phone Number In Phonebook 1:"));
    fATcmd(F("+CPBR=1"));             //Read Position Zero
    ret = 1;
  }

	clearBUFFD();
	clearagsmSerial();	
	return ret;
  //return 1;     //Disable check, let any message thru
}

int buttonCheckHandler() {
  //Check Button Status
  int ButtonFlag;
  ButtonFlag = (analogRead(Button1) < 500) ? 2 : 0;
  ButtonFlag = (analogRead(Button2) < 500) ? 1 : ButtonFlag;
  ButtonFlag = (analogRead(Button3) < 500) ? 4 : ButtonFlag;
  ButtonFlag = (analogRead(Button4) < 500) ? 3 : ButtonFlag;
  switch(ButtonFlag) {
    case 0:
      break;
    case 1:                 //Button 1 Turns ON/OFF Normal Light
      if(digitalRead(OutPort2) == HIGH) {
        digitalWrite(OutPort2,LOW);
      }
      else{
        digitalWrite(OutPort0,LOW);
        digitalWrite(OutPort1,LOW);
        digitalWrite(OutPort3,LOW);
        digitalWrite(OutPort2,HIGH);
      }
      Serial.println(F("ON/OFF Light activated by Button!"));
      delay(250);
      break;
    case 2:                 //Button 2 Turns ON/OFF Pizza Light
      if(digitalRead(OutPort3) == HIGH) {
        digitalWrite(OutPort3,LOW);
      }
      else{
        digitalWrite(OutPort0,LOW);
        digitalWrite(OutPort1,LOW);
        digitalWrite(OutPort2,LOW);
        digitalWrite(OutPort3,HIGH);
      }
      Serial.println(F("Pizza Light activated by Button!"));
      delay(250);  
      break;
    case 3:                 //Button 3 Turns ON/OFF Strobe Light
      if(digitalRead(OutPort1) == HIGH) {
        digitalWrite(OutPort1,LOW);
      }
      else{
        digitalWrite(OutPort0,LOW);
        digitalWrite(OutPort2,LOW);
        digitalWrite(OutPort3,LOW);
        digitalWrite(OutPort1,HIGH);
      }
      Serial.println(F("Strobe Light activated by Button!"));
      delay(250);  
      break;
    case 4:
      digitalWrite(OutPort0,LOW);
      digitalWrite(OutPort1,LOW);
      digitalWrite(OutPort2,LOW);
      digitalWrite(OutPort3,LOW);
      Serial.println(F("All Lights Deactivated by Button!"));
      delay(250);  
      break;
    default:
      break;      
  }
}

void loop(){
	#if defined(ButtonArray)
  	buttonCheckHandler();
  #endif
  
	//process SMS commands - start
	listSMS();																//find the last used SMS location
	clearagsmSerial();
	int cnt;
	cnt = noSMS;
	while (cnt>0){
		if(checkSMSAuthNo(cnt)<1){
			deleteSMS(cnt);
			Serial.println("no auth no");
			clearBUFFD();
		}
		else{
			readSMS(cnt);														//the SMS content will be returned in buffd 
			Serial.print(F("SMS content: "));Serial.flush();delay(50);
			Serial.println(buffd);Serial.flush();delay(50);
		}
		//here process SMS the content - start

		delay(50);
		clearagsmSerial();
		delay(50);

		if(strlen(buffd) > 0){												//non empty SMS
			int OutputVal0 = -1;
			int OutputVal1 = -1;
			int OutputVal2 = -1;
			int OutputVal3 = -1;
      int OutputVal4 = -1;
      int OutputVal5 = -1;
			sscanf(buffd, "%i,%i,%i,%i,%i,%i", &OutputVal0, &OutputVal1, &OutputVal2, &OutputVal3, &OutputVal4, &OutputVal5);
			clearBUFFD();
			sprintf(buffd, "%i,%i,%i,%i,%i,%i >> %s", OutputVal0, OutputVal1, OutputVal2, OutputVal3, OutputVal4, OutputVal5, (char*)executed);
			Serial.println(buffd);
      #if defined(ExtraIO)
        if(OutputVal0<0 || OutputVal1<0 || OutputVal2<0 || OutputVal3<0 || OutputVal4<0 || OutputVal5<0){//check parse message success
      //#if defined(FourRelays)
			#else
				if(OutputVal0<0 || OutputVal1<0){//check parse message success
			#endif
				procRespose((char*)nunderstand);
				//Serial.println("error");
			}
			else{
				digitalWrite(OutPort0, OutputVal0);
				digitalWrite(OutPort1, OutputVal1);
				#if defined(FourRelays)
					digitalWrite(OutPort2, OutputVal2);
					digitalWrite(OutPort3, OutputVal3);
				#endif
        #if defined(ExtraIO)
          delay(250);
          digitalWrite(OutPort4, OutputVal4);
          digitalWrite(OutPort5, OutputVal5);
          delay(250);
          digitalWrite(OutPort4, 1);
          digitalWrite(OutPort5, 1);
        #endif
				procRespose(buffd);
				//Serial.println("fine");
			}

			deleteSMS(cnt);													//free the SMS location
		}
		//here process SMS the content - end
		clearBUFFD();
		clearagsmSerial();
		cnt--;
	}
	//process SMS commands - stop
	delay(250);

}
