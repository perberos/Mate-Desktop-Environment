#include "config.h"
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>

#ifdef _WIN32
#include <windows.h>
#endif

int totem_private_asprintf(char **out, const char *fmt, ...)
{
    va_list ap;
    int ret_status = EOF;
    char dir_name[2001];
    char file_name[2000];
    FILE *fp = NULL;
    char *work = NULL;

    va_start(ap, fmt);

    /* Warning: tmpfile() does not work well on Windows (MinGW)
     *          if user does not have write access on the drive where 
     *          working dir is? */
#ifdef _WIN32
    /* file_name = G_tempfile(); */
    GetTempPath ( 2000, dir_name );
    GetTempFileName ( dir_name, "asprintf", 0, file_name );
    fp = fopen ( file_name, "w+" );
#else
    fp = tmpfile(); 
#endif /* _WIN32 */

    if ( fp ) {
        int count;

        count = vfprintf(fp, fmt, ap);
        if (count >= 0) {
            work = calloc(count + 1, sizeof(char));
            if (work != NULL) {
                rewind(fp);
                ret_status = fread(work, sizeof(char), count, fp);
                if (ret_status != count) {
                    ret_status = EOF;
                    free(work);
                    work = NULL;
                }
            }
        }
        fclose(fp);
#ifdef _WIN32
        unlink ( file_name );
#endif /* _WIN32 */
    }
    va_end(ap);
    *out = work;

    return ret_status;
}

