/*****************************************************************

  cdda-cddb.h

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

*****************************************************************/

#ifndef CDDA_CDDB_H
#define CDDA_CDDB_H

#include <glib.h>
#include <stdio.h>

#define size16 short
#define size32 int

#include <cdda_interface.h>

#define CURRENT_CDDBREVISION		2
#define MAX_TRACKS					100

/* CDDB hello structure */
struct CDDBHello {
   /* Program */
   char hello_program[256];
   /* Program version */
   char hello_version[256];
};

/* Used for keeping track of times */
struct disc_timeval {
   int minutes;
   int seconds;
};

/* Track specific information */
struct track_info {
  struct disc_timeval track_length;
  struct disc_timeval track_pos;
  int track_frames;
  int track_start;
};

/* Disc information such as current track, amount played, etc */
typedef struct {
   	int disc_present;				/* Is disc present? */
   	int disc_mode;				/* Current disc mode */
   	struct disc_timeval track_time;		/* Current track time */
   	struct disc_timeval disc_time;		/* Current disc time */
   	struct disc_timeval disc_length;		/* Total disc length */
	int disc_frame;				/* Current frame */
   	int disc_track;				/* Current track */
   	int disc_totaltracks;			/* Number of tracks on disc */
  	struct track_info track[MAX_TRACKS];		/* Track specific information */
} disc_info;

/* HTTP proxy server structure */
typedef struct _proxy_server {
  char name[256];
  int port;
} ProxyServer;

/* CDDB server structure */

typedef struct _cddb_server {
	char name[256];
	char cgi_prog[256];
	int port;
	int use_proxy;
	ProxyServer *proxy;
} CDDBServer;

#define CDDA_CDDB_LEVEL "3"  /* Current CDDB protocol level supported */

/* CDDB entry */
typedef struct _cddb_entry {
   unsigned int entry_id;
   int entry_genre;
} CDDBEntry;

/* CDDB hello structure */
typedef struct _cddb_hello {
   /* Program */
   char hello_program[256];
   /* Program version */
   char hello_version[256];
} CDDBHello;

#define MAX_INEXACT_MATCHES			16

/* An entry in the query list */
struct query_list_entry {
   int list_genre;
   int list_id;
   char list_title[64];
   char list_artist[64];
};

/* CDDB query structure */
typedef struct _cddb_query {
   int query_match;
   int query_matches;
   struct query_list_entry query_list[MAX_INEXACT_MATCHES];
} CDDBQuery;

/* Match values returned by a query */

#define MATCH_NOMATCH	 0
#define MATCH_EXACT	 1
#define MATCH_INEXACT	 2

/* Track database structure */

typedef struct _track_data {
	char track_name[256];	    	/* Track name */
	char track_artist[256];	      	/* Track artist */
	char track_extended[4096];	  	/* Extended information */
} TrackData;

/* Disc database structure */
typedef struct _disc_data {
	unsigned int data_id;				/* CD id */
	char data_title[256];	          	/* Disc title */
	char data_artist[256];	      		/* We may be able to extract this */
	char data_extended[4096];	      	/* Extended information */
	int data_genre;		      			/* Disc genre */
	int data_year;                      /* Disc year */
	char data_playlist[256];            /* Playlist info */
	gboolean data_multi_artist;         /* Is CD multi-artist? */
	TrackData data_track[MAX_TRACKS];   /* Track names */
} DiscData;


/* Encode list structure */
typedef struct _encode_track {
	int track_num;
	int start_frame;
	int end_frame;
	char song_name[80];
	char song_artist[80];
	char disc_name[80];
	char disc_artist[80];
	int song_year;
	int id3_genre;
	int mins;
	int secs;
	int discid;
} EncodeTrack;


unsigned int CDDBDiscid(cdrom_drive *drive);
char *CDDBGenre(int genre);
int CDDBGenreValue(char *genre);
gboolean CDDBDoQuery(cdrom_drive *cd_desc, CDDBServer *server, CDDBHello *hello,CDDBQuery *query);
gboolean CDDBRead(cdrom_drive *cd_desc,CDDBServer *server, CDDBHello *hello,CDDBEntry *entry, DiscData *data);
gboolean CDDBRead(cdrom_drive *cd_desc,CDDBServer *server, CDDBHello *hello,CDDBEntry *entry, DiscData *data);
gboolean CDDBStatDiscData(cdrom_drive *cd_desc);
int CDDBReadDiscData(cdrom_drive *cd_desc, DiscData *outdata);
int CDDBWriteDiscData(cdrom_drive *drive, DiscData *ddata,FILE *outfile, gboolean gripext);
void CDDBParseTitle(char *buf,char *title,char *artist,char *sep);
char *ChopWhite(char *buf);
gboolean CDDBLookupDisc (CDDBServer *server, cdrom_drive *drive, DiscData *disc_data);
int CDStat(int cd_desc, disc_info *disc, gboolean read_toc);

#endif
