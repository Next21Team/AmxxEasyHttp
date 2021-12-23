// vi: set ts=4 sw=4 :
// vim: set tw=75 :

// log_mega.cpp - logging routines

/*
 * Copyright (c) 2001-2003 Will Day <willday@hpgx.net>
 *
 *    This file is part of Metamod.
 *
 *    Metamod is free software; you can redistribute it and/or modify it
 *    under the terms of the GNU General Public License as published by the
 *    Free Software Foundation; either version 2 of the License, or (at
 *    your option) any later version.
 *
 *    Metamod is distributed in the hope that it will be useful, but
 *    WITHOUT ANY WARRANTY; without even the implied warranty of
 *    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *    General Public License for more details.
 *
 *    You should have received a copy of the GNU General Public License
 *    along with Metamod; if not, write to the Free Software Foundation,
 *    Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 *    In addition, as a special exception, the author gives permission to
 *    link the code of this program with the Half-Life Game Engine ("HL
 *    Engine") and Modified Game Libraries ("MODs") developed by Valve,
 *    L.L.C ("Valve").  You must obey the GNU General Public License in all
 *    respects for all of the code used other than the HL Engine and MODs
 *    from Valve.  If you modify this file, you may extend this exception
 *    to your version of the file, but you are not obligated to do so.  If
 *    you do not wish to do so, delete this exception statement from your
 *    version.
 *
 */

#include <stdio.h>		// vsnprintf, etc
#include <stdarg.h>		// va_start, etc

#include <extdll.h>				// always
#include "enginecallbacks.h"	// ALERT, etc

#include "log_meta.h"			// me
#include "osdep.h"				// win32 vsnprintf, etc

cvar_t meta_debug = {"meta_debug", "0", FCVAR_EXTDLL, 0, NULL};

enum MLOG_SERVICE {
	mlsCONS = 1,
	mlsDEV,
	mlsIWEL,
	mlsCLIENT
};

static void buffered_ALERT(MLOG_SERVICE service, ALERT_TYPE atype, const char *prefix, const char *fmt, va_list ap);


// Print to console.
void META_CONS(const char *fmt, ...) {
	va_list ap;
	char buf[MAX_LOGMSG_LEN];
	unsigned int len;

	va_start(ap, fmt);
	vsnprintf(buf, sizeof(buf), fmt, ap);
	va_end(ap);
	len=strlen(buf);
	if(len < sizeof(buf)-2)		// -1 null, -1 for newline
		strcat(buf, "\n");
	else
		buf[len-1] = '\n';

	SERVER_PRINT(buf);
}

// Log developer-level messages (obsoleted).
static const char *const prefixDEV = "[META] dev:";
void META_DEV(const char *fmt, ...) {
	va_list ap;
	int dev;

	if(NULL != g_engfuncs.pfnCVarGetFloat) {
		dev=(int) CVAR_GET_FLOAT("developer");
		if(dev==0) return;
	}

	va_start(ap, fmt);
	buffered_ALERT(mlsDEV, at_logged, prefixDEV, fmt, ap);
	va_end(ap);
}

// Log infos.
static const char *const prefixINFO = "[META] INFO:";
void META_INFO(const char *fmt, ...) {
	va_list ap;

	va_start(ap, fmt);
	buffered_ALERT(mlsIWEL, at_logged, prefixINFO, fmt, ap);
	va_end(ap);
}

// Log warnings.
static const char *const prefixWARNING = "[META] WARNING:";
void META_WARNING(const char *fmt, ...) {
	va_list ap;

	va_start(ap, fmt);
	buffered_ALERT(mlsIWEL, at_logged, prefixWARNING, fmt, ap);
	va_end(ap);
}

// Log errors.
static const char *const prefixERROR = "[META] ERROR:";
void META_ERROR(const char *fmt, ...) {
	va_list ap;

	va_start(ap, fmt);
	buffered_ALERT(mlsIWEL, at_logged, prefixERROR, fmt, ap);
	va_end(ap);
}

// Normal log messages.
static const char *const prefixLOG = "[META]";
void META_LOG(const char *fmt, ...) {
	va_list ap;

	va_start(ap, fmt);
	buffered_ALERT(mlsIWEL, at_logged, prefixLOG, fmt, ap);
	va_end(ap);
}

// Print to client.
void META_CLIENT(edict_t *pEntity, const char *fmt, ...) {
	va_list ap;
	char buf[MAX_CLIENTMSG_LEN];
	unsigned int len;

	va_start(ap, fmt);
	vsnprintf(buf, sizeof(buf), fmt, ap);
	va_end(ap);
	len=strlen(buf);
	if(len < sizeof(buf)-2)		// -1 null, -1 for newline
		strcat(buf, "\n");
	else
		buf[len-1] = '\n';

	CLIENT_PRINTF(pEntity, print_console, buf);
}



struct BufferedMessage {
	MLOG_SERVICE service;
	ALERT_TYPE atype;
	const char *prefix;
	char buf[MAX_LOGMSG_LEN];
	BufferedMessage *next;
};

static BufferedMessage *messageQueueStart = NULL;
static BufferedMessage *messageQueueEnd   = NULL;

void buffered_ALERT(MLOG_SERVICE service, ALERT_TYPE atype, const char *prefix, const char *fmt, va_list ap) {
	char buf[MAX_LOGMSG_LEN];
	BufferedMessage *msg;

	if (NULL != g_engfuncs.pfnAlertMessage) {
		vsnprintf(buf, sizeof(buf), fmt, ap);
		ALERT(atype, "%s %s\n", prefix, buf);
		return;
	}

	// Engine AlertMessage function not available. Buffer message.
	msg = new BufferedMessage;
	if (NULL == msg) {
		// though luck, gonna lose this message
		return;
	}
	msg->service = service;
	msg->atype = atype;
	msg->prefix = prefix;
	vsnprintf(msg->buf, sizeof(buf), fmt, ap);
	msg->next = NULL;

	if (NULL == messageQueueEnd) {
		messageQueueStart = messageQueueEnd = msg;
	} else {
		messageQueueEnd->next = msg;
		messageQueueEnd = msg;
	}	
} 


// Flushes the message queue, printing messages to the respective
// service. This function doesn't check anymore if the g_engfuncs
// jumptable is set. Don't call it if it isn't set.
void flush_ALERT_buffer(void) {
	BufferedMessage *msg = messageQueueStart;
	int dev = (int) CVAR_GET_FLOAT("developer");

	while (NULL != msg) {
		if(msg->service == mlsDEV && dev==0) {
			;
		} else {
			ALERT(msg->atype, "b>%s %s\n", msg->prefix, msg->buf);
		}
		messageQueueStart = messageQueueStart->next;
		delete msg;
		msg = messageQueueStart;
	}

	messageQueueStart = messageQueueEnd = NULL;
}

