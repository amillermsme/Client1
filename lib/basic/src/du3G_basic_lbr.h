/*
du3G_basic_lbr.ino v 0.9711/20160707 - d-u3G 1.13 LIBRARY SUPPORT
COPYRIGHT (c) 2015 Dragos Iosub / R&D Software Solutions srl

You are legaly entitled to use this SOFTWARE ONLY IN CONJUNCTION WITH d-u3G DEVICES USAGE. Modifications, derivates and redistribution
of this software must include unmodified this COPYRIGHT NOTICE. You can redistribute this SOFTWARE and/or modify it under the terms
of this COPYRIGHT NOTICE. Any other usage may be permited only after written notice of Dragos Iosub / R&D Software Solutions srl.

This SOFTWARE is distributed is provide "AS IS" in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied
warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

Dragos Iosub, Bucharest 2016.
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

#define agsm_BASIC

#if (ARDUINO >= 100)
  #include "Arduino.h"
	#if !defined(HARDWARESERIAL)
		#include <SoftwareSerial.h>
	#endif
#else
  #include "WProgram.h"
	#if !defined(HARDWARESERIAL)
		#include <NewSoftSerial.h>
	#endif
#endif


#define AGSM_DEFAULT_TIMEOUT 1500
#define AGSM_DEFAULT_TIMEOUT_NO 0

#define AGSM_DEFAULT_AT_OK_MSG "OK"
#define AGSM_DEFAULT_AT_ERR_MSG "ERROR"
#define AGSM_DEFAULT_AT_TIMEOUT 3

#define AGSM_DEFAULT_MIC_GAIN 14

#define GSM 1
#define GPRS 0

int activeSIM =0;
/*definition and how to in ags_basic.ino file*/
int getSignalStatus(void);
void clearBUFFD(void);
size_t aGsmCMD(char* cmd, int Delay_mS);
size_t aGsmWRITE(char* str);
inline char aGsmREAD(void);
int fATcmd(const __FlashStringHelper* cmd, int to = AGSM_DEFAULT_AT_TIMEOUT, char* ok = AGSM_DEFAULT_AT_OK_MSG, char* = AGSM_DEFAULT_AT_ERR_MSG);
int sendATcommand(char* outstr, char* ok = AGSM_DEFAULT_AT_OK_MSG, char* = AGSM_DEFAULT_AT_ERR_MSG, int to = AGSM_DEFAULT_AT_TIMEOUT);
int recUARTDATA(char* ok = AGSM_DEFAULT_AT_OK_MSG, char* = AGSM_DEFAULT_AT_ERR_MSG, int to = AGSM_DEFAULT_AT_TIMEOUT);
bool TXavailable(void);
int parseResponce(char* ok, char* head, char* retChar, char* separator , int index);
void getVoltage(void);
void getIMEI(void);
void getIMSI(void);
int hangup(void);
int answer(void);
int dial(char* destinationNO);
int printRegistration(int cmdValue);
int registration(int type);
void setupMODEMforSMSusage(void);

//void shutdownMODEM();
void resetMODEM(void);
void restartMODEM(void);
void clearagsmSerial(void);
void clearSerial(void);

//void setActiveSIM(int SIM);
//void activateTopSIM(void){setActiveSIM(0);}//call this function if you want to test/use SIM inserted in the SIM SOCKET placed on TOP of a-gsm
//void activateBottomSIM(void){setActiveSIM(1);};//call this function if you want to test/use SIM inserted in the SIM SOCKET placed on BOTTOM of a-gsm

//void setupMODEMforDTMFSusage(void);
//void setupMODEMforDTMFRusage(void);

void disableAutoanswer(void);
void enableAutoanswer(int ringNo = 2);

int getcallStatus(void);

int enableagsmClockUpdate(void);
int disableagsmClockUpdate(void);

int getagsmClock(char* clock);
int setagsmClock(char* clock);

void emergencyReset(void);

int getModemState(void);
void powerOffModem(void);
void powerOnModem(void);
void modemHWSetup(void);

int readline(unsigned long timeout = AGSM_DEFAULT_TIMEOUT_NO);
void(* resetFunc) (void) = 0;//declare reset function at address 0
