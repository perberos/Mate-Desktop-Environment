/* -*- Mode: C; tab-width: 8; indent-tabs-mode: 8; c-basic-offset: 8 -*- */

/*

  cdda-cddb.c

  Based on code from libcdaudio 0.5.0 (Copyright (C)1998 Tony Arcieri)

  All changes copyright (c) 1998 by Mike Oliphant - oliphant@ling.ed.ac.uk

    http://www.ling.ed.ac.uk/~oliphant/grip

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111, USA.

*/

#include "cdda-cddb.h"

#include <config.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <string.h>
#include <pwd.h>
#include <errno.h>
#include <unistd.h>

#include <glib.h>

#define size16 short
#define size32 int
#include <cdda_interface.h>

#include "cdda-cdrom-extensions.h"

/* This is here to work around a broken header file.
 * cdda_interface.h has a statically defined array of
 * chars that is unused. This will break our build
 * due to our strict error checking.
 */
char **broken_header_fix2 = strerror_tr;

static int 	CDDBSum(int val);
static int 	CDDBConnect(CDDBServer *server);
static void 	CDDBDisconnect(int sock);
static void 	CDDBSkipHTTP(int sock);
static int 	CDDBReadLine(int sock,char *inbuffer,int len);
static void 	CDDBMakeHello(CDDBHello *hello,char *hellobuf);
static void 	CDDBMakeRequest(CDDBServer *server, CDDBHello *hello, char *cmd,char *outbuf,int outlen);
static void 	CDDBProcessLine(char *inbuffer,DiscData *data, int numtracks);
#if 0
static void CDDBWriteLine(char *header,int num,char *data,FILE *outfile);
#endif

static const char *cddb_genres[] = {"unknown","blues","classical","country",
			      "data","folk","jazz","misc","newage",
			      "reggae","rock","soundtrack"};

#ifdef ENABLE_IPV6
/*Check whether the node is IPv6 enabled.*/
static gboolean
have_ipv6 (void)
{
	int s;

	s = socket (AF_INET6, SOCK_STREAM, 0);
	if (s != -1) {
		close (s);
		return TRUE;
	}

	return FALSE;
}
#endif

/* CDDB sum function */
static int 
CDDBSum(int val)
{
	char *bufptr, buf[16];
	int ret = 0;
   
	g_snprintf(buf,16,"%lu",(unsigned long int)val);

	for(bufptr = buf; *bufptr != '\0'; bufptr++) {
		ret += (*bufptr - '0');
	}   
	return ret;
}

/* Produce CDDB ID for CD currently in CD-ROM */
unsigned int 
CDDBDiscid (cdrom_drive *drive)
{
	int index, tracksum = 0, discid, retval;
	disc_info disc;

	retval = CDStat (drive->ioctl_fd, &disc, TRUE);
	
	for(index = 0; index < disc.disc_totaltracks; index++) {
    	tracksum += CDDBSum(disc.track[index].track_pos.minutes * 60 +
			 disc.track[index].track_pos.seconds);
  	}
  	
	discid = (disc.disc_length.minutes * 60 + disc.disc_length.seconds) -
			 (disc.track[0].track_pos.minutes * 60 + disc.track[0].track_pos.seconds);
  	
	return (tracksum % 0xFF) << 24 | discid << 8 | disc.disc_totaltracks;
}

/* Convert numerical genre to text */
char *CDDBGenre(int genre)
{
	if(genre>11) {
		return("unknown");
	}

	return cddb_genres[genre];
}

/* Convert genre from text form into an integer value */
int CDDBGenreValue(char *genre)
{
	int pos;

	for (pos = 0; pos < 12; pos++) {
    	if (!strcmp (genre,cddb_genres[pos])) {
    		return pos;
    	}
    }
	return 0;
}

/* Connect to a CDDB server */
static int 
CDDBConnect (CDDBServer *server)
{
	int sock = -1;
#ifdef ENABLE_IPV6
	struct sockaddr_in6 sin6;
	struct addrinfo hints, *result, *res;  /*info abt the IP of node*/
#endif
	struct sockaddr_in sin;
	struct hostent *host;
	char *sname;
  
#ifdef ENABLE_IPV6
	if (have_ipv6 ()) {
		result = NULL;

		memset (&sin6, 0 , sizeof (sin6));	
		sin6.sin6_family = AF_INET6;

		if (server->use_proxy) {
			sin6.sin6_port = htons (server->proxy->port);
		} else {
			sin6.sin6_port = htons (server->port);
		}
	}
#endif

	memset (&sin, 0, sizeof (sin));
	sin.sin_family = AF_INET;

	if (server->use_proxy)
		sin.sin_port=htons(server->proxy->port);
	else
		sin.sin_port = htons(server->port);
  
	if (server->use_proxy)
		sname=server->proxy->name;
	else
		sname=server->name;
  
#ifdef ENABLE_IPV6
	if (have_ipv6 ()) {
		memset (&hints, 0, sizeof (hints));
		hints.ai_socktype = SOCK_STREAM;

		if ((getaddrinfo (sname, NULL, &hints, &result)) != 0) {
			return -1;
		}
      
		for (res = result; res; res = res->ai_next) {

			if (res->ai_family != AF_INET && res->ai_family != AF_INET6) {
				continue;
			}

			sock = socket (res->ai_family, SOCK_STREAM, 0);
			if (sock < 0) {
				continue;
			}

			if (res->ai_family == AF_INET) {
				memcpy (&sin.sin_addr, &((struct sockaddr_in *)res->ai_addr)->sin_addr, sizeof (struct in_addr));

				if (connect (sock, (struct sockaddr *)&sin, sizeof (sin)) != -1) {
					break;
				}
			}

			if (res->ai_family == AF_INET6) {
				memcpy (&sin6.sin6_addr, &((struct sockaddr_in6 *)res->ai_addr)->sin6_addr, sizeof (struct in6_addr));
        
				if (connect (sock, (struct sockaddr *)&sin6, sizeof (sin6)) != -1) {
					break;
				}
			}

			close (sock);
		}

		freeaddrinfo (result);

		if (!res) {
			/* No valid address found. */
			return -1;
		}
	} else
#endif  /*IPv4*/
	{
		sin.sin_addr.s_addr = inet_addr (sname);
#ifdef SOLARIS
		if (sin.sin_addr.s_addr == (unsigned long)-1)
#else
		if (sin.sin_addr.s_addr == INADDR_NONE)
#endif
		{
			host = gethostbyname (sname);
			if (host == NULL) {
				return -1;
			}

			bcopy (host->h_addr, &sin.sin_addr, host->h_length);
		}

		sock = socket (AF_INET, SOCK_STREAM, 0);
		if (sock < 0) {
			return -1;
		}
  
		if (connect (sock, (struct sockaddr *)&sin, sizeof (sin)) < 0) {
			return -1;
		}

	}

	return sock;
}


/* Disconnect from CDDB server */

static void CDDBDisconnect(int sock)
{
  shutdown(sock,2);
  close(sock);
}

/* Skip http header */

static void CDDBSkipHTTP(int sock)
{
	char inchar;
	int len;

	do {
		len=0;
		do {
			read(sock,&inchar,1);
			len++;
			/* g_message ("%c",inchar); */
		}
		while(inchar!='\n');
	}	
	while(len>2);
}

/* Read a single line from the CDDB server*/

static int CDDBReadLine(int sock,char *inbuffer,int len)
{
  int index;
  char inchar;
  char *pos;
  
  pos=inbuffer;

  for(index = 0; index < len; index++) {
    read(sock, &inchar, 1);

    if(inchar == '\n') {
      	inbuffer[index] = '\0';
     	/* g_message ("[%s]\n",pos); */
      	pos=inbuffer+index;

		if(inbuffer[0] == '.')
			return 1;

		return 0;
	}

	inbuffer[index] = inchar;
	}

  return index;
}

/* Make a 'hello' string from a cddb_hello structure */

static void CDDBMakeHello(CDDBHello *hello,char *hellobuf)
{
  g_snprintf(hellobuf,256,"&hello=private+free.the.cddb+%s+%s",
	   hello->hello_program,hello->hello_version);
}

/* Make a CDDB http request string */

static void CDDBMakeRequest(CDDBServer *server,
			    CDDBHello *hello,
			    char *cmd,char *outbuf,int outlen)
{
  char hellobuf[256];  

  CDDBMakeHello(hello,hellobuf);

  if(server->use_proxy)
    g_snprintf(outbuf,outlen,
	     "GET http://%s/%s?cmd=%s%s&proto=%s HTTP/1.1\r\nHost: %s\r\nUser-Agent: %s/%s\r\nAccept: text/plain\n\n",
	     server->name,server->cgi_prog,cmd,hellobuf,
	       /* CDDA_CDDB_LEVEL, server->name, Program, Version);*/
	     CDDA_CDDB_LEVEL, server->name, "Loser", "1.0");
  else
    g_snprintf(outbuf,outlen,"GET /%s?cmd=%s%s&proto=%s HTTP/1.1\r\nHost: %s\r\nUser-Agent: %s/%s\r\nAccept: text/plain\n\n",
	     server->cgi_prog,cmd,hellobuf,CDDA_CDDB_LEVEL,server->name,
	       /* Program,Version); */
	     "Loser", "1.0");
}

/* Query the CDDB for the CD currently in the CD-ROM */
gboolean 
CDDBDoQuery (cdrom_drive *cd_desc, CDDBServer *server, CDDBHello *hello, CDDBQuery *query)
{
	int socket, index;
	disc_info disc;
	char *offset_buffer, *query_buffer, *http_buffer, inbuffer[256];
	int tot_len,len;

	socket = CDDBConnect (server);
	if (socket==-1) {
		/* g_message ("CDDBConnect failure"); */
  		return FALSE;
	}

	query->query_matches = 0;

  	CDStat (cd_desc->ioctl_fd, &disc, TRUE);
	
	/* Figure out a good buffer size -- 7 chars per track, plus 256 for the rest of the query */
	tot_len = (disc.disc_totaltracks * 7) + 256;

	offset_buffer = malloc(tot_len);
	len = 0;

	len += g_snprintf (offset_buffer + len, tot_len - len, "%d", disc.disc_totaltracks);

	for (index = 0; index < disc.disc_totaltracks; index++) {
		len += g_snprintf (offset_buffer + len, tot_len - len, "+%d", disc.track[index].track_start);
    }

	query_buffer = malloc(tot_len);

	g_snprintf (query_buffer, tot_len, "cddb+query+%08x+%s+%d",
				CDDBDiscid(cd_desc),
				offset_buffer,
				disc.disc_length.minutes * 60 +
				disc.disc_length.seconds);

	http_buffer = malloc(tot_len);

	CDDBMakeRequest (server, hello, query_buffer, http_buffer, tot_len);

	/* g_message ("Query is [%s]\n",http_buffer); */

	write (socket, http_buffer, strlen (http_buffer));

	free (offset_buffer);
	free (query_buffer);
	free (http_buffer);
   
	CDDBSkipHTTP (socket);

	*inbuffer='\0';

	CDDBReadLine(socket,inbuffer,256);

	/* Skip the keep-alive */
	if((strlen(inbuffer)<5)||!strncmp(inbuffer,"Keep",4)) {
		/* g_message("Skipping keepalive\n"); */
		CDDBReadLine (socket,inbuffer,256);
	}

  	/* g_message ("Reply is [%s]\n",inbuffer); */

	switch(strtol(strtok(inbuffer," "),NULL,10)) {
    /* 200 - exact match */
  case 200:
    query->query_match=MATCH_EXACT;
    query->query_matches=1;

    query->query_list[0].list_genre=
      CDDBGenreValue(ChopWhite(strtok(NULL," ")));

    sscanf(ChopWhite(strtok(NULL," ")),"%xd",
	   &query->query_list[0].list_id);

    CDDBParseTitle(ChopWhite(strtok(NULL,"")),query->query_list[0].list_title,
		   query->query_list[0].list_artist,"/");

    break;
    /* 211 - inexact match */
  case 211:
    query->query_match=MATCH_INEXACT;
    query->query_matches=0;

    while(query->query_matches < MAX_INEXACT_MATCHES && !CDDBReadLine(socket,inbuffer,256)) {
      query->query_list[query->query_matches].list_genre=
	CDDBGenreValue(ChopWhite(strtok(inbuffer," ")));
      
      sscanf(ChopWhite(strtok(NULL," ")),"%xd",
	     &query->query_list[query->query_matches].list_id);
      
      CDDBParseTitle(ChopWhite(strtok(NULL,"")),
		     query->query_list[query->query_matches].list_title,
		     query->query_list[query->query_matches].list_artist,"/");

      query->query_matches++;
    }
      
    break;
    
    /* No match */
	default:
    	query->query_match=MATCH_NOMATCH;

    CDDBDisconnect(socket);

    return FALSE;
  }

  CDDBDisconnect(socket);
  
  return TRUE;
}

/* Get rid of whitespace at the beginning or end of a string */

char *ChopWhite(char *buf)
{
  int pos;

  for(pos=strlen(buf)-1;(pos>=0)&&g_ascii_isspace(buf[pos]);pos--);

  buf[pos+1]='\0';

  for(;g_ascii_isspace(*buf);buf++);

  return buf;
}

/* Split string into title/artist */

void CDDBParseTitle(char *buf,char *title,char *artist,char *sep)
{
  char *tmp;

  tmp=strtok(buf,sep);

  if(!tmp) return;

  strncpy(artist,ChopWhite(tmp),64);

  tmp=strtok(NULL,"");

  if(tmp)
    strncpy(title,ChopWhite(tmp),64);
  else strcpy(title,artist);
}

/* Process a line of input data */

static void CDDBProcessLine(char *inbuffer,DiscData *data,
			    int numtracks)
{
  int track;
  int len = 0;
  char *st;
  
  if(!g_ascii_strncasecmp(inbuffer,"DTITLE",6)) {
    len = strlen(data->data_title);

    strncpy(data->data_title+len,ChopWhite(inbuffer+7),256-len);
  }
  else if(!g_ascii_strncasecmp(inbuffer,"DYEAR",5)) {
    strtok(inbuffer,"=");
    
    st = strtok(NULL, "");
    if(st == NULL)
        return;

    data->data_year=atoi(ChopWhite(st));
  }
  else if(!g_ascii_strncasecmp(inbuffer,"DGENRE",6)) {
    strtok(inbuffer,"=");
    
    st = strtok(NULL, "");
    if(st == NULL)
        return;
    
    data->data_genre=CDDBGenreValue(ChopWhite(st));
  }
  else if(!g_ascii_strncasecmp(inbuffer,"TTITLE",6)) {
    track=atoi(strtok(inbuffer+6,"="));
    
    if(track >= 0 && track<numtracks)
    {
      len=strlen(data->data_track[track].track_name);

      strncpy(data->data_track[track].track_name+len,
	    ChopWhite(strtok(NULL,"")),256-len);
    }
  }
  else if(!g_ascii_strncasecmp(inbuffer,"TARTIST",7)) {
    data->data_multi_artist=TRUE;

    track=atoi(strtok(inbuffer+7,"="));
    
    if(track >= 0 && track<numtracks)
    {
      len=strlen(data->data_track[track].track_artist);

      st = strtok(NULL, "");
      if(st == NULL)
	  return;    
      
      strncpy(data->data_track[track].track_artist+len,
	    ChopWhite(st),256-len);
    }
  }
  else if(!g_ascii_strncasecmp(inbuffer,"EXTD",4)) {
    len=strlen(data->data_extended);

    strncpy(data->data_extended+len,ChopWhite(inbuffer+5),4096-len);
  }
  else if(!g_ascii_strncasecmp(inbuffer,"EXTT",4)) {
    track=atoi(strtok(inbuffer+4,"="));
    
    if(track >= 0 && track<numtracks)
    {
      len=strlen(data->data_track[track].track_extended);

      st = strtok(NULL, "");
      if(st == NULL)
	return;

      strncpy(data->data_track[track].track_extended+len,
	  ChopWhite(st),4096-len);
    }
  }
  else if(!g_ascii_strncasecmp(inbuffer,"PLAYORDER",5)) {
    len=strlen(data->data_playlist);

    strncpy(data->data_playlist+len,ChopWhite(inbuffer+10),256-len);
  }
}


/* Read the actual CDDB entry */
gboolean CDDBRead(cdrom_drive *cd_desc, CDDBServer *server,
		  CDDBHello *hello,CDDBEntry *entry,
		  DiscData *data)
{
  int socket;
  int index;
  char outbuffer[256], inbuffer[512],cmdbuffer[256];
  disc_info disc;
  
  socket=CDDBConnect(server);
  if(socket==-1) return FALSE;
 
  memset (&disc, 0, sizeof (disc)); 
  /* CDStat(cd_desc,&disc,TRUE); */
  
  data->data_genre=entry->entry_genre;
  data->data_id=CDDBDiscid(cd_desc);
  *(data->data_extended)='\0';
  *(data->data_title)='\0';
  *(data->data_artist)='\0';
  *(data->data_playlist)='\0';
  data->data_multi_artist=FALSE;
  data->data_year=0;

  for(index=0;index<MAX_TRACKS;index++) {
    *(data->data_track[index].track_name)='\0';
    *(data->data_track[index].track_artist)='\0';
    *(data->data_track[index].track_extended)='\0';
  }

  g_snprintf(cmdbuffer,256,"cddb+read+%s+%08x",CDDBGenre(entry->entry_genre),
	   entry->entry_id);
  
  CDDBMakeRequest(server,hello,cmdbuffer,outbuffer,256);

  write(socket,outbuffer,strlen(outbuffer));
   
  CDDBSkipHTTP(socket);

  CDDBReadLine(socket,inbuffer,256);

  /* Skip the keep-alive */
  if((strlen(inbuffer)<5)||!strncmp(inbuffer,"Keep",4)) {
	  /* g_message ("Skipping keepalive\n"); */
		CDDBReadLine(socket,inbuffer,256);
  }

  while(!CDDBReadLine(socket,inbuffer,512))
    CDDBProcessLine(inbuffer,data,disc.disc_totaltracks);

  /* Both disc title and artist have been stuffed in the title field, so the
     need to be separated */

  CDDBParseTitle(data->data_title,data->data_title,data->data_artist,"/");

  CDDBDisconnect(socket);
   
  return 0;
}

/* See if a disc is in the local database */

gboolean CDDBStatDiscData(cdrom_drive *cd_desc)
{
  int index,id;
  /* disc_info disc; */
  struct stat st;
  char root_dir[256], file[256];
  
  /* CDStat(cd_desc,&disc,TRUE); */

  id=CDDBDiscid(cd_desc);
  
  g_snprintf(root_dir,256,"%s/.cddb",getenv("HOME"));
  
  if(stat(root_dir, &st) < 0)
    return FALSE;
  else {
    if(!S_ISDIR(st.st_mode))
      return FALSE;
  }
  
  g_snprintf(file,256,"%s/%08x",root_dir,id);
  if(stat(file,&st)==0) return TRUE;

  for(index=0;index<12;index++) {
    g_snprintf(file,256,"%s/%s/%08x",root_dir,CDDBGenre(index),id);

    if(stat(file,&st) == 0)
      return TRUE;
  }
   
  return FALSE;
}

/* Read from the local database */
int 
CDDBReadDiscData(cdrom_drive *cd_desc,DiscData *ddata)
{
	FILE *cddb_data = NULL;
	int index,genre;
	char root_dir[256], file[256], inbuf[512];
	disc_info disc;
	struct stat st;
  
	g_snprintf(root_dir,256,"%s/.cddb",getenv("HOME"));
  
	if(stat(root_dir, &st) < 0) {
    	return -1;
	} else {
    	if(!S_ISDIR(st.st_mode)) {
			errno = ENOTDIR;
			return -1;
		}
	}
  
	CDStat (cd_desc->ioctl_fd, &disc, TRUE);

  ddata->data_id=CDDBDiscid(cd_desc);
  *(ddata->data_extended)='\0';
  *(ddata->data_title)='\0';
  *(ddata->data_artist)='\0';
  *(ddata->data_playlist)='\0';
  ddata->data_multi_artist=FALSE;
  ddata->data_year=0;

  for(index=0;index<MAX_TRACKS;index++) {
    *(ddata->data_track[index].track_name)='\0';
    *(ddata->data_track[index].track_artist)='\0';
    *(ddata->data_track[index].track_extended)='\0';
  }

  g_snprintf(file,256,"%s/%08x",root_dir,ddata->data_id);
  if(stat(file,&st)==0) {
    cddb_data=fopen(file, "r");
  }
  else {
    for(genre=0;genre<12;genre++) {
      g_snprintf(file,256,"%s/%s/%08x",root_dir,CDDBGenre(genre),
	       ddata->data_id);
      
      if(stat(file,&st)==0) {
	cddb_data=fopen(file, "r");
	
	ddata->data_genre=genre;
	break;
      }
    }

    if(genre==12) return -1;
  }

  while(fgets(inbuf,512,cddb_data))
    CDDBProcessLine(inbuf,ddata,disc.disc_totaltracks);

  /* Both disc title and artist have been stuffed in the title field, so the
     need to be separated */

  CDDBParseTitle(ddata->data_title,ddata->data_title,ddata->data_artist,"/");

  fclose(cddb_data);
  
  return 0;
}

#if 0
static void CDDBWriteLine(char *header,int num,char *data,FILE *outfile)
{
  int len;
  char *offset;

  len=strlen(data);
  offset=data;

  for(;;) {
    if(len>80) {
      if(num==-1)
	fprintf(outfile,"%s=%.70s\n",header,offset);
      else fprintf(outfile,"%s%d=%.70s\n",header,num,offset);

      offset+=70;
      len-=70;
    }
    else {
      if(num==-1) fprintf(outfile,"%s=%s\n",header,offset);
      else fprintf(outfile,"%s%d=%s\n",header,num,offset);
      break;
    }
  }
}
#endif

/* Write to the local cache */
int CDDBWriteDiscData(cdrom_drive *drive, DiscData *ddata,FILE *outfile,
		      gboolean gripext)
{
#if 0
  FILE *cddb_data;
  int track;
  char root_dir[256],file[256];
  struct stat st;
  disc_info disc;
  
  //CDStat(cd_desc,&disc,TRUE);

  if(!outfile) {
    g_snprintf(root_dir,256,"%s/.cddb",getenv("HOME"));
    g_snprintf(file,256,"%s/%08x",root_dir,ddata->data_id);
  
    if(stat(root_dir,&st)<0) {
      if(errno != ENOENT) {
	//g_message("Stat error %d on %s\n",errno,root_dir);
	return -1;
      }
      else {
	//g_message("Creating directory %s\n",root_dir);
	mkdir(root_dir, 0755);
      }
    } else {
      if(!S_ISDIR(st.st_mode)) {
	//g_message("Error: %s exists, but is a file\n",root_dir);
	errno=ENOTDIR;
	return -1;
      }   
    }
      
    if((cddb_data=fopen(file,"w"))==NULL) {
      //g_message("Error: Unable to open %s for writing\n",file);
      return -1;
    }
  }
  else cddb_data=outfile;

  //fprintf(cddb_data,"# xmcd CD database file generated by %s %s\n", Program,Version);
  fprintf(cddb_data,"# xmcd CD database file generated by Loser 1.0\n");
  fputs("# \n",cddb_data);
  fputs("# Track frame offsets:\n",cddb_data);

  for (track = 0; track < disc.disc_totaltracks; track++)
    fprintf(cddb_data, "#       %d\n",disc.track[track].track_start);

  fputs("# \n",cddb_data);
  fprintf(cddb_data,"# Disc length: %d seconds\n",disc.disc_length.minutes *
	  60 + disc.disc_length.seconds);
  fputs("# \n",cddb_data);
  fprintf(cddb_data,"# Revision: %s\n",CDDA_CDDB_LEVEL);
  //fprintf(cddb_data,"# Submitted via: %s %s\n",Program,Version);
  fprintf(cddb_data,"# Submitted via: Loser 1.0\n");
  fputs("# \n",cddb_data);
  fprintf(cddb_data,"DISCID=%08x\n",ddata->data_id);

  fprintf(cddb_data,"DTITLE=%s / %s\n",
	  ddata->data_artist,ddata->data_title);

  if(gripext&&ddata->data_year)
    fprintf(cddb_data,"DYEAR=%d\n",ddata->data_year);

  if(gripext)
    fprintf(cddb_data,"DGENRE=%s\n",CDDBGenre(ddata->data_genre));

  for(track=0;track<disc.disc_totaltracks;track++) {
    if(gripext||!*(ddata->data_track[track].track_artist)) {
      fprintf(cddb_data,"TTITLE%d=%s\n",track,
	      ddata->data_track[track].track_name);
    }
    else {
      fprintf(cddb_data,"TTITLE%d=%s / %s\n",track,
	      ddata->data_track[track].track_name,
	      ddata->data_track[track].track_artist);
    }

    if(gripext&&*(ddata->data_track[track].track_artist))
      fprintf(cddb_data,"TARTIST%d=%s\n",track,
	      ddata->data_track[track].track_artist);
    
  }

  CDDBWriteLine("EXTD",-1,ddata->data_extended,cddb_data);
   
  for(track=0;track<disc.disc_totaltracks;track++)
    CDDBWriteLine("EXTT",track,
		  ddata->data_track[track].track_extended,cddb_data);
  
  if(outfile)
    fprintf(cddb_data,"PLAYORDER=\n");
  else {
    fprintf(cddb_data,"PLAYORDER=%s\n",ddata->data_playlist);
    fclose(cddb_data);
  }
#endif
  return 0;
}


gboolean
CDDBLookupDisc (CDDBServer *server, cdrom_drive *drive, DiscData *disc_data)
{
	CDDBHello hello;
	CDDBQuery query;
	CDDBEntry entry;
	gboolean success = FALSE;

	/*	if(server->use_proxy) {
		 g_message ("Querying %s (through %s:%d) for disc %02x.\n", server->name,
		   server->proxy->name, server->proxy->port, CDDBDiscid (drive)); 
	} else {
		g_message ("Querying %s for disc %02x.\n",server->name, CDDBDiscid (drive));
	}
	*/
	
	strncpy (hello.hello_program, "Loser", 256);
	strncpy (hello.hello_version, "1.0", 256);
	
	if (!CDDBDoQuery (drive, server, &hello, &query)) {
		g_message ("Query failed");
	} else {
		switch(query.query_match) {
			case MATCH_INEXACT:
			case MATCH_EXACT:
				/*g_message ("Match for \"%s / %s\"\nDownloading data...\n",
							query.query_list[0].list_artist,
							query.query_list[0].list_title);*/
				entry.entry_genre = query.query_list[0].list_genre;
      			entry.entry_id = query.query_list[0].list_id;
      			CDDBRead (drive, server, &hello, &entry, disc_data);
		
      			/* g_message ("Done\n"); */
      			success = TRUE;
		
      			/* if (CDDBWriteDiscData (drive, disc_data, NULL, TRUE) < 0) {
					printf ("Error saving disc data\n");
					} */
				break;
			
    		case MATCH_NOMATCH:
      			g_message ("No match\n");
      			break;
		}
	}
	return success;
}

/* Update a CD status structure... because operating system interfaces vary
   so does this function. */

int 
CDStat (int cd_desc, disc_info *disc, gboolean read_toc)
{
	struct cdrom_tochdr cdth;
	struct cdrom_tocentry cdte;  
	int readtracks,frame[MAX_TRACKS],pos;
	int retcode;

	retcode = ioctl(cd_desc, CDROM_DRIVE_STATUS, CDSL_CURRENT);
	/* g_message("Drive status is %d\n", retcode); */
	if (retcode < 0) {
		/* g_message("Drive doesn't support drive status check (assume CDS_NO_INFO)\n"); */
	} else if (retcode != CDS_DISC_OK && retcode != CDS_NO_INFO) {
		return -1;
	}

	disc->disc_present = 1;

	if (read_toc) {
		/* g_message ("Reading TOC"); */
		/* Read the Table Of Contents header */
		if(ioctl(cd_desc, CDROMREADTOCHDR, &cdth) < 0) {
			printf("Error: Failed to read disc contents\n");
			return -1;
		}
		disc->disc_totaltracks = cdth.cdth_trk1;
    
		/* Read the table of contents */
		for(readtracks = 0; readtracks <= disc->disc_totaltracks; readtracks++) {
			if(readtracks == disc->disc_totaltracks)	
				cdte.cdte_track = CDROM_LEADOUT;
			else
				cdte.cdte_track = readtracks + 1;
      
			cdte.cdte_format = CDROM_MSF;
			if(ioctl(cd_desc, CDROMREADTOCENTRY, &cdte) < 0) {
				printf("Error: Failed to read disc contents\n");
				return -1;
      	}
      
      disc->track[readtracks].track_pos.minutes = cdte.cdte_addr.msf.minute;
      disc->track[readtracks].track_pos.seconds = cdte.cdte_addr.msf.second;
      frame[readtracks] = cdte.cdte_addr.msf.frame;
    }
    
    for(readtracks = 0; readtracks <= disc->disc_totaltracks; readtracks++) {
      disc->track[readtracks].track_start=
	(disc->track[readtracks].track_pos.minutes * 60 +
	 disc->track[readtracks].track_pos.seconds) * 75 + frame[readtracks];
      
      if(readtracks > 0) {
	pos = (disc->track[readtracks].track_pos.minutes * 60 +
	       disc->track[readtracks].track_pos.seconds) -
	  (disc->track[readtracks - 1].track_pos.minutes * 60 +
	   disc->track[readtracks -1].track_pos.seconds);
	disc->track[readtracks - 1].track_length.minutes = pos / 60;
	disc->track[readtracks - 1].track_length.seconds = pos % 60;
      }
    }
    
    disc->disc_length.minutes=
      disc->track[disc->disc_totaltracks].track_pos.minutes;
    
    disc->disc_length.seconds=
      disc->track[disc->disc_totaltracks].track_pos.seconds;
  }
   
  disc->disc_track = 0;

  while(disc->disc_track < disc->disc_totaltracks &&
	disc->disc_frame >= disc->track[disc->disc_track].track_start)
    disc->disc_track++;

  pos=(disc->disc_frame - disc->track[disc->disc_track - 1].track_start) / 75;

  disc->track_time.minutes = pos / 60;
  disc->track_time.seconds = pos % 60;

  return 0;
}
