/*****************************************************
 * Copyright Grégory Mounié 2008-2013                *
 *           Matthieu Moy 2008                       *
 *           Simon Nieuviarts 2002-2009              *
 * This code is distributed under the GLPv3 licence. *
 * Ce code est distribué sous la licence GPLv3+.     *
 *****************************************************/

#include <stdio.h>
#include <stddef.h>
#include <stdlib.h>
#include <errno.h>
#include <limits.h>
#include <string.h>
#include "readcmd.h"

#ifdef USE_GNU_READLINE
#include <readline/readline.h>
#include <readline/history.h>
#endif

static void memory_error(void)
{
    errno = ENOMEM;
    perror(0);
    exit(1);
}


static void *xmalloc(size_t size)
{
    void *p = malloc(size);
    if (!p) memory_error();
    return p;
}


static void *xrealloc(void *ptr, size_t size)
{
    void *p = realloc(ptr, size);
    if (!p) memory_error();
    return p;
}

#ifndef USE_GNU_READLINE
/* Read a line from standard input and put it in a char[] */
static char *readline(char *prompt)
{
    size_t buf_len = 16;
    char *buf = xmalloc(buf_len * sizeof(char));

    printf(prompt);
    if (fgets(buf, buf_len, stdin) == NULL) {
        free(buf);
        return NULL;
    }

    do {
        size_t l = strlen(buf);
        if ((l > 0) && (buf[l-1] == '\n')) {
            l--;
            buf[l] = 0;
            return buf;
        }
        if (buf_len >= (INT_MAX / 2)) memory_error();
        buf_len *= 2;
        buf = xrealloc(buf, buf_len * sizeof(char));
        if (fgets(buf + l, buf_len - l, stdin) == NULL) return buf;
    } while (1);
}
#endif

#define READ_CHAR *(*cur_buf)++ = *(*cur)++
#define SKIP_CHAR (*cur)++


static void read_word(char ** cur, char ** cur_buf) {
    while(1) {
        char c = **cur;
        switch (c) {
            case '\0':
            case ' ':
            case '\t':
                **cur_buf = '\0';
                return;
            case '\\':
                SKIP_CHAR;
                READ_CHAR;
                break;
            default:
                READ_CHAR;
                break;
        }
    }
}

/* Split the string in words, according to the simple grammar. */
static char **split_in_words(char *line)
{
    char *cur = line;
    char *buf = malloc(strlen(line) + 1);
    char *cur_buf;
    char **tab = 0;
    size_t l = 0;
    char c;

    while ((c = *cur) != 0) {
        char *w = 0;
        switch (c) {
            case ' ':
            case '\t':
                /* Ignore any whitespace */
                cur++;
                break;
            default:
                /* Another word */
                cur_buf = buf;
                read_word(&cur, &cur_buf);
                w = strdup(buf);
        }
        if (w) {
            tab = xrealloc(tab, (l + 1) * sizeof(char *));
            tab[l++] = w;
        }
    }
    tab = xrealloc(tab, (l + 1) * sizeof(char *));
    tab[l++] = 0;
    free(buf);
    return tab;
}


static void freeseq(char ***seq)
{
    int i, j;

    for (i=0; seq[i]!=0; i++) {
        char **cmd = seq[i];

        for (j=0; cmd[j]!=0; j++) free(cmd[j]);
        free(cmd);
    }
    free(seq);
}


/* Free the fields of the structure but not the structure itself */
static void freecmd(struct cmdline *s)
{
    if (s->in) free(s->in);
    if (s->out) free(s->out);
    if (s->seq) freeseq(s->seq);
}


struct cmdline *readcmd(char *prompt)
{
    static struct cmdline *static_cmdline = 0;
    struct cmdline *s = static_cmdline;
    char *line;
    char **words;
    int i;
    char *w;
    char **cmd;
    char ***seq;
    size_t cmd_len, seq_len;

    line = readline(prompt);
    if (line == NULL) {
        if (s) {
            freecmd(s);
            free(s);
        }
        return static_cmdline = 0;
    }
#ifdef USE_GNU_READLINE
    else 
        add_history(line);
#endif

    cmd = xmalloc(sizeof(char *));
    cmd[0] = 0;
    cmd_len = 0;
    seq = xmalloc(sizeof(char **));
    seq[0] = 0;
    seq_len = 0;

    words = split_in_words(line);
    free(line);

    if (!s)
        static_cmdline = s = xmalloc(sizeof(struct cmdline));
    else
        freecmd(s);
    s->err = 0;
    s->in = 0;
    s->out = 0;
    s->seq = 0;
    s->bg = 0;

    i = 0;
    while ((w = words[i++]) != 0) {
        cmd = xrealloc(cmd, (cmd_len + 2) * sizeof(char *));
        cmd[cmd_len++] = w;
        cmd[cmd_len] = 0;
    }

    if (cmd_len != 0) {
        seq = xrealloc(seq, (seq_len + 2) * sizeof(char **));
        seq[seq_len++] = cmd;
        seq[seq_len] = 0;
    } else if (seq_len != 0) {
        s->err = "misplaced pipe";
        i--;
        goto error;
    } else
        free(cmd);
    free(words);
    s->seq = seq;
    return s;
error:
    while ((w = words[i++]) != 0) {
        free(w);
    }
    free(words);
    freeseq(seq);
    for (i=0; cmd[i]!=0; i++) free(cmd[i]);
    free(cmd);
    if (s->in) {
        free(s->in);
        s->in = 0;
    }
    if (s->out) {
        free(s->out);
        s->out = 0;
    }
    return s;
}