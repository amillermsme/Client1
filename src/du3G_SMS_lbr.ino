#include <Arduino.h>

/*
du3G_SMS_lbr.ino v 0.97/20151118 - d-u3G 1.13 LIBRARY SUPPORT
COPYRIGHT (c) 2015 Dragos Iosub / R&D Software Solutions srl

You are legaly entitled to use this SOFTWARE ONLY IN CONJUNCTION WITH d-u3G DEVICES USAGE. Modifications, derivates and redistribution
of this software must include unmodified this COPYRIGHT NOTICE. You can redistribute this SOFTWARE and/or modify it under the terms
of this COPYRIGHT NOTICE. Any other usage may be permited only after written notice of Dragos Iosub / R&D Software Solutions srl.

This SOFTWARE is distributed is provide "AS IS" in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied
warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

Dragos Iosub, Bucharest 2015.
http://itbrainpower.net
***************************************************************************************
SOFTWARE:
This file MUST be present, toghether with other files, inside a folder named
like your main sketch!
***************************************************************************************
HARDWARE:
Read the readme file(s) inside the arhive/folder.
***************************************************************************************
*/
//#define SMSLibDebug
/*
	send "message" to "phno".
	Returns 1 for success, 0 for failure
	phtype:
		129 (default)
		145
		161
*/
int sendSMS(char* phno, char* message, char* phtype){
	if(ready4SMS != 1)
		setupMODEMforSMSusage();
	int res=0;
	clearBUFFD();
	sprintf(buffd,"AT+CMGS=\"%s\",%s\r", phno, phtype);
	agsmSerial.print(buffd);
	res = recUARTDATA(">","ERROR",12);
	if(res==1) {
		clearBUFFD();
		sprintf(message,"%s%c",message,0x1A);
		aGsmCMD(message,2);
		res = recUARTDATA("OK","ERROR",30);
		delay(150);
		#if defined(SMSLibDebug)
			if(res==1 && strstr(buffd,"+CMGS:")) {
				Serial.println(F("SMS success"));
			}else{
				Serial.println(F("SMS error"));
			}
			Serial.flush();
			delay(150);
		#endif
		/*
		if(res==1 && strstr(buffd,"+CMGS:")) Serial.println(F("SMS succeed"));
		else Serial.println(F("SMS error"));
		Serial.flush();
		*/
		delay(150);
	}
	else{
		#if defined(SMSLibDebug)
			Serial.println(F("SMS rejected"));
			Serial.flush();
		#endif
	}
 	return res;
}

/*
	read SMS at SMSindex location
	SMS content loaded in buffd
	you can check...if strlen(buffd)<1 => no SMS at that location
	SMSindex between 1 and noSMS (last used SMS location)
*/
void readSMS(int SMSindex){
	clearBUFFD();
	if(ready4SMS != 1) setupMODEMforSMSusage();
	if(totSMS<1) listSMS();
	if(SMSindex > noSMS || SMSindex < 1) return;
	int cnt=0;
	int j=0;
	int run=1;
	char c;
	unsigned long startTime = 0;
	clearagsmSerial();
	agsmSerial.print("AT+CMGR=");//send command to modem
	agsmSerial.println(SMSindex);//send command to d-u3G modem
	//agsmSerial.print(SMSindex);//send command to d-uGSM modem
	//agsmSerial.println(",0");//send command to c-uGSM modem
	delay(1);
	readline();//just 2 remove modem cmd echo
	readline();//just 2 remove second line ==> (containing OK / +CMS ERROR / SMS header)
	if(strlen(buffd)<20)
		if(strstr(buffd,"OK")||strstr(buffd,"ERROR:")) {//check empty message location or beyond interval SMS location
			clearBUFFD();
			clearagsmSerial();
			#if defined(atDebug)
				Serial.println(F("noSMS at this location"));
			#endif
			return;
		}
	startTime = millis();
	while(run){//here
		if(TXavailable()){
			c = agsmSerial.read();
			if(c == '\r') {//look for CR char(end of SMS message)
				buffd[cnt]=0x00;
				break;
			}
			if(cnt < BUFFDSIZE-1) {
				buffd[cnt] = c;
			} else {//buffer overflow here (avoided)
				break;
			}
			cnt++;
		}
		else if(millis() - startTime > (unsigned long) 2000) //unblocking procedure
			  break;
	}
	buffd[cnt]=0x00;//add string terminator
	clearagsmSerial();//clear remaining serial chars
}

/*just print all SMSs to the debug console...*/
void readAllSMS(){
	listSMS();
	clearagsmSerial();
	int cnt;
	cnt = noSMS;
	while (cnt>0){
		Serial.print(F("SMS "));//noSMS
		Serial.println(cnt);
		readSMS(cnt);
		Serial.println(buffd);delay(50);
		clearBUFFD();
		clearagsmSerial();
		cnt--;
	}
}

/*
	delete SMS at SMSindex location...
	to make some space, delete from bigges to smalles(desired) SMSindex
	you can try something like:

	listSMS();//update noSMS(last used SMS location)
	int cnt;
	cnt = noSMS;
	//while (cnt > 0){//to delete all
	while (cnt > DesiredIndex){//to delete until DesiredIndex
		deleteSMS(cnt--);
	}
*/
void deleteSMS(int SMSindex){
	if(ready4SMS != 1)
		setupMODEMforSMSusage();

	char tmpChar[20];//40
	memset(tmpChar,0x00, sizeof(tmpChar));
	clearBUFFD();
	if(!SMSindex){
		Serial.println(F("Deleting ALL SMS!"));
		sprintf(tmpChar,"+CMGD=%i,4\r",1);//format the delete command
	}
	else{
		Serial.print(F("Deleting SMS is Mem Pos #"));
		Serial.println(SMSindex);
		sprintf(tmpChar,"+CMGD=%i\r",SMSindex);//format the delete command
	}
	//sprintf(tmpChar,"+CMGD=%i,4\r",SMSindex);//format the delete command EDIT: delete ALL messages!
	//Serial.println(tmpChar);
	sendATcommand(tmpChar,"OK","ERROR",3);//send command to modem
}

/*
	read SMS storage capacity,
	total SMS locations(capacity) ==> noSMS(global var),
	last used SMS location ==> noSMS(global var)
*/
int listSMS(){
	if(ready4SMS != 1)
		setupMODEMforSMSusage();
	int res=0;
	int j=0;
	char * pch;
	while(res!=1){
		//fATcmd(F("+CMGL=\"REC UNREAD\",1"),10, "OK","+CMS ERROR:");
		res = fATcmd(F("+CPMS?"),10, "OK","+CMS ERROR:");//+CPMS: "SM",8,50,"SM",8,50,"SM",8,50// +CMS ERROR:
		if(j++ > 3){
			Serial.println(F("List SMS timeout! Shitty Signal? Rebooting Modem..."));
			restartMODEM();
		}
	}

	j=0;
	pch = strtok (buffd,",");
	while (pch != NULL){//parsing for SMS SIMM entries
		if(j==1){// used SMS
			noSMS=atoi(pch);
		}else if(j==2){//total SMS
			totSMS=atoi(pch);
		}
		pch = strtok (NULL, ",");
		j++;
	}
	return res;
}
