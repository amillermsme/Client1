/*
du3G_SMS_lbr.h v 0.9711/20160707 - d-u3G 1.13 LIBRARY SUPPORT
COPYRIGHT (c) 2016 Dragos Iosub / R&D Software Solutions srl

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

int noSMS=0, totSMS=0;

#define agsm_SMS
#if !defined (agsm_BASIC)
	#include "du3G_basic_lbr.h"
#endif

int ready4SMS = 0, ready4Voice = 0;

int sendSMS(char* phno, char* message, char* phtype = "129");

void readSMS(int SMSindex);
void readAllSMS(void);
void deleteSMS(int SMSindex);
int	 listSMS(void);
