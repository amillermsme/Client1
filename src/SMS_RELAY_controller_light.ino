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


//static char nunderstand[95] = "BAD COMMAND! digital0,digital1,rel0,rel1,rel2,rel3 >> 1 ->activate relay, 0 ->release relay";
//const __FlashStringHelper *welcomeTXT;
const char welcomeTXT[] PROGMEM = "WELCOME TO LIFE LITE\nPlease text back your registration number to pair your phone with your Life Lite.";
const char bootTXT[] PROGMEM = "Your Life Lite has been activated";
/*static char welcomeTXT[106] = "WELCOME TO LIFE LITE\nPlease text back your registration number to pair your phone with your Life Lite.";
static char pairedTXT[135] = "LIFE LITE PAIRED\nNice to meet you! Did you know you use your Life Lite just by sending me texts? Try it, text me the word REGISTER.";
static char registeredTXT[142] = "Excellent! Complete your emergency medical information at www.lifelite.org/register. When you are done, text NEXT for further instructions.";
static char nextTXT[135] = "Good work! Your light will activate with an emergency call on it’s own. You can also use it via text. Text SUPPORT for instructions.";
static char supportTXT[153] = "INSTRUCTIONS\nText ON to turn on light\nText HELP to turn on emergency light\nText OFF to turn off light\nText SUPPORT to get these instructions again";
static char onTXT[50] = "Your Life Lite is now ON. Text OFF to turn off.";
static char helpTXT[137] = "Your Emergency Life Lite is now ON. Text OFF to turn off. This does not alert emergency services. If you have an emergency call 9-1-1.";
static char offTXT[50] = "Your Life Lite is now OFF. Text ON to turn on.";
static char pizzaTXT[61] = "Your Life Lite is now on PIZZA mode. Text OFF to turn off.";
static char executed[40] = " cmd executed";
static char bootTXT[37] = "Your Life Lite has been activated!";
static char gateAlertTXT[84] = "Your Life Lite has been triggered by your pool gate sensor. Text OFF to turn off.";
static char deleteTXT[10] = "Phone Number Deleted.";
static char deleteFailTXT[14] = "Cannot Delete Root Phone Number";*/

//static char welcomeTXT[11] = "welcomeTXT";
static char pairedTXT[10] = "pairedTXT";
static char registeredTXT[14] = "registeredTXT";
static char nextTXT[8] = "nextTXT";
static char supportTXT[11] = "supportTXT";
static char onTXT[6] = "onTXT";
static char helpTXT[8] = "helpTXT";
static char offTXT[7] = "offTXT";
static char pizzaTXT[9] = "pizzaTXT";
//static char bootTXT[5] = "bootTXT";
static char gateAlertTXT[13] = "gateAlertTXT";
static char deleteTXT[10] = "deleteTXT";
static char deleteFailTXT[14] = "deleteFailTXT";

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
#define BUFFDSIZE 200              //used by modem library

SoftwareSerial agsmSerial(2, 3);   //RX==>2 ,TX soft==>3

char ch;
char buffd[BUFFDSIZE];
char readBuffer[200];
int gateStatus = 0, gateAlertSent = 0;
//static char mBuffer[162];
int state=0, i=0, powerState = 0, globalCount = 0;
int appvdNumber = 3;
int nextPhonebookEntry;

//unsigned long startTime = 0;

void procRespose(char* msg, char* outgoingNumber){
	memset(readBuffer,0x00,sizeof(readBuffer));
	sprintf(readBuffer, "%s", msg);
	Serial.print(F("Sending SMS Content:  "));
	Serial.println(readBuffer);
	Serial.println(outgoingNumber);
	Serial.flush();
	delay(150);
	sendSMS(outgoingNumber, readBuffer);			//confirm command
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

	//welcomeTXT = F("WELCOME TO LIFE LITE\nPlease text back your registration number to pair your phone with your Life Lite.");

	agsmSerial.begin(9600);					//next, modem setup
	clearagsmSerial();
	clearSerial();
	delay(10);
	char tmpChar[BUFFDSIZE];

	modemHWSetup();							//configure UNO IN and OUT to be used with modem

	Serial.flush();
	agsmSerial.flush();
	delay(500);

	Serial.println(F("wait until c-uGSM/d-u3G is ready"));
	Serial.println(F("wait until c-uGSM/d-u3G is ready"));
	Serial.println(F("wait until c-uGSM/d-u3G is ready"));
	Serial.print(F("Arduino version: "));
	Serial.println(ARDUINO);
	//Serial.println(welcomeTXT);
  Serial.print(F("Default Phone Number:   "));
	Serial.println(phoneNumber);
	delay(100);
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

	strcpy_P(tmpChar, (char*)pgm_read_byte(bootTXT));
	Serial.print(F("About to send: "));
	Serial.println(tmpChar);
	procRespose(tmpChar,phoneNumber);
	Serial.print(F("Finished Sending: "));
	Serial.println(tmpChar);
	memset(tmpChar,0x00, sizeof(tmpChar));
	//procRespose(bootTXT,phoneNumber);
	//Blink the light to indicate bootTXT
	digitalWrite(OutPort2, HIGH);
	delay(200);
	digitalWrite(OutPort2, LOW);

}

int checkSMSAuthNo(int SMSindex, char *incomingNumber, int *phoneNumberSlot){
	int ret = 0, n = 0;
	char authStatus[16] = "UNKNOWN";
	char *pch;
	Serial.print(F("Entered checkSMSAuthNo..."));
  clearBUFFD();
	if(ready4SMS != 1) setupMODEMforSMSusage();
	if(totSMS<1) listSMS();
	if(SMSindex > noSMS || SMSindex < 1) return -1;
	char tmpChar[BUFFDSIZE];
	memset(tmpChar,0x00, sizeof(tmpChar));
	char c;
	Serial.print(F("Variables Declared..."));
	clearagsmSerial();
	Serial.print(F("Sent AT+CMGR..."));
	Serial.print(F("Sending SMSindex to Modem = "));
	itoa(SMSindex,&c,10);
	Serial.println(&c);
	Serial.flush();
	sprintf(tmpChar,"+CMGR=%i",SMSindex);//format the command
	sendATcommand(tmpChar,"OK","ERROR",3);//send command to modem
	memset(tmpChar,0x00, sizeof(tmpChar));
	delay(20);
	Serial.print("Money Line-->\t");
	pch = strtok(buffd,"\n");
	pch = strtok (NULL, "\n");
	Serial.println(pch);
	if(!strstr(pch,"CMGR")){
		clearBUFFD();
		clearagsmSerial();
		return 8;
	}
	//Extract Phone Number
	pch = strtok(pch,",");
	pch = strtok (NULL, ",");
	pch[13] = NULL;
	strcpy(incomingNumber, &pch[1]);
	clearBUFFD();
	clearagsmSerial();
	Serial.print(F("Incoming Text Phone Number: "));
	Serial.println(incomingNumber);
	delay(100);

	//Get Authorization Status of phone number
	for(n=1; n<nextPhonebookEntry; n++){
		sprintf(tmpChar,"+CPBR=%i",n);//format the command
		sendATcommand(tmpChar,"OK","ERROR",3);//send command to modem
		memset(tmpChar,0x00, sizeof(tmpChar));
		pch = strtok(buffd,"\n");
		pch = strtok (NULL, "\n");
		pch = strtok(pch,",");
		pch = strtok(NULL, ",");
		if(strstr(pch,incomingNumber)){
			pch = strtok(NULL, ",");
			pch = strtok(NULL, ",");
			strcpy(authStatus, pch);
			*phoneNumberSlot = n;
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
	ret = (strstr(authStatus,"REGISTERED")) ? 5 : ret;		//Registered, not yet nexted
	ret = (strstr(authStatus,"AUTH")) ? 6 : ret;			//Registered.

	clearBUFFD();
	clearagsmSerial();
	return ret;
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
      }
      else{
        digitalWrite(OutPort0,LOW);
        digitalWrite(OutPort1,LOW);
        digitalWrite(OutPort3,LOW);
        digitalWrite(OutPort2,HIGH);
				digitalWrite(OutPort4,HIGH);
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
				digitalWrite(OutPort4,HIGH);
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
				delay(250);
				digitalWrite(OutPort4,LOW);
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
      Serial.println(F("Life Lite Reboot by Button!"));
			clearBUFFD();
			clearagsmSerial();
			Serial.flush();
			//Blink the light to indicate boot
			digitalWrite(OutPort3, HIGH);
			delay(200);
			digitalWrite(OutPort3, LOW);
			resetFunc(); //call reset
      delay(250);
      break;
		case 5:							//Case 5 is when the gate has just opened
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
	return ButtonFlag;
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
	int cnt, senderAuth = 1;
	//Serial.print(F("About to Assign cnt to noSMS = "));
	//Serial.println(noSMS);
	cnt = noSMS;
	int validMsgFlag = 0;
	int phoneNumberSlot;
	char incomingNumber[16];
	char tmpChar[BUFFDSIZE];
	memset(tmpChar,0x00, sizeof(tmpChar));

	if(gateStatus == 1 && gateAlertSent == 0){
		procRespose(gateAlertTXT,phoneNumber);
		Serial.println(gateAlertTXT);
		clearBUFFD();
		gateAlertSent = 1;
	}
	while (cnt>0){
		Serial.print(F("SMS Message Detected! Count = "));
		Serial.println(cnt);
		senderAuth = checkSMSAuthNo(cnt, incomingNumber, &phoneNumberSlot);
		readSMS(cnt);														//the SMS content will be returned in buffd
		Serial.print(F("SMS content: "));Serial.flush();delay(50);
		Serial.println(buffd);Serial.flush();delay(50);
		delay(50);
		clearagsmSerial();
		delay(50);

		//here process SMS the content - start
		if(strlen(buffd) > 0 || senderAuth == 8){												//non empty SMS
			if (senderAuth == 1 || senderAuth == 6){		//Valid Authorization codes
				if(strstr(buffd,"pizza") || strstr(buffd,"Pizza") || strstr(buffd,"PIZZA") || strstr(buffd,"D83CDF55")){
					digitalWrite(OutPort0, LOW);
					digitalWrite(OutPort1, LOW);
					digitalWrite(OutPort2, LOW);
					digitalWrite(OutPort3, HIGH);
					digitalWrite(OutPort4, HIGH);
					procRespose(pizzaTXT,phoneNumber);
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
					procRespose(onTXT,phoneNumber);
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
					procRespose(helpTXT,phoneNumber);
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
					procRespose(offTXT,phoneNumber);
					Serial.println(offTXT);
					clearBUFFD();
					validMsgFlag = 1;
				}
				if(strstr(buffd,"delete") || strstr(buffd,"Delete") || strstr(buffd,"DELETE")){
					Serial.print(F("Delete Request Received: "));
					Serial.println(buffd);
					if(phoneNumberSlot != 1){
						sprintf(tmpChar,"AT+CPBW=%i",phoneNumberSlot);
						sendATcommand(tmpChar,"OK","ERROR",3);//send command to modem
						memset(tmpChar,0x00, sizeof(tmpChar));
						//agsmSerial.print(buffd);
						delay(200);
						clearagsmSerial();
						clearBUFFD();
						procRespose(deleteTXT,incomingNumber);
						Serial.println(deleteTXT);
					}
					else{
						clearagsmSerial();
						clearBUFFD();
						procRespose(deleteFailTXT,incomingNumber);
						Serial.println(deleteFailTXT);
					}
					validMsgFlag = 1;
				}
				if(!validMsgFlag){
					procRespose(supportTXT,phoneNumber);
					Serial.println(supportTXT);
					clearBUFFD();
				}
			}
			if (senderAuth == 2){			//Entirely new Phone Number detected
				Serial.print(F("New Phone Number detected: "));
		    Serial.println(incomingNumber);
				sprintf(tmpChar,"AT+CPBW=%i,\"%s\",145,\"KNOWN\"\r",nextPhonebookEntry,incomingNumber);
				sendATcommand(tmpChar,"OK","ERROR",3);//send command to modem
				memset(tmpChar,0x00, sizeof(tmpChar));
				//agsmSerial.print(buffd);
				delay(200);
				clearagsmSerial();
				clearBUFFD();
				strcpy_P(tmpChar, (char*)pgm_read_word(welcomeTXT));
				procRespose(tmpChar,incomingNumber);
				Serial.println(tmpChar);
				memset(tmpChar,0x00, sizeof(tmpChar));
			}
			if (senderAuth == 3){			//Known but Unpaired Phone Number detected
				Serial.print(F("Known Number detected: "));
		    Serial.println(incomingNumber);
				if(strstr(buffd,"1234")){
					Serial.print(F("Correct Passcode Detected: "));
					Serial.println(buffd);
					sprintf(tmpChar,"+CPBW=%i,\"%s\",145,\"PAIRED\"\r",phoneNumberSlot,incomingNumber);
					sendATcommand(tmpChar,"OK","ERROR",3);//send command to modem
					memset(tmpChar,0x00, sizeof(tmpChar));
					//agsmSerial.print(buffd);
					delay(200);
					clearagsmSerial();
					clearBUFFD();
					strcpy(phoneNumber,incomingNumber);
					procRespose(pairedTXT,phoneNumber);
					Serial.println(pairedTXT);
				}
				else{
					delay(200);
					clearagsmSerial();
					clearBUFFD();
					strcpy_P(tmpChar, (char*)pgm_read_word(welcomeTXT));
					procRespose(tmpChar,incomingNumber);
					Serial.println(tmpChar);
					memset(tmpChar,0x00, sizeof(tmpChar));
				}
			}
			if(senderAuth == 4){			//Paired Number detected
				Serial.print(F("Paired Number Detected: "));
		    Serial.println(incomingNumber);
				if(strstr(buffd,"REGISTER") || strstr(buffd,"Register") || strstr(buffd,"register")){
					Serial.print(F("Registration Requested: "));
					Serial.println(buffd);
					sprintf(tmpChar,"AT+CPBW=%i,\"%s\",145,\"REGISTERED\"\r",phoneNumberSlot,incomingNumber);
					sendATcommand(tmpChar,"OK","ERROR",3);//send command to modem
					memset(tmpChar,0x00, sizeof(tmpChar));
					//agsmSerial.print(buffd);
					delay(200);
					clearagsmSerial();
					clearBUFFD();
					procRespose(registeredTXT,phoneNumber);
					Serial.println(registeredTXT);
				}
				else{
					delay(200);
					clearagsmSerial();
					clearBUFFD();
					procRespose(pairedTXT,phoneNumber);
					Serial.println(pairedTXT);
				}
			}
			if(senderAuth == 5){			//Registered Number detected
				Serial.print(F("Registered Number Detected: "));
		    Serial.println(incomingNumber);
				if(strstr(buffd,"NEXT") || strstr(buffd,"Next") || strstr(buffd,"next")){
					sprintf(tmpChar,"AT+CPBW=%i,\"%s\",145,\"AUTH\"\r",phoneNumberSlot,incomingNumber);
					sendATcommand(tmpChar,"OK","ERROR",3);//send command to modem
					memset(tmpChar,0x00, sizeof(tmpChar));
					//agsmSerial.print(buffd);
					delay(200);
					clearagsmSerial();
					clearBUFFD();
					procRespose(nextTXT,phoneNumber);
					Serial.println(nextTXT);
				}
				else{
					delay(200);
					clearagsmSerial();
					clearBUFFD();
					procRespose(registeredTXT,phoneNumber);
					Serial.println(registeredTXT);
				}
			}
			if(senderAuth ==8){				//Bad SMS Content
				Serial.println(F("SMS Fuckup. Dumping ALL Messages."));
				deleteSMS(0);
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
