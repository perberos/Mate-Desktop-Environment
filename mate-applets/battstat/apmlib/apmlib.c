/* apmlib.c -- Sample APM interface routines
 * Created: Mon Jan  8 10:28:16 1996 by faith@acm.org
 * Revised: Fri Dec 26 21:38:29 1997 by faith@acm.org
 * Copyright 1996, 1997 Rickard E. Faith (faith@acm.org)
 * 
 * This library is free software; you can redistribute it and/or modify it
 * under the terms of the GNU Library General Public License as published
 * by the Free Software Foundation; either version 2 of the License, or (at
 * your option) any later version.
 * 
 * This library is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 * 
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 */
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <ctype.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/sysmacros.h>
#include "apm.h"

#define BACKWARD_COMPAT 1

/* If APM support of the right version exists in kernel, return zero.
 * Otherwise, return 1 if no support exists, or 2 if it is the wrong
 * version.  *NOTE* The sense of the return value is not intuitive.
 */
int apm_exists(void)
{
    apm_info i;

    if (access(APM_PROC, R_OK))
	return 1;
    return apm_read(&i);
}


/* Read information from /proc/apm.  Return 0 on success, 1 if APM not
 * installed, 2 if APM installed, but old version. 
 */
int apm_read(apm_info * i)
{
    FILE *str;
    char units[10];
    char buffer[100];
    int retcode = 0;

    if (!(str = fopen(APM_PROC, "r")))
	return 1;
    fgets(buffer, sizeof(buffer) - 1, str);
    buffer[sizeof(buffer) - 1] = '\0';

    /* Should check for other driver versions; driver 1.9 (and some
     * others) uses this format, which doesn't expose # batteries.
     */
    sscanf(buffer, "%s %d.%d %x %x %x %x %d%% %d %s\n",
	   (char *) i->driver_version,
	   &i->apm_version_major,
	   &i->apm_version_minor,
	   &i->apm_flags,
	   &i->ac_line_status,
	   &i->battery_status,
	   &i->battery_flags,
	   &i->battery_percentage,
	   &i->battery_time,
	   units);
    i->using_minutes = !strncmp(units, "min", 3) ? 1 : 0;
    if (i->driver_version[0] == 'B')
    {				/* old style.  argh. */
#if !BACKWARD_COMPAT
	retcode = 2;
#else
	strcpy((char *) i->driver_version, "pre-0.7");
	i->apm_version_major = 0;
	i->apm_version_minor = 0;
	i->apm_flags = 0;
	i->ac_line_status = 0xff;
	i->battery_status = 0xff;
	i->battery_flags = 0xff;
	i->battery_percentage = -1;
	i->battery_time = -1;
	i->using_minutes = 1;

	sscanf(buffer, "BIOS version: %d.%d",
	       &i->apm_version_major, &i->apm_version_minor);
	fgets(buffer, sizeof(buffer) - 1, str);
	sscanf(buffer, "Flags: 0x%02x", &i->apm_flags);
	if (i->apm_flags & APM_32_BIT_SUPPORT)
	{
	    fgets(buffer, sizeof(buffer) - 1, str);
	    fgets(buffer, sizeof(buffer) - 1, str);
	    if (buffer[0] != 'P')
	    {
		if (!strncmp(buffer + 4, "off line", 8))
		    i->ac_line_status = 0;
		else if (!strncmp(buffer + 4, "on line", 7))
		    i->ac_line_status = 1;
		else if (!strncmp(buffer + 4, "on back", 7))
		    i->ac_line_status = 2;

		fgets(buffer, sizeof(buffer) - 1, str);
		if (!strncmp(buffer + 16, "high", 4))
		    i->battery_status = 0;
		else if (!strncmp(buffer + 16, "low", 3))
		    i->battery_status = 1;
		else if (!strncmp(buffer + 16, "crit", 4))
		    i->battery_status = 2;
		else if (!strncmp(buffer + 16, "charg", 5))
		    i->battery_status = 3;

		fgets(buffer, sizeof(buffer) - 1, str);
		if (strncmp(buffer + 14, "unknown", 7))
		    i->battery_percentage = atoi(buffer + 14);
		if (i->apm_version_major >= 1 && i->apm_version_minor >= 1)
		{
		    fgets(buffer, sizeof(buffer) - 1, str);
		    sscanf(buffer, "Battery flag: 0x%02x", &i->battery_flags);

		    fgets(buffer, sizeof(buffer) - 1, str);
		    if (strncmp(buffer + 14, "unknown", 7))
			i->battery_time = atoi(buffer + 14);
		}
	    }
	}
#endif
    }

    /* Fix possible kernel bug -- percentage
     * set to 0xff (==255) instead of -1. 
     */
    if (i->battery_percentage > 100)
	i->battery_percentage = -1;

    fclose(str);
    return retcode;
}


/* Lookup the device number for the apm_bios device. */
dev_t apm_dev(void)
{
    FILE *str;
    static int cached = -1;
    char buf[80];
    char *pt;
    apm_info i;

    if (cached >= 0)
	return cached;

    if (access(APM_PROC, R_OK) || apm_read(&i) == 1)
	return cached = -1;
    if (i.driver_version[0] == '1')
	return cached = makedev(10, 134);

    if (!(str = fopen(APM_DEV, "r")))
	return -1;
    while (fgets(buf, sizeof(buf) - 1, str))
    {
	buf[sizeof(buf) - 1] = '\0';
	for (pt = buf; *pt && isspace(*pt); ++pt);	/* skip leading spaces */
	for (; *pt && !isspace(*pt); ++pt);	/* find next space */
	if (isspace(*pt))
	{
	    *pt++ = '\0';
	    pt[strlen(pt) - 1] = '\0';	/* get rid of newline */
	    if (!strcmp(pt, APM_NAME))
	    {
		fclose(str);
		return cached = makedev(atoi(buf), 0);
	    }
	}
    }
    fclose(str);
    return cached = -1;
}


/* Return a file descriptor for the apm_bios device, or -1 if there is an
 * error.  Is this method secure?  Should we make the device in /dev
 * instead of /tmp? 
 *
 * apenwarr 2001/05/11: just throw out the weird temporary device file stuff.
 *	It was only for ancient kernel versions anyway.
 */
int apm_open(void)
{
    int fd;
    apm_info i;

    if (access(APM_PROC, R_OK) || apm_read(&i) == 1)
	return -1;
    if (i.driver_version[0] >= '1')
    {
	if ((fd = open(APM_DEVICE, O_RDWR)) < 0)
	{
	    /* Try to create it.  This is reasonable
	     * for backward compatibility.
	     */
	    if (mknod(APM_DEVICE, S_IFCHR | S_IRUSR | S_IWUSR, apm_dev()))
	    {
		unlink(APM_DEVICE);
		return -1;
	    }
	    fd = open(APM_DEVICE, O_RDWR);
	}
	
	return fd;
    }
    
    return -1;
}


/* Given a file descriptor for the apm_bios device, close it. */
int apm_close(int fd)
{
    return close(fd);
}

/* Given a file descriptor for the apm_bios device, this routine will wait
 * timeout seconds for APM events.  Up to n events will be placed in the
 * events queue.  The return code will indicate the number of events
 * stored.  Since this routine uses select(2), it will return if an
 * unblocked signal is caught.  A timeout < 0 means to block indefinately.
 *
 * Note that if you read a request to standby or to suspend, the kernel
 * will be waiting for you to respond to it with a call to apm_suspend()
 * or to apm_standby() !
 */
int apm_get_events(int fd, int timeout, apm_event_t * events, int n)
{
    int retcode;
    fd_set fds;
    struct timeval t;

    t.tv_sec = timeout;
    t.tv_usec = 0;

    FD_ZERO(&fds);
    FD_SET(fd, &fds);
    retcode = select(fd + 1, &fds, NULL, NULL, timeout < 0 ? NULL : &t);
    if (retcode <= 0)
	return 0;
    return read(fd, events, n * sizeof(apm_event_t)) / sizeof(apm_event_t);
}


/* Try to set the Power State to Suspend. */
int apm_suspend(int fd)
{
    sync();
    return ioctl(fd, APM_IOC_SUSPEND, NULL);
}


/* Try to set the Power State to Standby. */
int apm_standby(int fd)
{
    sync();
    return ioctl(fd, APM_IOC_STANDBY, NULL);
}

/* Return the last error code generated by the kernel APM driver */
unsigned int apm_last_error( int fd )
{
    int err = 0;

#ifdef APM_IOC_LAST_ERROR
    int ierr = 0;
    if ( (ierr = ioctl( fd, APM_IOC_LAST_ERROR, &err)) )
	return ierr;
#endif
    return err;
}

/* Define lookup table for error messages */
typedef struct lookup_t {
	int	key;
	char *	msg;
} lookup_t;

/* APM error messages, arranged by error code */
static const lookup_t error_table[] = {
/* N/A	{ APM_SUCCESS,		"Operation succeeded" }, */
	{ APM_DISABLED,		"Power management disabled" },
	{ APM_CONNECTED,	"Real mode interface already connected" },
	{ APM_NOT_CONNECTED,	"Interface not connected" },
	{ APM_16_CONNECTED,	"16 bit interface already connected" },
/* N/A	{ APM_16_UNSUPPORTED,	"16 bit interface not supported" }, */
	{ APM_32_CONNECTED,	"32 bit interface already connected" },
	{ APM_32_UNSUPPORTED,	"32 bit interface not supported" },
	{ APM_BAD_DEVICE,	"Unrecognized device ID" },
	{ APM_BAD_PARAM,	"Parameter out of range" },
	{ APM_NOT_ENGAGED,	"Interface not engaged" },
#ifdef APM_BAD_FUNCTION
	{ APM_BAD_FUNCTION,     "Function not supported" },
#endif
#ifdef APM_RESUME_DISABLED
	{ APM_RESUME_DISABLED,	"Resume timer disabled" },
#endif
	{ APM_BAD_STATE,	"Unable to enter requested state" },
/* N/A	{ APM_NO_EVENTS,	"No events pending" }, */
	{ APM_NOT_PRESENT,	"No APM present" }
};
#define ERROR_COUNT	(sizeof(error_table)/sizeof(lookup_t))

/* Return character string describing error messages from APM kernel */
const char *apm_error_name( unsigned int err )
{
  int i;
  
  for(i=0; i<ERROR_COUNT; i++)
    if(err == error_table[i].key) return(error_table[i].msg);

  return "Unknown error";
}

int apm_reject( int fd )
{
#ifdef APM_IOC_REJECT
    if ( ioctl( fd, APM_IOC_REJECT, NULL ) ) 
	return apm_last_error( fd );
    else
#endif
	return 0;
}

#ifdef APM_IOC_IGNORE		/* detect kernel support of IGNORE/NOIGNORE functions */
int apm_set_ignore(int fd, int mode)
/* Ignore Standby. */
{
    if (mode == IGNORE)
    {
	printf("Telling kernel to ignore system standby/suspend mode\n");
	return ioctl(fd, APM_IOC_IGNORE, NULL);
    }
    else
    {
	printf("Telling kernel not to ignore system standby/suspend mode\n");
	return ioctl(fd, APM_IOC_NOIGNORE, NULL);
    }
    printf("NOTE: User-generated suspend/standby requests are not ignored\n");
}
#endif

/* Return a string describing the event. From p. 16 of the Intel/Microsoft
 * Advanded Power Management (APM) BIOS Interface Specification, Revision
 * 1.1 (September 1993). Intel Order Number: 241704-001.  Microsoft Part
 * Number: 781-110-X01.
 *
 * Updated to APM BIOS 1.2 spec (February 1996).  Available on-line.
 */
const char *apm_event_name(apm_event_t event)
{
    switch (event)
    {
    case APM_SYS_STANDBY:
	return "System Standby Request";
    case APM_SYS_SUSPEND:
	return "System Suspend Request";
    case APM_NORMAL_RESUME:
	return "Normal Resume System";
    case APM_CRITICAL_RESUME:
	return "Critical Resume System";
    case APM_LOW_BATTERY:
	return "Battery Low";
    case APM_POWER_STATUS_CHANGE:
	return "Power Status Change";
    case APM_UPDATE_TIME:
	return "Update Time";
    case APM_CRITICAL_SUSPEND:
	return "Critical Suspend";
    case APM_USER_STANDBY:
	return "User System Standby Request";
    case APM_USER_SUSPEND:
	return "User System Suspend Request";
    case APM_STANDBY_RESUME:
	return "System Standby Resume";
#ifdef APM_CAPABILITY_CHANGE
    case APM_CAPABILITY_CHANGE:
	return "Capability Change";
#endif
    }
    return "Unknown";
}


/* This is a convenience function that has nothing to do with APM.  It just
 * formats a time nicely.  If you don't like this format, then write your
 * own.
 */
#define SEC_PER_DAY  (60*60*24)
#define SEC_PER_HOUR (60*60)
#define SEC_PER_MIN  (60)

const char *apm_delta_time(time_t then, time_t now)
{
    return apm_time(now - then);
}

const char *apm_time(time_t t)
{
    static char buffer[128];
    unsigned long s, m, h, d;

    d = t / SEC_PER_DAY;
    t -= d * SEC_PER_DAY;
    h = t / SEC_PER_HOUR;
    t -= h * SEC_PER_HOUR;
    m = t / SEC_PER_MIN;
    t -= m * SEC_PER_MIN;
    s = t;

    if (d)
	sprintf(buffer, "%lu day%s, %02lu:%02lu:%02lu",
		d, d > 1 ? "s" : "", h, m, s);
    else
	sprintf(buffer, "%02lu:%02lu:%02lu", h, m, s);

    if (t == -1)
	sprintf(buffer, "unknown");

    return buffer;
}

const char *apm_time_nosec(time_t t)
{
    static char buffer[128];
    unsigned long s, m, h, d;

    d = t / SEC_PER_DAY;
    t -= d * SEC_PER_DAY;
    h = t / SEC_PER_HOUR;
    t -= h * SEC_PER_HOUR;
    m = t / SEC_PER_MIN;
    t -= m * SEC_PER_MIN;
    s = t;

    if (s > 30)
	++m;

    if (d)
	sprintf(buffer, "%lu day%s, %lu:%02lu",
		d, d > 1 ? "s" : "", h, m);
    else
	sprintf(buffer, "%lu:%02lu", h, m);

    if (t == -1)
	sprintf(buffer, "unknown");

    return buffer;
}
