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
  #define InPort5   13               	//Arduino digital port used to drive Strobe Board micro
#endif

#if defined(ButtonArray)
  #define ButtonGnd  4
  #define Button1    A0
  #define Button2    A1
  #define Button3    A2
  #define Button4    A3
#endif


static char nunderstand[95] = "BAD COMMAND! digital0,digital1,rel0,rel1,rel2,rel3 >> 1 ->activate relay, 0 ->release relay";
static char welcomeTXT[106] = "WELCOME TO LIFE LITE\nPlease text back your registration number to pair your phone with your Life Lite.";
static char pairedTXT[135] = "LIFE LITE PAIRED\nNice to meet you! Did you know you use your Life Lite just by sending me texts? Try it, text me the word REGISTER.";
static char registeredTXT[142] = "Excellent! Complete your emergency medical information at www.lifelite.org/register. When you are done, text NEXT for further instructions.";
static char nextTXT[135] = "Good work! Your light will activate with an emergency call on it’s own. You can also use it via text. Text SUPPORT for instructions.";
static char supportTXT[153] = "INSTRUCTIONS\nText ON to turn on light\nText HELP to turn on emergency light\nText OFF to turn off light\nText SUPPORT to get these instructions again";
static char onTXT[50] = "Your Life Lite is now ON. Text OFF to turn off.";
static char helpTXT[137] = "Your Emergency Life Lite is now ON. Text OFF to turn off. This does not alert emergency services. If you have an emergency call 9-1-1.";
static char offTXT[50] = "Your Life Lite is now OFF. Text ON to turn on.";
static char pizzaTXT[61] = "Your Life Lite is now on PIZZA mode. Text OFF to turn off.";
static char executed[40] = " cmd executed";
static char boot[37] = "Your Life Lite has been activated!";
static char gateAlertTXT[84] = "Your Life Lite has been triggered by your pool gate sensor. Text OFF to turn off.";


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
int gateStatus = 0, gateAlertSent = 0;
//static char mBuffer[162];
int state=0, i=0, powerState = 0, globalCount = 0;
int appvdNumber = 3;
int nextPhonebookEntry;

//unsigned long startTime = 0;

void procRespose(char* msg){
	memset(readBuffer,0x00,sizeof(readBuffer));
	sprintf(readBuffer, "%s", msg);
	Serial.print(F("Sending SMS Content:  "));
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
	digitalWrite(OutPort0, LOW);
	digitalWrite(OutPort1, LOW);
	#if defined(FourRelays)
		pinMode(OutPort2, OUTPUT);				//configure the UNO Dxx as output
		pinMode(OutPort3, OUTPUT);				//configure the UNO Dxx as output
		digitalWrite(OutPort2, LOW);
		digitalWrite(OutPort3, LOW);
	#endif
  #if defined(ExtraIO)
    pinMode(OutPort4, OUTPUT);        //configure the UNO Dxx as output
    pinMode(InPort5, INPUT);        //configure the UNO Dxx as output
    digitalWrite(OutPort4, HIGH);
    digitalWrite(InPort5, INPUT_PULLUP);
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
	Serial.println(F("wait until c-uGSM/d-u3G is ready"));
	Serial.println(F("wait until c-uGSM/d-u3G is ready"));
	Serial.print(F("Arduino version: "));
	Serial.println(ARDUINO);
  Serial.print(F("Default Phone Number:   "));
  Serial.println(phoneNumber);
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

	Serial.println(F("modem ready.. let's proceed and blink"));
	Serial.flush();

	procRespose((char*)boot);
	//Blink the light to indicate boot
	digitalWrite(OutPort2, HIGH);
	delay(200);
	digitalWrite(OutPort2, LOW);

}

int checkSMSAuthNo(int SMSindex){
	int ret = 0, isPhNum = 0, indexPhNum = 0, n = 0, badPhNumberFlag = 0;
	char incomingNumber[16];
	char authStatus[16] = "UNKNOWN";
	char *pch;
	Serial.print(F("Entered checkSMSAuthNo..."));
  clearBUFFD();
	if(ready4SMS != 1) setupMODEMforSMSusage();
	if(totSMS<1) listSMS();
	if(SMSindex > noSMS || SMSindex < 1) return -1;
	int cnt=0;
	int j=0;
	int run=1;
	char c;
	unsigned long startTime = 0;
	Serial.print(F("Variables Declared..."));
	clearagsmSerial();
	Serial.print(F("Sent AT+CMGR..."));
	Serial.print(F("Sending SMSindex to Modem = "));
	itoa(SMSindex,&c,10);
	Serial.println(&c);
	Serial.flush();
	agsmSerial.print("AT+CMGR=");//send command to modem
	delay(1);
	agsmSerial.println(SMSindex);//send command to modem
	//agsmSerial.println(0);
	//agsmSerial.print(SMSindex);//send command to modem  (FreeTalk SIM not compatible with 2 arg CMGR?)
	//agsmSerial.println(",0");//send command to modem    (FreeTalk SIM not compatible with 2 arg CMGR?)
	//fATcmd(F("CMGR=1"));
	delay(20);
	//Serial.println(buffd);
	readline();
	delay(20);
	//Serial.println(buffd);
	readline();
	delay(20);
	Serial.println(buffd);

	//Extract Phone Number
	pch = strtok(buffd,",");
	pch = strtok (NULL, ",");
	strcpy(incomingNumber, pch);
	clearBUFFD();
	clearagsmSerial();
	Serial.print(F("Incoming Text Phone Number: "));
	Serial.println(incomingNumber);
	delay(100);

	readline();
	delay(20);
	Serial.println(buffd);

	//Get Authorization Status of phone number
	for(n=1; n<nextPhonebookEntry; n++){
		agsmSerial.print("AT+CPBR=");//send command to modem
		agsmSerial.println(n);//send command to modem
		delay(20);
		readline();
		delay(20);
		readline();
		delay(20);
		pch = strtok(buffd,",");
		pch = strtok (NULL, ",");
		if(strstr(pch,incomingNumber)){
			pch = strtok (NULL, ",");
			pch = strtok (NULL, ",");
			strcpy(authStatus, pch);
			n = nextPhonebookEntry;
		}
		clearagsmSerial();
		clearBUFFD();
	}
	Serial.print(F("Incoming Authorization Status: "));
	Serial.println(authStatus);


	ret = (strstr(authStatus,"ROOT")) ? 1 : 0;				//Root # used. All Good, process command.
	ret = (strstr(authStatus,"UNKNOWN")) ? 2 : ret;		//New Phone number detected
	ret = (strstr(authStatus,"KNOWN")) ? 3 : ret;			//Seen, not yet paired
	ret = (strstr(authStatus,"PAIRED")) ? 4 : ret;		//Paired, not yet registered
	ret = (strstr(authStatus,"AUTH")) ? 5 : ret;			//Registered.

	switch(ret){
		case 0:
			Serial.println(F("Unknown Status...Shouldn't be Possible."));
			ret = 0;
			break;
		case 1:
			Serial.println(F("ROOT Phone Number Recognized"));
			break;
		case 2:
			Serial.println(F("UNKNOWN Phone Number Recognized"));
			ret = 0;
			break;
		case 3:
			Serial.println(F("KNOWN Phone Number Recognized"));
			ret = 0;
			break;
		case 4:
			Serial.println(F("PAIRED Phone Number Recognized"));
			ret = 0;
			break;
		case 5:
			Serial.println(F("AUTH Phone Number Recognized"));
			break;
	}


	if(strstr(incomingNumber, phoneNumber) && appvdNumber == 3){
    ret = 1;
		Serial.println(F("Phone Number Recognized"));
  }
	else if(strstr(buffd,"1234")){
		appvdNumber = 1;
		Serial.println(F("PIN entered successfully"));
		procRespose(pairedTXT);
		Serial.println(pairedTXT);
		clearBUFFD();
	}
	else if(strstr(buffd,"OK")){
		//This catches fuckups by CMGR. Sometimes CMGR returns "OK" because the modem is fucking retarded
		Serial.println(F("Modem or sim fuckup. Deleting all messages."));
		deleteSMS(0);
	}
  else{
    while (n == 0){
      if (buffd[indexPhNum++] == 34){
        isPhNum++;
				Serial.print(F("Comma found! isPhNum = "));
				Serial.println(isPhNum);
      }
      if (isPhNum == 3){
        while (buffd[indexPhNum + n] != 34){
          phoneNumber[n] = buffd[indexPhNum + n]; //parse buffd for ph #
					Serial.print(indexPhNum);
					Serial.print(F("\t"));
          Serial.println(buffd[indexPhNum + n]);
          n++;
        }
      }
			if(indexPhNum > 30){
				Serial.print(F("Bad Phone # in Caller ID: "));
				Serial.println(buffd);
				badPhNumberFlag = 1;
				break;
			}
    }
		if(!badPhNumberFlag){
			clearBUFFD();
			Serial.print(F("n = "));
	    Serial.println(n);
	    Serial.print(F("New Phone Number detected: "));
	    Serial.println(phoneNumber);
			appvdNumber = 0;
	    //fATcmd(F("+CPBR=1"));             //Read Position Zero
	    //delay(200);
	    /*agsmSerial.print("AT+CPBW=1,\"");//send command to modem
	    agsmSerial.print(phoneNumber);//send command to modem
	    agsmSerial.println("\",145,\"1\"");
	    delay(200);*/
			sprintf(buffd,"AT+CPBW=%i,\"%s\",145,\"1\"\r",nextPhonebookEntry,phoneNumber);
			agsmSerial.print(buffd);
			delay(200);
			Serial.print(F("Writing New Phone Number: "));
	    Serial.println(buffd);
			clearBUFFD();
			clearagsmSerial();
	    Serial.print(F("New Phone Number In Phonebook 1:   "));
			fATcmd(F("+CPBR=1"));             //Read Position Zero
		  delay(1);                               //Dump 2 lines of AT echo
		  //readline();
		  delay(1);
		  //readline();
			Serial.println(buffd);
			procRespose(welcomeTXT);
			Serial.println(welcomeTXT);
			clearBUFFD();
	    ret = 0;
		}
  }

	clearBUFFD();
	clearagsmSerial();
	return ret;
  //return 1;     //Disable check, let any message thru
}

int buttonCheckHandler() {
  //Check Button Status
	if (!ready4SMS){
		return -1;
	}
  int ButtonFlag;
  ButtonFlag = (analogRead(Button1) < 500) ? 2 : 0;
  ButtonFlag = (analogRead(Button2) < 500) ? 1 : ButtonFlag;
  ButtonFlag = (analogRead(Button3) < 500) ? 4 : ButtonFlag;
  ButtonFlag = (analogRead(Button4) < 500) ? 3 : ButtonFlag;
	ButtonFlag = (digitalRead(InPort5) == HIGH && gateStatus == 0) ? 5 : ButtonFlag;
	ButtonFlag = (digitalRead(InPort5) == LOW && gateStatus == 1) ? 6 : ButtonFlag;


	if(ButtonFlag){			//DEBUG: Serial Comm of Button Content
		Serial.print(F("Button Detected, Status = "));
		Serial.println(ButtonFlag);
	}

  switch(ButtonFlag) {
    case 0:						//Case 0 means No buttons pressed and gate is closed and has been closed for at least one loop
      break;
    case 1:                 //Button 1 Turns ON/OFF Normal Light
      if(digitalRead(OutPort2) == HIGH) {
        digitalWrite(OutPort2,LOW);
				//procRespose("Your Life Lite is now OFF. Text ON to turn on.");
      }
      else{
        digitalWrite(OutPort0,LOW);
        digitalWrite(OutPort1,LOW);
        digitalWrite(OutPort3,LOW);
        digitalWrite(OutPort2,HIGH);
				digitalWrite(OutPort4,HIGH);
				//procRespose("Your Life Lite is now ON. Text OFF to turn off.");
      }
      Serial.println(F("ON/OFF Light activated by Button!"));
      delay(250);
      break;
    case 2:                 //Button 2 Turns ON/OFF Pizza Light
      if(digitalRead(OutPort3) == HIGH) {
        digitalWrite(OutPort3,LOW);
				//procRespose("Your Life Lite is now OFF. Text ON to turn on.");
      }
      else{
        digitalWrite(OutPort0,LOW);
        digitalWrite(OutPort1,LOW);
        digitalWrite(OutPort2,LOW);
        digitalWrite(OutPort3,HIGH);
				digitalWrite(OutPort4,HIGH);
				//procRespose("Your Life Lite is now on PIZZA mode. Text OFF to turn off.");
      }
      Serial.println(F("Pizza Light activated by Button!"));
      delay(250);
      break;
    case 3:                 //Button 3 Turns ON/OFF Strobe Light
      if(digitalRead(OutPort1) == HIGH) {
        digitalWrite(OutPort1,LOW);
				//procRespose("Your Life Lite is now OFF. Text ON to turn on.");
      }
      else{
        digitalWrite(OutPort0,LOW);
        digitalWrite(OutPort2,LOW);
        digitalWrite(OutPort3,LOW);
        digitalWrite(OutPort1,HIGH);
				delay(250);
				digitalWrite(OutPort4,LOW);
				//procRespose("Your Emergency Life Lite is now ON. Text OFF to turn off. This does not alert emergency services. If you have an emergency call 9-1-1.");
      }
      Serial.println(F("Strobe Light activated by Button!"));
      delay(250);
			digitalWrite(OutPort4,HIGH);
      break;
    case 4:							//Button 4 turns off all lights
      digitalWrite(OutPort0,LOW);
      digitalWrite(OutPort1,LOW);
      digitalWrite(OutPort2,LOW);
      digitalWrite(OutPort3,LOW);
			digitalWrite(OutPort4,HIGH);
      Serial.println(F("All Lights Deactivated by Button!"));
			clearBUFFD();
			clearagsmSerial();
			Serial.flush();
			resetFunc(); //call reset
			//procRespose("Your Life Lite is now OFF. Text ON to turn on.");
      delay(250);
      break;
		case 5:							//Case 5 is when the gate has just opened
			//procRespose("Gate Open Detected!");
			digitalWrite(OutPort0,LOW);
			digitalWrite(OutPort2,LOW);
			digitalWrite(OutPort3,LOW);
			digitalWrite(OutPort1,HIGH);
			delay(250);
			digitalWrite(OutPort4,LOW);
			gateStatus = 1;
			break;
		case 6:			//Gate has been just been closed
			gateStatus = 0;
			gateAlertSent = 0;
			break;
    default:
      break;
  }
}

void loop(){
	#if defined(ButtonArray)
  	buttonCheckHandler();
  #endif
	if(!(++globalCount % 10)){			//DEBUG: Communicates thru USB to confirm continuing functionality
		globalCount = 0;
		Serial.println(F("Still Ticking!"));
	}

	//process SMS commands - start
	listSMS();																//find the last used SMS location
	clearagsmSerial();
	int cnt;
	//Serial.print(F("About to Assign cnt to noSMS = "));
	//Serial.println(noSMS);
	cnt = noSMS;
	int validMsgFlag = 0;

	if(gateStatus == 1 && gateAlertSent == 0){
		procRespose(gateAlertTXT);
		Serial.println(gateAlertTXT);
		clearBUFFD();
		gateAlertSent = 1;
	}
	while (cnt>0){
		Serial.print(F("SMS Message Detected! Count = "));
		Serial.println(cnt);
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
			if(strstr(buffd,"pizza") || strstr(buffd,"Pizza") || strstr(buffd,"PIZZA") || strstr(buffd,"D83CDF55")){
				digitalWrite(OutPort0, LOW);
				digitalWrite(OutPort1, LOW);
				digitalWrite(OutPort2, LOW);
				digitalWrite(OutPort3, HIGH);
				digitalWrite(OutPort4, HIGH);
				procRespose(pizzaTXT);
				Serial.println(pizzaTXT);
				clearBUFFD();
				validMsgFlag = 1;
			}
			if(strstr(buffd,"on") || strstr(buffd,"On") || strstr(buffd,"ON")){
				digitalWrite(OutPort0, LOW);
				digitalWrite(OutPort1, LOW);
				digitalWrite(OutPort2, HIGH);
				digitalWrite(OutPort3, LOW);
				digitalWrite(OutPort4, HIGH);
				procRespose(onTXT);
				Serial.println(onTXT);
				clearBUFFD();
				validMsgFlag = 1;
			}
			if(strstr(buffd,"help") || strstr(buffd,"Help") || strstr(buffd,"HELP")){
				digitalWrite(OutPort0, LOW);
				digitalWrite(OutPort1, HIGH);
				digitalWrite(OutPort2, LOW);
				digitalWrite(OutPort3, LOW);
				digitalWrite(OutPort4, HIGH);
				delay(250);
				digitalWrite(OutPort4, LOW);
				delay(250);
				digitalWrite(OutPort4, HIGH);
				procRespose(helpTXT);
				Serial.println(helpTXT);
				clearBUFFD();
				validMsgFlag = 1;
			}
			if(strstr(buffd,"off") || strstr(buffd,"Off") || strstr(buffd,"OFF")){
				digitalWrite(OutPort0, LOW);
				digitalWrite(OutPort1, LOW);
				digitalWrite(OutPort2, LOW);
				digitalWrite(OutPort3, LOW);
				digitalWrite(OutPort4, HIGH);
				procRespose(offTXT);
				Serial.println(offTXT);
				clearBUFFD();
				validMsgFlag = 1;
			}
			if(!validMsgFlag){
				procRespose(supportTXT);
				Serial.println(supportTXT);
				clearBUFFD();
			}
			deleteSMS(cnt);													//free the SMS location
		}
		deleteSMS(1);
		//here process SMS the content - end
		clearBUFFD();
		clearagsmSerial();
		cnt--;
	}
	//process SMS commands - stop
	delay(250);

}
