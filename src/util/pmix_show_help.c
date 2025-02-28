/*
 * Copyright (c) 2004-2005 The Trustees of Indiana University and Indiana
 *                         University Research and Technology
 *                         Corporation.  All rights reserved.
 * Copyright (c) 2004-2006 The University of Tennessee and The University
 *                         of Tennessee Research Foundation.  All rights
 *                         reserved.
 * Copyright (c) 2004-2005 High Performance Computing Center Stuttgart,
 *                         University of Stuttgart.  All rights reserved.
 * Copyright (c) 2004-2005 The Regents of the University of California.
 *                         All rights reserved.
 * Copyright (c) 2008      Cisco Systems, Inc.  All rights reserved.
 * Copyright (c) 2015      Research Organization for Information Science
 *                         and Technology (RIST). All rights reserved.
 * Copyright (c) 2016-2020 Intel, Inc.  All rights reserved.
 * Copyright (c) 2021-2022 Nanook Consulting.  All rights reserved.
 * $COPYRIGHT$
 *
 * Additional copyrights may follow
 *
 * $HEADER$
 */

#include "src/include/pmix_config.h"

#include <errno.h>
#include <locale.h>
#include <stdio.h>
#include <string.h>

#include "pmix_common.h"
#include "src/mca/pinstalldirs/pinstalldirs.h"
#include "src/util/pmix_argv.h"
#include "src/util/pmix_os_path.h"
#include "src/util/pmix_output.h"
#include "src/util/pmix_printf.h"
#include "src/util/pmix_show_help.h"
#include "src/util/showhelp/showhelp_lex.h"

/*
 * Private variables
 */
static const char *default_filename = "help-messages";
static const char *dash_line
    = "--------------------------------------------------------------------------\n";
static int output_stream = -1;
static char **search_dirs = NULL;

/*
 * Local functions
 */
int pmix_show_help_init(char *helpdir)
{
    pmix_output_stream_t lds;

    PMIX_CONSTRUCT(&lds, pmix_output_stream_t);
    lds.lds_want_stderr = true;
    output_stream = pmix_output_open(&lds);

    pmix_argv_append_nosize(&search_dirs, pmix_pinstall_dirs.pmixdatadir);
    if(NULL != helpdir) {
        pmix_argv_append_nosize(&search_dirs, helpdir);
    }

    return PMIX_SUCCESS;
}

int pmix_show_help_finalize(void)
{
    pmix_output_close(output_stream);
    output_stream = -1;

    /* destruct the search list */
    if (NULL != search_dirs) {
        pmix_argv_free(search_dirs);
        search_dirs = NULL;
    };

    return PMIX_SUCCESS;
}

/*
 * Make one big string with all the lines.  This isn't the most
 * efficient method in the world, but we're going for clarity here --
 * not optimization.  :-)
 */
static int array2string(char **outstring, int want_error_header, char **lines)
{
    int i, count;
    size_t len;

    /* See how much space we need */

    len = want_error_header ? 2 * strlen(dash_line) : 0;
    count = pmix_argv_count(lines);
    for (i = 0; i < count; ++i) {
        if (NULL == lines[i]) {
            break;
        }
        len += strlen(lines[i]) + 1;
    }

    /* Malloc it out */

    (*outstring) = (char *) malloc(len + 1);
    if (NULL == *outstring) {
        return PMIX_ERR_OUT_OF_RESOURCE;
    }

    /* Fill the big string */

    *(*outstring) = '\0';
    if (want_error_header) {
        strcat(*outstring, dash_line);
    }
    for (i = 0; i < count; ++i) {
        if (NULL == lines[i]) {
            break;
        }
        strcat(*outstring, lines[i]);
        strcat(*outstring, "\n");
    }
    if (want_error_header) {
        strcat(*outstring, dash_line);
    }

    return PMIX_SUCCESS;
}

/*
 * Find the right file to open
 */
static int open_file(const char *base, const char *topic)
{
    char *filename;
    char *err_msg = NULL;
    size_t base_len;
    int i;

    /* If no filename was supplied, use the default */

    if (NULL == base) {
        base = default_filename;
    }

    /* if this is called prior to someone initializing the system,
     * then don't try to look
     */
    if (NULL != search_dirs) {
        /* Try to open the file.  If we can't find it, try it with a .txt
         * extension.
         */
        for (i = 0; NULL != search_dirs[i]; i++) {
            filename = pmix_os_path(false, search_dirs[i], base, NULL);
            pmix_showhelp_yyin = fopen(filename, "r");
            if (NULL == pmix_showhelp_yyin) {
                if (NULL != err_msg) {
                    free(err_msg);
                }
                if (0 > asprintf(&err_msg, "%s: %s", filename, strerror(errno))) {
                    free(filename);
                    return PMIX_ERR_OUT_OF_RESOURCE;
                }
                base_len = strlen(base);
                if (4 > base_len || 0 != strcmp(base + base_len - 4, ".txt")) {
                    free(filename);
                    if (0
                        > asprintf(&filename, "%s%s%s.txt", search_dirs[i], PMIX_PATH_SEP, base)) {
                        free(err_msg);
                        return PMIX_ERR_OUT_OF_RESOURCE;
                    }
                    pmix_showhelp_yyin = fopen(filename, "r");
                }
            }
            free(filename);
            if (NULL != pmix_showhelp_yyin) {
                break;
            }
        }
    }

    /* If we still couldn't open it, then something is wrong */
    if (NULL == pmix_showhelp_yyin) {
        pmix_output(output_stream,
                    "%sSorry!  You were supposed to get help about:\n    %s\nBut I couldn't open "
                    "the help file:\n    %s.  Sorry!\n%s",
                    dash_line, topic, err_msg, dash_line);
        free(err_msg);
        return PMIX_ERR_NOT_FOUND;
    }

    if (NULL != err_msg) {
        free(err_msg);
    }

    /* Set the buffer */

    pmix_showhelp_init_buffer(pmix_showhelp_yyin);

    /* Happiness */

    return PMIX_SUCCESS;
}

/*
 * In the file that has already been opened, find the topic that we're
 * supposed to output
 */
static int find_topic(const char *base, const char *topic)
{
    int token, ret;
    char *tmp;

    /* Examine every topic */

    while (1) {
        token = pmix_showhelp_yylex();
        switch (token) {
        case PMIX_SHOW_HELP_PARSE_TOPIC:
            tmp = strdup(pmix_showhelp_yytext);
            if (NULL == tmp) {
                return PMIX_ERR_OUT_OF_RESOURCE;
            }
            tmp[strlen(tmp) - 1] = '\0';
            ret = strcmp(tmp + 1, topic);
            free(tmp);
            if (0 == ret) {
                return PMIX_SUCCESS;
            }
            break;

        case PMIX_SHOW_HELP_PARSE_MESSAGE:
            break;

        case PMIX_SHOW_HELP_PARSE_DONE:
            pmix_output(output_stream,
                        "%sSorry!  You were supposed to get help about:\n    %s\nfrom the file:\n  "
                        "  %s\nBut I couldn't find that topic in the file.  Sorry!\n%s",
                        dash_line, topic, base, dash_line);
            return PMIX_ERR_NOT_FOUND;

        default:
            break;
        }
    }

    /* Never get here */
}

/*
 * We have an open file, and we're pointed at the right topic.  So
 * read in all the lines in the topic and make a list of them.
 */
static int read_topic(char ***array)
{
    int token, rc;

    while (1) {
        token = pmix_showhelp_yylex();
        switch (token) {
        case PMIX_SHOW_HELP_PARSE_MESSAGE:
            /* pmix_argv_append_nosize does strdup(pmix_show_help_yytext) */
            rc = pmix_argv_append_nosize(array, pmix_showhelp_yytext);
            if (rc != PMIX_SUCCESS) {
                return rc;
            }
            break;

        default:
            return PMIX_SUCCESS;
        }
    }

    /* Never get here */
}

static int load_array(char ***array, const char *filename, const char *topic)
{
    int ret;

    if (PMIX_SUCCESS != (ret = open_file(filename, topic))) {
        return ret;
    }

    ret = find_topic(filename, topic);
    if (PMIX_SUCCESS == ret) {
        ret = read_topic(array);
    }

    fclose(pmix_showhelp_yyin);
    pmix_showhelp_yylex_destroy();

    if (PMIX_SUCCESS != ret) {
        pmix_argv_free(*array);
    }

    return ret;
}

char *pmix_show_help_vstring(const char *filename, const char *topic, int want_error_header,
                             va_list arglist)
{
    int rc;
    char *single_string, *output, **array = NULL;

    /* Load the message */
    if (PMIX_SUCCESS != (rc = load_array(&array, filename, topic))) {
        return NULL;
    }

    /* Convert it to a single raw string */
    rc = array2string(&single_string, want_error_header, array);

    if (PMIX_SUCCESS == rc) {
        /* Apply the formatting to make the final output string */
        if (0 > vasprintf(&output, single_string, arglist)) {
            output = NULL;
        }
        free(single_string);
    }

    pmix_argv_free(array);
    return (PMIX_SUCCESS == rc) ? output : NULL;
}

char *pmix_show_help_string(const char *filename, const char *topic, int want_error_handler, ...)
{
    char *output;
    va_list arglist;

    va_start(arglist, want_error_handler);
    output = pmix_show_help_vstring(filename, topic, want_error_handler, arglist);
    va_end(arglist);

    return output;
}

int pmix_show_vhelp(const char *filename, const char *topic, int want_error_header, va_list arglist)
{
    char *output;

    /* Convert it to a single string */
    output = pmix_show_help_vstring(filename, topic, want_error_header, arglist);

    /* If we got a single string, output it with formatting */
    if (NULL != output) {
        pmix_output(output_stream, "%s", output);
        free(output);
    }

    return (NULL == output) ? PMIX_ERROR : PMIX_SUCCESS;
}

int pmix_show_help(const char *filename, const char *topic, int want_error_header, ...)
{
    va_list arglist;
    char *output;

    va_start(arglist, want_error_header);
    output = pmix_show_help_vstring(filename, topic, want_error_header, arglist);
    va_end(arglist);

    /* If nothing came back, there's nothing to do */
    if (NULL == output) {
        return PMIX_SUCCESS;
    }

    fprintf(stderr, "%s\n", output);
    free(output);
    return PMIX_SUCCESS;
}

int pmix_show_help_add_dir(const char *directory)
{
    pmix_argv_append_nosize(&search_dirs, directory);
    return PMIX_SUCCESS;
}
