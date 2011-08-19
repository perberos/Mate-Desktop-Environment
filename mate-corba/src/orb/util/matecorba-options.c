#include <config.h>
#include <glib.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include <glib/gstdio.h>

#include "matecorba-options.h"

#ifdef G_OS_WIN32
extern const gchar *MateCORBA_win32_get_system_rcfile (void);

#undef MATECORBA_SYSTEM_RCFILE
#define MATECORBA_SYSTEM_RCFILE MateCORBA_win32_get_system_rcfile ()
#endif

#undef DEBUG

/*
 * MateCORBA_option_set:
 * @option: an #MateCORBA_option describing the option.
 * @val: a pointer to the option value string.
 *
 * Sets @option's arg member member to the appropiate form
 * of @val.
 *
 * If the option is an string option the string will be
 * duplicated, if the option if a 'none' option it will
 * be treated as a boolean option of value TRUE.
 */
static void
MateCORBA_option_set (const MateCORBA_option *option,
		  const gchar        *val)
{
	g_assert (option != NULL);

	if (!option->arg)
		return;

#ifdef DEBUG
	fprintf (stderr, "Setting option %s to %s\n", option->name, 
	                                              val ? val : "(none)" );
#endif

	switch (option->type) {
	case MATECORBA_OPTION_NONE:
		*(gboolean *)option->arg = TRUE;
		break;
	case MATECORBA_OPTION_BOOLEAN:
		*(gboolean *)option->arg = (gboolean)atoi (val);
		break;
	case MATECORBA_OPTION_INT:
		*(gint *)option->arg = atoi (val);	
		break;
	case MATECORBA_OPTION_ULONG:
		*(guint *)option->arg = strtoul(val, (char **)NULL, 10);	
		break;
	case MATECORBA_OPTION_STRING: {
		gchar **str_arg = (char **) option->arg;

		if (*str_arg)
			g_free (*str_arg);

		*str_arg = g_strdup (val);
		break;
	}
	case MATECORBA_OPTION_KEY_VALUE: {
		GSList **list = (GSList**) option->arg;

		/* split string into tuple */ 
		gchar **str_vec = g_strsplit (val, "=", 2);
		
		if (!str_vec || !str_vec[0] || !str_vec[1])
		{
			g_warning ("Option %s requieres key=value pair: %s", option->name, val);
			if (str_vec) g_strfreev (str_vec);
			break;
		}
		g_assert (str_vec[0] != NULL);
		g_assert (str_vec[1] != NULL);

		{
			MateCORBA_OptionKeyValue *tuple 
				= g_new0 (MateCORBA_OptionKeyValue, 1);

			tuple->key   = g_strdup (str_vec[0]);
			tuple->value = g_strdup (str_vec[1]);

			*list = g_slist_append (*list, tuple);
		}

		g_strfreev (str_vec);

		break;		
	}
	default:
		g_assert_not_reached ();
		break;
	}
}

/*
 * MateCORBA_option_rc_parse:
 * @rcfile: the path of the matecorbarc file.
 * @option_list: the #MateCORBA_options to parse from the file.
 *
 * Parses @rcfile for any of the options in @option_list. The syntax
 * of rcfile is simple : 'option=value'.
 *
 * Note: leading or trailing whitespace is allowed for both the option 
 *       and its value.
 */
static void
MateCORBA_option_rc_parse (const gchar         *rcfile,
		       const MateCORBA_option  *option_list)
{
	gchar  line [1024];
	FILE  *fh;

	fh = g_fopen (rcfile, "r");
	if (!fh)
		return;

#ifdef DEBUG
	fprintf (stderr, "Parsing file %s for options\n", rcfile);
#endif

	while (fgets (line, sizeof (line), fh)) {
		const MateCORBA_option  *option = NULL;
		gchar              **strvec;
		gchar               *key;
		gchar               *value;

		if (line [0] == '#')
			continue;

		strvec = g_strsplit (line, "=", 3);

		if (!strvec || !strvec[0] || !strvec[1])
			continue;

		key = g_strchomp (g_strchug (strvec[0]));

                for (option = option_list; option->name; option++)
			if (!strcmp (key, option->name))
				break;

		if (!option->name) {
			option = NULL;
			continue;
		}

		value = g_strchomp (g_strchug (strvec[1]));

		MateCORBA_option_set (option, value);

		g_strfreev (strvec);
	}

	fclose (fh);
}

/*
 * MateCORBA_option_command_line_parse:
 * @argc: main's @argc param.
 * @argv: main's @argv param.
 * @option_list: list of #MateCORBA_options to parse from @argv.
 *
 * Parse @argv looking for options contained in @option_list and
 * setting values as appropriate. Also strip these options from
 * @argv and adjust @argc.
 */
static void 
MateCORBA_option_command_line_parse (int                 *argc,
				 char               **argv,
				 const MateCORBA_option  *option_list)
{
	gboolean *erase;
	gint      i, j, numargs;
	gchar     name [1024];
	gchar    *tmpstr;
	const MateCORBA_option *option = NULL;

#ifdef DEBUG
	fprintf (stderr, "Parsing command line for options\n");
#endif

	if (!argc || !argv)
		return;

	erase = g_new0 (gboolean, *argc);

	for (i = 1, numargs = *argc; i < *argc; i++) {

		if (argv [i][0] != '-') {
			if (!option)
				continue;

			erase [i] = TRUE;
			numargs--;

			if (!option->arg) {
				option = NULL;
				continue;
			}

			MateCORBA_option_set (option, argv [i]);
			option = NULL;
			continue;

                } else if (option && option->type != MATECORBA_OPTION_NONE)
			g_warning ("Option %s requires an argument\n",
				   option->name);

                tmpstr = argv [i];
                while (*tmpstr && *tmpstr == '-')
			tmpstr++;
		
                strncpy (name, tmpstr, sizeof (name) - 1);
		name [sizeof (name) - 1] = '\0';

                tmpstr = strchr (name, '=');
                if (tmpstr)
                        *tmpstr++ = '\0';

                for (option = option_list; option->name; option++)
			if (!strcmp (name, option->name))
				break;

		if (!option->name) {
			option = NULL;
			continue;
		}
		
		erase [i] = TRUE;
		numargs--;

		if (option->type != MATECORBA_OPTION_NONE && tmpstr) {
			MateCORBA_option_set (option, tmpstr);
			option = NULL;
		}
	}

	/* erase all consumed arguments from @argv list */
        for (i = j = 1; i < *argc; i++) {
		if (erase [i])
                        continue;

		if (j < numargs)
			argv [j++] = argv [i];
 		else
			argv [j++] = "";
        }

        *argc = numargs;

        g_free (erase);
}

static gboolean no_sysrc  = FALSE;
static gboolean no_userrc = FALSE;

static MateCORBA_option matecorba_sysrc_options [] = {
        {"ORBNoSystemRC", MATECORBA_OPTION_NONE, &no_sysrc},
        {"ORBNoUserRC",   MATECORBA_OPTION_NONE, &no_userrc},
	{NULL,            0,                 NULL}
};

/*
 * MateCORBA_option_parse:
 * @argc: main's @argc param.
 * @argv: main's @argv param.
 * @option_list: list of #MateCORBA_options.
 *
 * First parses the command line - @argv - to check for matecorbarc related
 * options, then parses the relevant matecorbarc files and finally parse the
 * command line for all other ORB related options.
 *
 * All MateCORBA options are stripped from @argv and @argc is adjusted.
 *
 * Note: Command line arguments override matecorbarc options and ~/.matecorbarc
 *       overrides ${sysconfdir}/matecorbarc.
 */
void 
MateCORBA_option_parse (int                 *argc,
		    char               **argv,
		    const MateCORBA_option  *option_list)
{
	MateCORBA_option_command_line_parse (argc, argv, matecorba_sysrc_options);

	if (!no_sysrc)
		MateCORBA_option_rc_parse (MATECORBA_SYSTEM_RCFILE, option_list);

	if (!no_userrc) {
		gchar *rcfile;
		const gchar *home = g_get_home_dir ();

		if (home != NULL) {
			rcfile = g_strdup_printf ("%s" G_DIR_SEPARATOR_S "%s", home, MATECORBA_USER_RCFILE);
			MateCORBA_option_rc_parse (rcfile, option_list);
			g_free (rcfile);
		}
	}

	MateCORBA_option_command_line_parse (argc, argv, option_list);
}
