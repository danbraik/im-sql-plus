/*****************************************************
 * Copyright Grégory Mounié 2008-2013                *
 *           Simon Nieuviarts 2002-2009              *
 * This code is distributed under the GLPv3 licence. *
 * Ce code est distribué sous la licence GPLv3+.     *
 *****************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <unistd.h>
#include <sys/stat.h>
#include <linux/stat.h>

#include "readcmd.h"
#include "common.h"

#define FIFO_FILE       "/tmp/MYFIFO"

int main()
{
    FILE *fifo;

    /* Create the FIFO if it does not exist */
    umask(0);
    mkfifo(FIFO_FILE, S_IWUSR | S_IRGRP | S_IROTH);

    printf("Waiting a reader on '%s'\n", FIFO_FILE);

    fifo = fopen(FIFO_FILE, "w");

    while (1) {
        struct cmdline *l;
        char *prompt = "my-sql-plus>";

        l = readcmd(prompt);

        /* If input stream closed, normal termination */
        if (!l) {
            printf("exit\n");
            exit(0);
        }

        if (l->err) {
            /* Syntax error, read another command */
            fprintf(stderr, "error: %s\n", l->err);
            continue;
        }

#ifdef DEBUG
        if (l->in) printf("in: %s\n", l->in);
        if (l->out) printf("out: %s\n", l->out);
        if (l->bg) printf("background (&)\n");

        /* Display each command of the pipe */
        for (int i=0; l->seq[i]!=0; i++) {
            char **cmd = l->seq[i];
            printf("seq[%d]: ", i);
            for (int j=0; cmd[j]!=0; j++) {
                    printf("'%s' ", cmd[j]);
            }
            printf("\n");
        }
#endif

        // set action at the reception of SIGCHLD

        if (l->seq[0] != NULL) {

            for (int i=0; l->seq[i]!=0; i++) {
                char **cmd = l->seq[i];
                for (int j=0; cmd[j]!=0; j++) {
                   fprintf(fifo, "%s ", cmd[j]);
                }
                fprintf(fifo, "\n");
                fflush(fifo);
            }
        }
    }

    fclose(fifo);

    return EXIT_SUCCESS;
}
