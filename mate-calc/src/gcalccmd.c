/*  $Header$
 *
 *  Copyright (c) 2009 Rich Burridge
 *           
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2, or (at your option)
 *  any later version.
 *           
 *  This program is distributed in the hope that it will be useful, but 
 *  WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU 
 *  General Public License for more details.
 *           
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA
 *  02111-1307, USA.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <time.h>

#include "mp-equation.h"

#define MAXLINE 1024

static void
solve(const char *equation)
{
    int ret;
    MPEquationOptions options;
    MPNumber z;
    char result_str[MAXLINE];
    
    memset(&options, 0, sizeof(options));
    options.base = 10;
    options.wordlen = 32;
    options.angle_units = MP_DEGREES;
    
    ret = mp_equation_parse(equation, &options, &z, NULL);

    if (ret == PARSER_ERR_MP)
        fprintf(stderr, "Error %s\n", mp_get_error());
    else if (ret)        
        fprintf(stderr, "Error %d\n", ret);
    else {
        mp_cast_to_string(&z, 10, 10, 9, 1, result_str, MAXLINE);
        printf("%s\n", result_str);
    }
}


/* Adjust user input equation string before solving it. */
static void
str_adjust(char *str)
{
    int i, j = 0;

    str[strlen(str)-1] = '\0';        /* Remove newline at end of string. */
    for (i = 0; str[i] != '\0'; i++) {        /* Remove whitespace. */
        if (str[i] != ' ' && str[i] != '\t')
            str[j++] = str[i];
    }
    str[j] = '\0';
    if (j > 0 && str[j-1] == '=')     /* Remove trailing '=' (if present). */
        str[j-1] = '\0';
}

int
main(int argc, char **argv)
{
    char *equation, *line;
    size_t nbytes = MAXLINE;

    /* Seed random number generator. */
    srand48((long) time((time_t *) 0));

    equation = (char *) malloc(MAXLINE * sizeof(char));
    while (1) {
        printf("> ");
        line = fgets(equation, nbytes, stdin);

        if (line != NULL)
            str_adjust(equation);

        if (line == NULL || strcmp(equation, "exit") == 0 || strcmp(equation, "quit") == 0 || strlen(equation) == 0)
            break;

        solve(equation);
    }
    free(equation);

    return 0;
}
