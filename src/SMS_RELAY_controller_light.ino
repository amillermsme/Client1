#include <Arduino.h>
#include <avr/io.h>
#include <avr/pgmspace.h>
#include <MemoryFree.h>
#include <EEPROM.h>

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
char rootNumber[16] = "+18319053945";      //destination number
const char andrewNumber[16] = "+18319053945";      //Andrew's number

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

#define FS(x) (__FlashStringHelper*)(x)

#define NUMBERENTRY_ROOT 			0
#define NUMBERENTRY_ACTIVE 		16
#define phoneNumAddress(x) 		32+16*x
#define authStatusAddress(x)	32+16*x+13
#define isActiveAddress(x)		32+16*x+14

#define STATUS_ROOT 			1
#define STATUS_UNKNOWN 		2
#define STATUS_RECORDED 	3
#define STATUS_PAIRED 		4
#define STATUS_REGISTERED 5
#define STATUS_AUTH 			6



const char testTXT[] PROGMEM = "This is a test string in FLASH memory.";
const char welcomeTXT[] PROGMEM = "WELCOME TO LIFE LITE\nPlease text back your registration number to pair your phone with your Life Lite.";
const char bootTXT[] PROGMEM = "Your Life Lite has been activated";
const char pairedTXT[] PROGMEM = "LIFE LITE PAIRED\nNice to meet you! Did you know you use your Life Lite just by sending me texts? Try it, text me the word REGISTER.";
const char registeredTXT[] PROGMEM = "Excellent! Complete your emergency medical information at www.lifelite.org/register. When you are done, text NEXT for further instructions.";
const char nextTXT[] PROGMEM = "Good work! Your light will activate with an emergency call on it’s own. You can also use it via text. Text SUPPORT for instructions.";
const char supportTXT[] PROGMEM = "INSTRUCTIONS\nText ON to turn on light\nText HELP to turn on emergency light\nText OFF to turn off light\nText SUPPORT to get these instructions again";
const char onTXT[] PROGMEM = "Your Life Lite is now ON. Text OFF to turn off.";
const char helpTXT[] PROGMEM = "Your Emergency Life Lite is now ON. Text OFF to turn off. This does not alert emergency services. If you have an emergency call 9-1-1.";
const char offTXT[] PROGMEM = "Your Life Lite is now OFF. Text ON to turn on.";
const char pizzaTXT[] PROGMEM = "Your Life Lite is now on PIZZA mode. Text OFF to turn off.";
const char failTXT[] PROGMEM = "Network communication issue. Previous commands may not be executed. This is usually caused by sending too many messages too quickly.";
const char gateAlertTXT[] PROGMEM = "Your Life Lite has been triggered by your pool gate sensor. Text OFF to turn off.";
const char deleteTXT[] PROGMEM = "Phone Number Deleted and Replaced with Root Number.";
const char deleteFailTXT[] PROGMEM = "Root Phone Number Replaced with Andrew's Number.";
const char rootTXT[] PROGMEM = "New Root Phone Number Defined! This is now the root phone for the Life Lite.";
const char signalTXT[] PROGMEM = "Lite Lite's Bad-Signal-Click toggled.";

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
#define FALSE 0
#define TRUE 1

SoftwareSerial agsmSerial(2, 3);   //RX==>2 ,TX soft==>3

char ch;
char buffd[BUFFDSIZE];
char readBuffer[BUFFDSIZE];
char tmpChar[BUFFDSIZE];
int gateStatus = 0, gateAlertSent = 0;
//static char mBuffer[162];
int state=0, i=0, powerState = 0, globalCount = 0;
int appvdNumber = 3;
int nextPhonebookEntry;
int gateEnable = FALSE;
int signalChirp = FALSE;

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

	agsmSerial.begin(9600);					//next, modem setup
	clearagsmSerial();
	clearSerial();
	delay(10);
	memset(tmpChar,0x00,sizeof(tmpChar));

	modemHWSetup();							//configure UNO IN and OUT to be used with modem

	Serial.flush();
	agsmSerial.flush();
	delay(500);
	Serial.flush();
	Serial.print(F("\n"));
	Serial.print(F("\n"));
	Serial.println(F("wait until c-uGSM/d-u3G is ready"));
	Serial.print(F("Arduino version: "));
	Serial.println(ARDUINO);

	powerOnModem();

	clearBUFFD();
	while(strlen(buffd)<1){
		getIMEI();
		delay(500);
	}

	ready4SMS = 0;
	ready4Voice = 0;

	setupMODEMforSMSusage();

	Serial.flush();
	Serial.println(F("modem ready.. let's proceed and blink"));
	Serial.flush();
	retrieveMessageFromFlash(FS(bootTXT),tmpChar,sizeof(tmpChar));
	procRespose(tmpChar,phoneNumber);
	Serial.println(tmpChar);
	Serial.flush();
	Serial.println(F("Finished Sending."));

	//Check Gate state
	if(digitalRead(InPort5) == LOW) gateEnable = TRUE;

	/*Serial.println(F("Attempting to write to EEPROM position 0..."));
	if(writeNumberToEEPROM(phoneNumAddress(NUMBERENTRY_ROOT),andrewNumber,sizeof(andrewNumber)) == 1){
		Serial.println(F("Write to EEPROM position 0 complete."));
	}
	else while(1);

	Serial.println(F("Write to EEPROM position 0 complete.\nNow attempting to Read..."));
	if(retrieveNumberFromEEPROM(phoneNumAddress(NUMBERENTRY_ROOT),tmpChar,sizeof(tmpChar)) == 1){
		Serial.print(F("Read EEPROM position 0 complete: "));
		Serial.println(tmpChar);
	}
	else while(1);

	while(1);*/

	digitalWrite(OutPort2, HIGH);
	delay(200);
	digitalWrite(OutPort2, LOW);
}
//Writes a Phone Number to EEPROM
int writeNumberToEEPROM(int addressEEPROM, char* buffer, size_t buffersize){
	if(!strstr(buffer,"+1")){
		Serial.print("EEPROM phone number write fail! Bad Number: ");
		Serial.println(buffer);
		return 2;
	}
	for(int i = 0; i < 12; i++){
		if((char)buffer[i] == "\0"){
			Serial.print("EEPROM phone number write fail! Too Short: ");
			Serial.println(buffer);
			return 3;
		}
		EEPROM.write(addressEEPROM+i,(int)buffer[i]);
	}
}
//Reads a Phone Number from EEPROM
int retrieveNumberFromEEPROM(int addressEEPROM, char* buffer, size_t buffersize){
	memset(buffer,0x00, buffersize);
	for(int i = 0; i < 12; i++){
		if((char)buffer[i] == "\0"){
			Serial.print("EEPROM phone number retrieve fail! Too Short: ");
			Serial.println(buffer);
			return 3;
		}
		buffer[i] = (char)EEPROM.read(addressEEPROM+i);
	}
	if(!strstr(buffer,"+1")){
		Serial.print("EEPROM phone number retrieve fail! Bad Number: ");
		Serial.println(buffer);
		return 2;
	}
}
// retrieves a message stored in flash memeory.
void retrieveMessageFromFlash(const __FlashStringHelper* message, char* buffer, size_t buffersize){
	const char PROGMEM *p = (const char PROGMEM *)message;
	//Serial.println("Mark 1");
	//Serial.flush();
	size_t messageIndex = 0;
	//Serial.println("Mark 2");
	//Serial.flush();
	memset(buffer,0x00, buffersize);
	//Serial.println("Mark 3");
	//Serial.flush();
	while (1) {
  	char ch = pgm_read_byte(p++);
    if (ch == 0) break;
    buffer[messageIndex] = ch;
		messageIndex++;
  }
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
	char c;
	Serial.print(F("Variables Declared..."));
	clearagsmSerial();
	Serial.print(F("Sent AT+CMGR..."));
	Serial.print(F("Sending SMSindex to Modem = "));
	itoa(SMSindex,&c,10);
	Serial.println(&c);
	Serial.flush();
	memset(tmpChar,0x00, sizeof(tmpChar));
	sprintf(tmpChar,"+CMGR=%i",SMSindex);//format the command
	sendATcommand(tmpChar,"OK","ERROR",3);//send command to modem
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
	if(strstr(pch,"+1")){
		strcpy(incomingNumber, &pch[1]);
		Serial.print(F("Incoming Text Phone Number: "));
		Serial.println(incomingNumber);
	}
	else{
		Serial.print(F("Bad Text Phone Number: "));
		Serial.println(&pch[1]);
		return 8;
	}
	clearBUFFD();
	clearagsmSerial();
	delay(100);

	*phoneNumberSlot = nextPhonebookEntry;
	//Get Authorization Status of phone number
	for(n=2; n<nextPhonebookEntry; n++){
		memset(tmpChar,0x00, sizeof(tmpChar));
		sprintf(tmpChar,"+CPBR=%i",n);//format the command
		sendATcommand(tmpChar,"OK","ERROR",3);//send command to modem
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
	if(*phoneNumberSlot != nextPhonebookEntry){
		Serial.print(F("Recognized Phone Number Found in Phonebook Location "));
		Serial.println(*phoneNumberSlot);
	}
	else{
		Serial.print(F("New Phone Number Saved in Phonebook Location "));
		Serial.println(*phoneNumberSlot);
		memset(tmpChar,0x00,sizeof(tmpChar));
		sprintf(tmpChar,"+CPBW=%i,\"%s\",145,\"UNKNOWN\"\r",phoneNumberSlot,incomingNumber);
		sendATcommand(tmpChar,"OK","ERROR",3);//send command to modem
		nextPhonebookEntry++;
		clearagsmSerial();
	}
	Serial.print(F("Incoming Authorization Status: "));
	Serial.println(authStatus);
	ret = (strstr(authStatus,"ROOT")) ? 1 : 0;						//Root # used. All Good, process command.
	ret = (strstr(authStatus,"UNKNOWN")) ? 2 : ret;				//New Phone number detected
	ret = (strstr(authStatus,"RECORDED")) ? 3 : ret;					//Seen, not yet paired
	ret = (strstr(authStatus,"PAIRED")) ? 4 : ret;				//Paired, not yet registered
	ret = (strstr(authStatus,"REGISTERED")) ? 5 : ret;		//Registered, not yet nexted
	ret = (strstr(authStatus,"AUTH")) ? 6 : ret;					//Registered.

	memset(tmpChar,0x00,sizeof(tmpChar));
	if(ret == 1) sprintf(tmpChar,"+CPBW=1,\"%s\",145,\"ROOT\"\r",incomingNumber);
	if(ret == 4) sprintf(tmpChar,"+CPBW=1,\"%s\",145,\"PAIRED\"\r",incomingNumber);
	if(ret == 5) sprintf(tmpChar,"+CPBW=1,\"%s\",145,\"REGISTERED\"\r",incomingNumber);
	if(ret == 6) sprintf(tmpChar,"+CPBW=1,\"%s\",145,\"AUTH\"\r",incomingNumber);

	sendATcommand(tmpChar,"OK","ERROR",3);//send command to modem
	clearagsmSerial();

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
	ButtonFlag = (digitalRead(InPort5) == HIGH && gateStatus == 0) ? 5 : ButtonFlag;
	ButtonFlag = (digitalRead(InPort5) == LOW && gateStatus == 1) ? 6 : ButtonFlag;
  ButtonFlag = (analogRead(Button2) < 500) ? 1 : ButtonFlag;
  ButtonFlag = (analogRead(Button3) < 500) ? 4 : ButtonFlag;
  ButtonFlag = (analogRead(Button4) < 500) ? 3 : ButtonFlag;

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
      delay(500);
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
      delay(500);
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
      delay(500);
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
			if(gateEnable == TRUE){
				digitalWrite(OutPort0,LOW);
				digitalWrite(OutPort2,LOW);
				digitalWrite(OutPort3,LOW);
				digitalWrite(OutPort1,HIGH);
				delay(250);
				digitalWrite(OutPort4,LOW);
				Serial.println(F("Gate Open Detected!"));
			}
			gateStatus = 1;
			break;
		case 6:			//Gate has been just been closed
			if(gateEnable == FALSE){
				Serial.println(F("Gate Plugged in!"));
				gateAlertSent = 1;
			}
			gateEnable = TRUE;
			if(gateAlertSent){
				gateStatus = 0;
				gateAlertSent = 0;
				Serial.println(F("Gate Close Detected!"));
			}
			break;
    default:
      break;
  }
	return ButtonFlag;
}

void loop(){
	//Serial.println(F("Loop Iteration"));
	#if defined(ButtonArray)
  	buttonCheckHandler();
  #endif
	if(!(++globalCount % 10)){			//DEBUG: Communicates thru USB to confirm continuing functionality
		globalCount = 0;
		Serial.print(F("Still Ticking! Free SRAM = "));
		Serial.println(freeMemory());
		Serial.flush();
		if(getSignalStatus() < 2){
			if(signalChirp){
				digitalWrite(OutPort0,LOW);
				delay(200);
				digitalWrite(OutPort0,HIGH);
				delay(200);
				digitalWrite(OutPort0,LOW);
			}
		}
	}

	//process SMS commands - start
	listSMS();																//find the last used SMS location
	clearagsmSerial();
	//Serial.println(F("SMS Listed."));
	int cnt, senderAuth = 0;
	//Serial.print(F("About to Assign cnt to noSMS = "));
	//Serial.println(noSMS);
	cnt = noSMS;
	int validMsgFlag = 0;
	int phoneNumberSlot;
	char incomingNumber[16];

	if(gateStatus == 1 && gateAlertSent == 0 && gateEnable == TRUE){
		Serial.println(F("Sending Gate Open Notification."));
		retrieveMessageFromFlash(FS(gateAlertTXT),tmpChar,sizeof(tmpChar));
		procRespose(tmpChar,phoneNumber);
		Serial.println(tmpChar);
		Serial.flush();
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

		memset(tmpChar,0x00, sizeof(tmpChar));
		//here process SMS the content - start
		if(strlen(buffd) > 0 || senderAuth == 8){		//non empty SMS
			if(senderAuth == 1 || senderAuth == 4 || senderAuth == 5 || senderAuth == 6){		//Valid Authorization codes
				strcpy(phoneNumber,incomingNumber);
				if(strstr(buffd,"pizza") || strstr(buffd,"Pizza") || strstr(buffd,"PIZZA") || strstr(buffd,"D83CDF55")){
					digitalWrite(OutPort0, LOW);
					digitalWrite(OutPort1, LOW);
					digitalWrite(OutPort2, LOW);
					digitalWrite(OutPort3, HIGH);
					digitalWrite(OutPort4, HIGH);
					retrieveMessageFromFlash(FS(pizzaTXT),tmpChar,sizeof(tmpChar));
					validMsgFlag = 1;
				}
				else if(strstr(buffd,"on") || strstr(buffd,"On") || strstr(buffd,"ON")){
					digitalWrite(OutPort0, LOW);
					digitalWrite(OutPort1, LOW);
					digitalWrite(OutPort2, HIGH);
					digitalWrite(OutPort3, LOW);
					digitalWrite(OutPort4, HIGH);
					retrieveMessageFromFlash(FS(onTXT),tmpChar,sizeof(tmpChar));
					validMsgFlag = 1;
				}
				else if(strstr(buffd,"help") || strstr(buffd,"Help") || strstr(buffd,"HELP")){
					digitalWrite(OutPort0, LOW);
					digitalWrite(OutPort1, HIGH);
					digitalWrite(OutPort2, LOW);
					digitalWrite(OutPort3, LOW);
					digitalWrite(OutPort4, HIGH);
					delay(250);
					digitalWrite(OutPort4, LOW);
					delay(250);
					digitalWrite(OutPort4, HIGH);
					retrieveMessageFromFlash(FS(helpTXT),tmpChar,sizeof(tmpChar));
					validMsgFlag = 1;
				}
				else if(strstr(buffd,"off") || strstr(buffd,"Off") || strstr(buffd,"OFF")){
					digitalWrite(OutPort0, LOW);
					digitalWrite(OutPort1, LOW);
					digitalWrite(OutPort2, LOW);
					digitalWrite(OutPort3, LOW);
					digitalWrite(OutPort4, HIGH);
					retrieveMessageFromFlash(FS(offTXT),tmpChar,sizeof(tmpChar));
					validMsgFlag = 1;
				}
				else if(strstr(buffd,"delete") || strstr(buffd,"Delete") || strstr(buffd,"DELETE")){
					Serial.print(F("Delete Request Received: "));
					Serial.println(buffd);
					if(phoneNumberSlot != 2){
						strcpy(phoneNumber,rootNumber);
						memset(tmpChar,0x00,sizeof(tmpChar));
						sprintf(tmpChar,"+CPBW=%i",phoneNumberSlot);
						sendATcommand(tmpChar,"OK","ERROR",3);//send command to modem
						clearagsmSerial();
						memset(tmpChar,0x00,sizeof(tmpChar));
						sprintf(tmpChar,"+CPBW=1,\"%s\",145,\"ROOT\"\r",rootNumber);
						sendATcommand(tmpChar,"OK","ERROR",3);//send command to modem
						clearagsmSerial();
						retrieveMessageFromFlash(FS(deleteTXT),tmpChar,sizeof(tmpChar));
					}
					else{
						strcpy(rootNumber,andrewNumber);
						strcpy(phoneNumber,andrewNumber);
						memset(tmpChar,0x00,sizeof(tmpChar));
						sprintf(tmpChar,"+CPBW=2,\"%s\",145,\"ROOT\"\r",andrewNumber);
						sendATcommand(tmpChar,"OK","ERROR",3);//send command to modem
						clearagsmSerial();
						memset(tmpChar,0x00,sizeof(tmpChar));
						sprintf(tmpChar,"+CPBW=1,\"%s\",145,\"ROOT\"\r",andrewNumber);
						sendATcommand(tmpChar,"OK","ERROR",3);//send command to modem
						clearagsmSerial();
						retrieveMessageFromFlash(FS(deleteFailTXT),tmpChar,sizeof(tmpChar));
					}
					validMsgFlag = 1;
				}
				else if(strstr(buffd,"root") || strstr(buffd,"Root") || strstr(buffd,"ROOT")){
					Serial.print(F("Root Request Received: "));
					Serial.println(buffd);
					memset(tmpChar,0x00,sizeof(tmpChar));
					sprintf(tmpChar,"+CPBW=2,\"%s\",145,\"ROOT\"\r",phoneNumber);
					sendATcommand(tmpChar,"OK","ERROR",3);//send command to modem
					clearagsmSerial();
					memset(tmpChar,0x00,sizeof(tmpChar));
					sprintf(tmpChar,"+CPBW=%i,\"%s\",145,\"AUTH\"\r",phoneNumberSlot,phoneNumber);
					sendATcommand(tmpChar,"OK","ERROR",3);//send command to modem
					clearagsmSerial();
					retrieveMessageFromFlash(FS(rootTXT),tmpChar,sizeof(tmpChar));
					validMsgFlag = 1;
				}
				else if(strstr(buffd,"signal") || strstr(buffd,"Signal") || strstr(buffd,"SIGNAL"))
				{
					signalChirp = (signalChirp == 0) ? 1 : 0;
					Serial.print(F("Signal Request Received: "));
					Serial.println(buffd);
					retrieveMessageFromFlash(FS(signalTXT),tmpChar,sizeof(tmpChar));
					validMsgFlag = 1;
				}
			}
			if(senderAuth == 2 && validMsgFlag == 0){			//Entirely new Phone Number detected
				memset(tmpChar,0x00,sizeof(tmpChar));
				sprintf(tmpChar,"+CPBW=%i,\"%s\",145,\"RECORDED\"\r",phoneNumberSlot,incomingNumber);
				sendATcommand(tmpChar,"OK","ERROR",3);//send command to modem
				clearagsmSerial();
				retrieveMessageFromFlash(FS(welcomeTXT),tmpChar,sizeof(tmpChar));
				validMsgFlag = 1;
				}
			if(senderAuth == 3 && validMsgFlag == 0){			//RECORDED but Unpaired Phone Number detected
				Serial.print(F("Recorded Number detected: "));
		    Serial.println(incomingNumber);
				if(strstr(buffd,"1234")){
					Serial.print(F("Correct Passcode Detected: "));
					Serial.println(buffd);
					strcpy(phoneNumber,incomingNumber);
					memset(tmpChar,0x00,sizeof(tmpChar));
					sprintf(tmpChar,"+CPBW=%i,\"%s\",145,\"PAIRED\"\r",phoneNumberSlot,phoneNumber);
					sendATcommand(tmpChar,"OK","ERROR",3);//send command to modem
					clearagsmSerial();
					memset(tmpChar,0x00,sizeof(tmpChar));
					sprintf(tmpChar,"+CPBW=1,\"%s\",145,\"PAIRED\"\r",phoneNumber);
					sendATcommand(tmpChar,"OK","ERROR",3);//send command to modem
					clearagsmSerial();
					retrieveMessageFromFlash(FS(pairedTXT),tmpChar,sizeof(tmpChar));
					validMsgFlag = 1;
				}
				else{
					retrieveMessageFromFlash(FS(welcomeTXT),tmpChar,sizeof(tmpChar));
					validMsgFlag = 1;
				}
			}
			if(senderAuth == 4 && validMsgFlag == 0){			//Paired Number detected
				Serial.print(F("Paired Number Detected: "));
		    Serial.println(phoneNumber);
				if(strstr(buffd,"REGISTER") || strstr(buffd,"Register") || strstr(buffd,"register")){
					Serial.print(F("Registration Requested: "));
					Serial.println(buffd);
					memset(tmpChar,0x00,sizeof(tmpChar));
					sprintf(tmpChar,"+CPBW=%i,\"%s\",145,\"REGISTERED\"\r",phoneNumberSlot,phoneNumber);
					sendATcommand(tmpChar,"OK","ERROR",3);//send command to modem
					clearagsmSerial();
					retrieveMessageFromFlash(FS(registeredTXT),tmpChar,sizeof(tmpChar));
					validMsgFlag = 1;
				}
				else{
					retrieveMessageFromFlash(FS(pairedTXT),tmpChar,sizeof(tmpChar));
					validMsgFlag = 1;
				}
			}
			if(senderAuth == 5 && validMsgFlag == 0){			//Registered Number detected
				Serial.print(F("Registered Number Detected: "));
		    Serial.println(incomingNumber);
				if(strstr(buffd,"NEXT") || strstr(buffd,"Next") || strstr(buffd,"next")){
					memset(tmpChar,0x00,sizeof(tmpChar));
					sprintf(tmpChar,"+CPBW=%i,\"%s\",145,\"AUTH\"\r",phoneNumberSlot,phoneNumber);
					sendATcommand(tmpChar,"OK","ERROR",3);//send command to modem
					clearagsmSerial();
					retrieveMessageFromFlash(FS(nextTXT),tmpChar,sizeof(tmpChar));
					validMsgFlag = 1;
				}
				else{
					retrieveMessageFromFlash(FS(registeredTXT),tmpChar,sizeof(tmpChar));
					validMsgFlag = 1;
				}
			}
			if(senderAuth == 8 && validMsgFlag == 0){			//Bad SMS Content
				Serial.println(F("SMS Fuckup. Dumping ALL Messages."));
				retrieveMessageFromFlash(FS(failTXT),tmpChar,sizeof(tmpChar));
				validMsgFlag = 1;
				deleteSMS(0);
			}
			if(!validMsgFlag) retrieveMessageFromFlash(FS(supportTXT),tmpChar,sizeof(tmpChar));
			procRespose(tmpChar,incomingNumber);
			//Serial.println(tmpChar);
			Serial.flush();
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
