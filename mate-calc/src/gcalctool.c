/*  Copyright (c) 1987-2008 Sun Microsystems, Inc. All Rights Reserved.
 *  Copyright (c) 2008-2009 Robert Ancell
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

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "currency.h"
#include "unittest.h"
#include "math-window.h"
#include "mp-equation.h"

static GSettings *settings = NULL;

static MathWindow *window;

static void
version(const gchar *progname)
{
    /* NOTE: Is not translated so can be easily parsed */
    fprintf(stderr, "%1$s %2$s\n", progname, VERSION);
}


static void
solve(const char *equation)
{
    MPEquationOptions options;
    MPErrorCode error;
    MPNumber result;
    char result_str[1024];

    memset(&options, 0, sizeof(options));
    options.base = 10;
    options.wordlen = 32;
    options.angle_units = MP_DEGREES;

    error = mp_equation_parse(equation, &options, &result, NULL);
    if(error == PARSER_ERR_MP) {
        fprintf(stderr, "Error: %s\n", mp_get_error());
        exit(1);
    }
    else if(error != 0) {
        fprintf(stderr, "Error: %s\n", mp_error_code_to_string(error));
        exit(1);
    }
    else {
        mp_cast_to_string(&result, 10, 10, 9, 1, result_str, 1024);
        printf("%s\n", result_str);
        exit(0);
    }
}


static void
usage(const gchar *progname, gboolean show_application, gboolean show_gtk)
{
    fprintf(stderr,
            /* Description on how to use gcalctool displayed on command-line */
            _("Usage:\n"
              "  %s — Perform mathematical calculations"), progname);

    fprintf(stderr,
            "\n\n");

    fprintf(stderr,
            /* Description on gcalctool command-line help options displayed on command-line */
            _("Help Options:\n"
              "  -v, --version                   Show release version\n"
              "  -h, -?, --help                  Show help options\n"
              "  --help-all                      Show all help options\n"
              "  --help-gtk                      Show GTK+ options"));
    fprintf(stderr,
            "\n\n");

    if (show_gtk) {
        fprintf(stderr,
                /* Description on gcalctool command-line GTK+ options displayed on command-line */
                _("GTK+ Options:\n"
                  "  --class=CLASS                   Program class as used by the window manager\n"
                  "  --name=NAME                     Program name as used by the window manager\n"
                  "  --screen=SCREEN                 X screen to use\n"
                  "  --sync                          Make X calls synchronous\n"
                  "  --gtk-module=MODULES            Load additional GTK+ modules\n"
                  "  --g-fatal-warnings              Make all warnings fatal"));
        fprintf(stderr,
                "\n\n");
    }

    if (show_application) {
        fprintf(stderr,
                /* Description on gcalctool application options displayed on command-line */
                _("Application Options:\n"
                  "  -u, --unittest                  Perform unit tests\n"
                  "  -s, --solve <equation>          Solve the given equation"));
        fprintf(stderr,
                "\n\n");
    }
}


static void
get_options(int argc, char *argv[])
{
    int i;
    char *progname, *arg;

    progname = g_path_get_basename(argv[0]);

    for (i = 1; i < argc; i++) {
        arg = argv[i];

        if (strcmp(arg, "-v") == 0 ||
            strcmp(arg, "--version") == 0) {
            version(progname);
            exit(0);
        }
        else if (strcmp(arg, "-h") == 0 ||
                 strcmp(arg, "-?") == 0 ||
                 strcmp(arg, "--help") == 0) {
            usage(progname, TRUE, FALSE);
            exit(0);
        }
        else if (strcmp(arg, "--help-all") == 0) {
            usage(progname, TRUE, TRUE);
            exit(0);
        }
        else if (strcmp(arg, "--help-gtk") == 0) {
            usage(progname, FALSE, TRUE);
            exit(0);
        }
        else if (strcmp(arg, "-s") == 0 ||
            strcmp(arg, "--solve") == 0) {
            i++;
            if (i >= argc) {
                fprintf(stderr,
                        /* Error printed to stderr when user uses --solve argument without an equation */
                        _("Argument --solve requires an equation to solve"));
                fprintf(stderr, "\n");
                exit(1);
            }
            else
                solve(argv[i]);
        }
        else if (strcmp(arg, "-u") == 0 ||
            strcmp(arg, "--unittest") == 0) {
            unittest();
        }
        else {
            fprintf(stderr,
                    /* Error printed to stderr when user provides an unknown command-line argument */
                    _("Unknown argument '%s'"), arg);
            fprintf(stderr, "\n");
            usage(progname, TRUE, FALSE);
            exit(1);
        }
    }
}


static void
quit_cb(MathWindow *window)
{
    MathEquation *equation;
    MathButtons *buttons;

    equation = math_window_get_equation(window);
    buttons = math_window_get_buttons(window);

    g_settings_set_int(settings, "accuracy", math_equation_get_accuracy(equation));
    g_settings_set_int(settings, "word-size", math_equation_get_word_size(equation));
    g_settings_set_int(settings, "base", math_buttons_get_programming_base(buttons));
    g_settings_set_boolean(settings, "show-thousands", math_equation_get_show_thousands_separators(equation));
    g_settings_set_boolean(settings, "show-zeroes", math_equation_get_show_trailing_zeroes(equation));
    g_settings_set_enum(settings, "number-format", math_equation_get_number_format(equation));
    g_settings_set_enum(settings, "angle-units", math_equation_get_angle_units(equation));
    g_settings_set_enum(settings, "button-mode", math_buttons_get_mode(buttons));
    g_settings_set_string(settings, "source-currency", math_equation_get_source_currency(equation));
    g_settings_set_string(settings, "target-currency", math_equation_get_target_currency(equation));
    g_settings_sync();

    currency_free_resources();
    gtk_main_quit();
}


int
main(int argc, char **argv)
{
    MathEquation *equation;
    int accuracy = 9, word_size = 64, base = 10;
    gboolean show_tsep = FALSE, show_zeroes = FALSE;
    DisplayFormat number_format;
    MPAngleUnit angle_units;
    ButtonMode button_mode;
    gchar *source_currency, *target_currency;

    g_type_init();

    bindtextdomain(GETTEXT_PACKAGE, LOCALE_DIR);
    bind_textdomain_codeset(GETTEXT_PACKAGE, "UTF-8");
    textdomain(GETTEXT_PACKAGE);

    /* Seed random number generator. */
    srand48((long) time((time_t *) 0));

    get_options(argc, argv);

    settings = g_settings_new ("org.mate.gcalctool");
    accuracy = g_settings_get_int(settings, "accuracy");
    word_size = g_settings_get_int(settings, "word-size");
    base = g_settings_get_int(settings, "base");
    show_tsep = g_settings_get_boolean(settings, "show-thousands");
    show_zeroes = g_settings_get_boolean(settings, "show-zeroes");
    number_format = g_settings_get_enum(settings, "number-format");
    angle_units = g_settings_get_enum(settings, "angle-units");
    button_mode = g_settings_get_enum(settings, "button-mode");
    source_currency = g_settings_get_string(settings, "source-currency");
    target_currency = g_settings_get_string(settings, "target-currency");

    equation = math_equation_new();
    math_equation_set_accuracy(equation, accuracy);
    math_equation_set_word_size(equation, word_size);
    math_equation_set_show_thousands_separators(equation, show_tsep);
    math_equation_set_show_trailing_zeroes(equation, show_zeroes);
    math_equation_set_number_format(equation, number_format);
    math_equation_set_angle_units(equation, angle_units);
    math_equation_set_source_currency(equation, source_currency);
    math_equation_set_target_currency(equation, target_currency);
    g_free(source_currency);
    g_free(target_currency);

    gtk_init(&argc, &argv);

    window = math_window_new(equation);
    g_signal_connect(G_OBJECT(window), "quit", G_CALLBACK(quit_cb), NULL);
    math_buttons_set_programming_base(math_window_get_buttons(window), base);
    math_buttons_set_mode(math_window_get_buttons(window), button_mode); // FIXME: We load the basic buttons even if we immediately switch to the next type

    gtk_widget_show(GTK_WIDGET(window));
    gtk_main();

    return(0);
}
