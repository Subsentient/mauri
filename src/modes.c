/*This code is part of the Epoch Boot System.
* The Epoch Boot System is maintained by Subsentient.
* This software is public domain.
* Please read the file LICENSE.TXT for more information.*/

/**This file holds functions for the various things we can do, depending on argv[0].
 * Most are to be called by main() at some point or another.**/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <ctype.h>
#include <dirent.h>
#include <time.h>
#include "epoch.h"

rStatus TellInitToDo(const char *MembusCode)
{ /*Client side to send a request to halt/reboot/power off/disable or enable CAD/etc.*/
	char InitsResponse[MEMBUS_SIZE/2 - 1], *PCode[2], *PErrMsg;
	unsigned long cCounter = 0;
	
	if (!strcmp(MembusCode, MEMBUS_CODE_HALT))
	{
		PCode[0] = MEMBUS_CODE_ACKNOWLEDGED " " MEMBUS_CODE_HALT;
		PCode[1] = MEMBUS_CODE_FAILURE " " MEMBUS_CODE_HALT;
		PErrMsg = "Unable to halt.";
	}
	else if (!strcmp(MembusCode, MEMBUS_CODE_POWEROFF))
	{
		PCode[0] = MEMBUS_CODE_ACKNOWLEDGED " " MEMBUS_CODE_POWEROFF;
		PCode[1] = MEMBUS_CODE_FAILURE " " MEMBUS_CODE_POWEROFF;
		PErrMsg = "Unable to power off.";
	}
	else if (!strcmp(MembusCode, MEMBUS_CODE_REBOOT))
	{
		PCode[0] = MEMBUS_CODE_ACKNOWLEDGED " " MEMBUS_CODE_REBOOT;
		PCode[1] = MEMBUS_CODE_FAILURE " " MEMBUS_CODE_REBOOT;
		PErrMsg = "Unable to reboot.";
	}
	else if (!strcmp(MembusCode, MEMBUS_CODE_POWEROFFNOW))
	{
		PCode[0] = MEMBUS_CODE_ACKNOWLEDGED " " MEMBUS_CODE_POWEROFFNOW;
		PCode[1] = MEMBUS_CODE_FAILURE " " MEMBUS_CODE_POWEROFFNOW;
		PErrMsg = "Unable to power off.";
	}
	else if (!strcmp(MembusCode, MEMBUS_CODE_REBOOTNOW))
	{
		PCode[0] = MEMBUS_CODE_ACKNOWLEDGED " " MEMBUS_CODE_REBOOTNOW;
		PCode[1] = MEMBUS_CODE_FAILURE " " MEMBUS_CODE_REBOOTNOW;
		PErrMsg = "Unable to reboot.";
	}
	else if (!strcmp(MembusCode, MEMBUS_CODE_HALTNOW))
	{
		PCode[0] = MEMBUS_CODE_ACKNOWLEDGED " " MEMBUS_CODE_HALTNOW;
		PCode[1] = MEMBUS_CODE_FAILURE " " MEMBUS_CODE_HALTNOW;
		PErrMsg = "Unable to halt.";
	}
	else if (!strcmp(MembusCode, MEMBUS_CODE_CADON))
	{
		PCode[0] = MEMBUS_CODE_ACKNOWLEDGED " " MEMBUS_CODE_CADON;
		PCode[1] = MEMBUS_CODE_FAILURE " " MEMBUS_CODE_CADON;
		PErrMsg = "Unable to enable CTRL-ALT-DEL instant reboot.";
	}
	else if (!strcmp(MembusCode, MEMBUS_CODE_CADOFF))
	{
		PCode[0] = MEMBUS_CODE_ACKNOWLEDGED " " MEMBUS_CODE_CADOFF;
		PCode[1] = MEMBUS_CODE_FAILURE " " MEMBUS_CODE_CADOFF;
		PErrMsg = "Unable to disable CTRL-ALT-DEL instant reboot.";
	}
	else
	{
		SpitError("Invalid MEMBUS_CODE passed to TellInitToDo().");
		return FAILURE;
	}
	
	if (!InitMemBus(false))
	{
		SpitError("Failed to connect to membus.");
		return FAILURE;
	}
	
	if (!MemBus_Write(MembusCode, false))
	{
		SpitError("Failed to write to membus.");
		return FAILURE;
	}
	
	while (!MemBus_Read(InitsResponse, false))
	{
		usleep(100000); /*0.1 secs.*/
		++cCounter;
		
		if (cCounter == 200) /*Twenty seconds.*/
		{
			SpitError("Failed to get reply via membus.");
			return FAILURE;
		}
	}
	
	if (!strcmp(InitsResponse, PCode[0]))
	{
		ShutdownMemBus(false);
		return SUCCESS;
	}
	else if (!strcmp(InitsResponse, PCode[1]))
	{ /*Nothing uses this right now.*/
		SpitError(PErrMsg);
		ShutdownMemBus(false);
		
		return FAILURE;
	}
	
	return SUCCESS;
}

rStatus ObjStartStop(const char *ObjectID, const char *MemBusSignal)
{ /*Start and stop services.*/
	char RemoteResponse[MEMBUS_SIZE/2 - 1];
	char OutMsg[MEMBUS_SIZE/2 - 1];
	char PossibleResponses[4][MEMBUS_SIZE/2 - 1];
	unsigned long cCounter = 0;
	
	snprintf(OutMsg, sizeof OutMsg, "%s %s", MemBusSignal, ObjectID);
	
	if (!MemBus_Write(OutMsg, false))
	{
		return FAILURE;
	}
	
	while (!MemBus_Read(RemoteResponse, false))
	{
		usleep(100000); /*0.1 secs*/
		++cCounter;
		
		if (cCounter == 200)
		{ /*Extra newline because we probably are going to print in a progress report.*/
			SpitError("\nFailed to get reply via membus.");
			return FAILURE;
		}
	}
	
	snprintf(PossibleResponses[0], sizeof PossibleResponses[0], "%s %s %s",
		MEMBUS_CODE_ACKNOWLEDGED, MemBusSignal, ObjectID);
		
	snprintf(PossibleResponses[1], sizeof PossibleResponses[1], "%s %s %s",
		MEMBUS_CODE_FAILURE, MemBusSignal, ObjectID);
		
	snprintf(PossibleResponses[2], sizeof PossibleResponses[2], "%s %s",
		MEMBUS_CODE_BADPARAM, OutMsg);

	snprintf(PossibleResponses[3], sizeof PossibleResponses[3], "%s %s %s",
		MEMBUS_CODE_WARNING, MemBusSignal, ObjectID);
		
	if (!strcmp(RemoteResponse, PossibleResponses[0]))
	{
		return SUCCESS;
	}
	else if (!strcmp(RemoteResponse, PossibleResponses[1]))
	{
		return FAILURE;
	}
	else if (!strcmp(RemoteResponse, PossibleResponses[3]))
	{
		return WARNING;
	}
	else if (!strcmp(RemoteResponse, PossibleResponses[2]))
	{
		SpitError("\nWe are being told that we sent a bad parameter.");
		return FAILURE;
	}
	else
	{
		SpitError("\nReceived invalid reply from membus.");
		return FAILURE;
	}
}

Bool AskObjectStarted(const char *ObjectID)
{ /*Just request if the object is running or not.*/
	char RemoteResponse[MEMBUS_SIZE/2 - 1];
	char OutMsg[MEMBUS_SIZE/2 - 1];
	char PossibleResponses[3][MEMBUS_SIZE/2 - 1];
	unsigned long cCounter = 0;
	
	snprintf(OutMsg, sizeof OutMsg, "%s %s", MEMBUS_CODE_STATUS, ObjectID);
	
	if (!MemBus_Write(OutMsg, false))
	{
		return -1; /*Yes, you can do that with the Bool type. It's just a char.*/
	}
	
	while (!MemBus_Read(RemoteResponse, false))
	{ /*Third function, I really am just copying and pasting this part.
		* Hey, this is exactly what I was going to write anyways! */
		usleep(100000); /*0.1 secs*/
		++cCounter;
		
		if (cCounter == 200)
		{ /*Extra newline because we probably are going to print in a progress report.*/
			SpitError("\nFailed to get reply via membus.");
			return -1;
		}
	}
	
	snprintf(PossibleResponses[0], sizeof PossibleResponses[0], "%s %s %s",
		MEMBUS_CODE_STATUS, ObjectID, "0");
	snprintf(PossibleResponses[1], sizeof PossibleResponses[1], "%s %s %s",
		MEMBUS_CODE_STATUS, ObjectID, "1");
	snprintf(PossibleResponses[2], sizeof PossibleResponses[2], "%s %s",
		MEMBUS_CODE_FAILURE, MEMBUS_CODE_STATUS);
		
	if (!strcmp(RemoteResponse, PossibleResponses[0]))
	{
		return false;
	}
	else if (!strcmp(RemoteResponse, PossibleResponses[1]))
	{
		return true;
	}
	else if (!strcmp(RemoteResponse, PossibleResponses[2]))
	{ /*Not sure why I do this when the statement below returns the same.*/
		return -1;
	}
	else
	{
		return -1;
	}
}
	
rStatus EmulKillall5(unsigned long InSignal)
{ /*Used as the killall5 utility.*/
	DIR *ProcDir;
	struct dirent *CurDir;
	unsigned long CurPID, OurPID, Inc;
	FILE *TempDescriptor;
	char TmpBuf[1024], SessionID[8192], SessionID_Targ[8192], TChar;


	if (InSignal > OSCTL_SIGNAL_STOP || InSignal == 0) /*Won't be negative since we are unsigned.*/
	{
		SpitError("EmulKillall5() Bad value for unsigned long InSignal.");
	}
	
	OurPID = (unsigned long)getpid(); /*We need this so we don't kill ourselves.*/
	
	/*Get our Session ID in ASCII, because it's often too big to fit in a 32 bit integer.
	 * Anything in our session ID must live so that our shell lives.*/
	snprintf(TmpBuf, 1024, "/proc/%lu/sessionid", OurPID);
	
	if (!(TempDescriptor = fopen(TmpBuf, "r")))
	{
		SpitError("Failed to read session ID file for ourselves. Aborting.");
		return FAILURE;
	}
	
	for (Inc = 0; (TChar = getc(TempDescriptor)) != EOF && Inc < 8192; ++Inc)
	{
		SessionID[Inc] = TChar;
	}
	SessionID[Inc] = '\0';
	
	fclose(TempDescriptor);
	
	/*We get everything from /proc.*/
	ProcDir = opendir("/proc/");
	
	while ((CurDir = readdir(ProcDir)))
	{
		if (isdigit(CurDir->d_name[0]) && CurDir->d_type == 4)
		{			
			CurPID = atoi(CurDir->d_name); /*Convert the new PID to a true number.*/
			
			if (CurPID == 1 || CurPID == OurPID)
			{ /*Don't try to kill init, or us.*/
				continue;
			}
			
			snprintf(TmpBuf, 1024, "/proc/%lu/sessionid", CurPID);
			
			if (!(TempDescriptor = fopen(TmpBuf, "r")))
			{
				snprintf(TmpBuf, 1024, "Failed to read session ID file for process %lu. Aborting.", CurPID);
				
				SpitError(TmpBuf);
				return FAILURE;
			}
			
			/*Copy in the contents of the file sessionid, using EOF to know when to stop. NOW SHUT UP ABOUT THE LOOPS!
			 * I can't really do this with fread() because these files report zero length!*/
			for (Inc = 0; (TChar = getc(TempDescriptor)) != EOF && Inc < 8192; ++Inc)
			{
				SessionID_Targ[Inc] = TChar;
			}
			SessionID_Targ[Inc] = '\0';
			
			fclose(TempDescriptor);
			
			if (!strncmp(SessionID, SessionID_Targ, 8192))
			{ /*It's in our session ID, so don't touch it.*/
				continue;
			}
			
			/*We made it this far, must be safe to nuke this process.*/
			kill(CurPID, InSignal); /*Actually send the kill, stop, whatever signal.*/
		}
	}
	closedir(ProcDir);
	
	return SUCCESS;
}

void EmulWall(const char *InStream, Bool ShowUser)
{ /*We not only use this as a CLI applet, we use it to notify of impending shutdown too.*/
	FILE *Descriptor = NULL;
	struct tm *TimeP;
	char OutBuf[8192];
	char HMS[3][16];
	char OurUser[256];
	char OurHostname[256];
	char FName[128] = "/dev/tty0";
	long HMS_I[3];
	time_t Clock;
	unsigned long Inc = 0;
	
	
	if (getuid() != 0)
	{ /*Not root?*/
		SpitWarning("You are not root. Only sending to ttys you have privileges on.");
	}
	
	time(&Clock);
	TimeP = localtime(&Clock);
	
	HMS_I[0] = TimeP->tm_hour;
	HMS_I[1] = TimeP->tm_min;
	HMS_I[2] = TimeP->tm_sec;
	
	for (; Inc < 3; ++Inc)
	{
		if (HMS_I[Inc] < 10)
		{
			snprintf(HMS[Inc], 16, "0%ld", HMS_I[Inc]);
		}
		else
		{
			snprintf(HMS[Inc], 16, "%ld", HMS_I[Inc]);
		}
	}
		
	snprintf(OutBuf, 64, "\007\n%s[%s:%s:%s | %d/%d/%d]%s ", CONSOLE_COLOR_RED, HMS[0], HMS[1], HMS[2],
		TimeP->tm_mon + 1, TimeP->tm_mday, TimeP->tm_year + 1900, CONSOLE_ENDCOLOR);
	
	if (ShowUser)
	{
		if (getlogin_r(OurUser, sizeof OurUser) != 0)
		{
			snprintf(OurUser, sizeof OurUser, "%s", "(unknown)");
		}
		
		if (gethostname(OurHostname, sizeof OurHostname) != 0)
		{
			snprintf(OurHostname, sizeof OurHostname, "%s", "(unknown)");
		}
		
		/*I really enjoy pulling stuff off like the line below.*/
		snprintf(&OutBuf[strlen(OutBuf)], sizeof OutBuf - strlen(OutBuf), "Broadcast message from %s@%s: ", OurUser, OurHostname);
		
	}
	else
	{
		snprintf(&OutBuf[strlen(OutBuf)], sizeof OutBuf - strlen(OutBuf), "%s", "Broadcast message: ");
	}
	
	snprintf(&OutBuf[strlen(OutBuf)], sizeof OutBuf - strlen(OutBuf), "\n%s\n\n", InStream);
	
	/*Now write to the ttys.*/
	for (Inc = 1; (Descriptor = fopen(FName, "r")); ++Inc) /*See, we use fopen() as a way to check if the file exists.*/
	{ /*Your eyes bleeding yet?*/
		fclose(Descriptor);
		
		Descriptor = fopen(FName, "w");
		fwrite(OutBuf, 1, strlen(OutBuf), Descriptor);
		fflush(NULL);
		fclose(Descriptor);
		Descriptor = NULL;
		
		snprintf(FName, sizeof FName, "/dev/tty%lu", Inc);
	}
	
	snprintf(FName, sizeof FName, "%s", "/dev/pts/0");
	
	for (Inc = 1; (Descriptor = fopen(FName, "r")); ++Inc)
	{
		fclose(Descriptor);
		
		Descriptor = fopen(FName, "w");
		fwrite(OutBuf, 1, strlen(OutBuf), Descriptor);
		fflush(NULL);
		fclose(Descriptor);
		
		Descriptor = NULL;
		
		snprintf(FName, sizeof FName, "/dev/pts/%lu", Inc);
	}
}
