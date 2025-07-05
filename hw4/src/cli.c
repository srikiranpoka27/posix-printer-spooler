/*
 * Presi: Command-line interface
 */


#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include <stdint.h>
#include <signal.h>
#include <unistd.h>

#include "presi.h"
#include "state.h"
#include "sf_readline.h"

static void cli_init_once(void) {
    static int done = 0;
    if (done) return;
    state_init();
    install_sig_handlers();
    sf_set_readline_signal_hook(sig_hook);
    done = 1;
}

static void show_printers(FILE *out) {     // Function to show all available printers

    for (size_t i=0; i<n_printers; i++) {
        PRINTER *p = &printers[i];
        if (!p->name) continue;
        fprintf(out, "PRINTER %2d %-10s type=%-4s %s\n",
            p->id, p->name, p->type,
            (p->status == PRINTER_DISABLED ? "disabled" :
                p->status == PRINTER_IDLE ? "idle" : "busy"));
    }
}

static void show_jobs(FILE *out) {          // Function to show all the jobs
    for (size_t i=0; i<n_jobs; i++) {
        JOB *j = &jobs[i];
        if (!j->file_name || j->status == JOB_DELETED) continue;
        fprintf(out, "JOB[%2d] %-10s %s\n",
            j->id, job_status_names[j->status], j->file_name);
    }
}

static int type_cmd(int argc, char **argv) {        // Function to define a new filetype
    if (argc != 2 || add_type(argv[1]) < 0) return -1;
    return 0;
}

static int printer_cmd(int argc, char **argv) {      // Function to define a new printer
    if (argc != 3 || add_printer(argv[1], argv[2]) < 0) return -1;
    return 0;
}

static int conversion_cmd(int argc, char **argv) {    // Function for defining a new file conversion

    if (argc < 4) return -1;
    FILE_TYPE *from = lookup_type(argv[1]);
    FILE_TYPE *to = lookup_type(argv[2]);
    if (!from || !to) return -1;

    CONVERSION *c = define_conversion(argv[1], argv[2], &argv[3]);

    return c ? 0 : -1;
}

static int enable_disable_cmd(int enable, int argc, char **argv) {     // Function to enable/disable printer
    if (argc != 2) return -1;
    PRINTER *p = lookup_printer(argv[1]);
    if (!p) return -1;

    PRINTER_STATUS target = enable ? PRINTER_IDLE : PRINTER_DISABLED;
    if (p->status != target) {
        p->status = target;
        sf_printer_status(p->name, target);
        if (enable) try_dispatch();
    }

    return 0;
}

static int print_cmd(int argc, char **argv) {       // Function for assigning a print job

    if (argc < 2) return -1;

    FILE_TYPE *ft = infer_file_type(argv[1]);
    if (!ft) return -1;

    uint32_t eligible = (argc == 2) ? UINT32_MAX : 0;

    for (int i=2; i<argc; i++) {
        PRINTER *p = lookup_printer(argv[i]);
        if (!p) return -1;
        eligible |= (1u << p->id);
    }

    if (add_job(argv[1], ft->name, eligible) < 0) return -1;
    try_dispatch();
    return 0;
}

static int pause_resume_cancel_cmd(int kind, int argc, char **argv) {      // Function for pause/resume/cancel a job, (kind argument corresponds to the type of action)

    if (argc != 2) return -1;
    int id = atoi(argv[1]);
    JOB *j = lookup_job(id);
    if (!j) return -1;

    if (kind == 0 && j->status == JOB_RUNNING) {
        killpg(j->pgid, SIGSTOP);
        return 0;
    }

    if (kind == 1 && j->status == JOB_PAUSED) {
        killpg(j->pgid, SIGCONT);
        return 0;
    }

    if (kind == 2) {
        if (j->status == JOB_CREATED) {
            j->status = JOB_ABORTED;
            j->finish_time = time(NULL);
            sf_job_status(j->id, JOB_ABORTED);
            sf_job_aborted(j->id, 0);
            return 0;
        }

        if (j->status == JOB_RUNNING || j->status == JOB_PAUSED) {
            killpg(j->pgid, SIGTERM);
            if (j->status == JOB_PAUSED) killpg(j->pgid, SIGCONT);
            return 0;
        }
    }
    return -1;
}

int run_cli(FILE *in, FILE *out)
{
    cli_init_once();

    int interactive = (in == stdin);
    char *line;

    while ((line = sf_readline(interactive ? "presi> " : ""))) {
        char *argv[32];
        int argc = 0;
        for (char *tok = strtok(line, " \t\n"); tok && argc < 32; tok = strtok(NULL, " \t\n"))
            argv[argc++] = tok;
        int rc = 0;
        if (argc == 0) {
            free(line);
            continue;

        } else if (!strcmp(argv[0], "help")) {
            fprintf(out,
                "Commands:\n"
                "help quit\n"
                "type printer conversion\n"
                "printers jobs\n"
                "print [pause, resume, cancel] [enable, disable]\n");
        } else if (!strcmp(argv[0], "quit")) {
            free(line);
            sf_cmd_ok();
            return -1;
        }
        else if (!strcmp(argv[0], "type")) rc = type_cmd(argc, argv);
        else if (!strcmp(argv[0], "printer")) rc = printer_cmd(argc, argv);
        else if (!strcmp(argv[0], "conversion")) rc = conversion_cmd(argc, argv);
        else if (!strcmp(argv[0], "printers")) show_printers(out);
        else if (!strcmp(argv[0], "jobs")) show_jobs(out);
        else if (!strcmp(argv[0], "print")) rc = print_cmd(argc, argv);
        else if (!strcmp(argv[0], "pause")) rc = pause_resume_cancel_cmd(0, argc, argv);
        else if (!strcmp(argv[0], "resume")) rc = pause_resume_cancel_cmd(1, argc, argv);
        else if (!strcmp(argv[0], "cancel")) rc = pause_resume_cancel_cmd(2, argc, argv);
        else if (!strcmp(argv[0], "enable")) rc = enable_disable_cmd(1, argc, argv);
        else if (!strcmp(argv[0], "disable")) rc = enable_disable_cmd(0, argc, argv);
        else rc = -1;

        if (rc == 0) sf_cmd_ok();
        else sf_cmd_error("bad command");

        free(line);

    }
    return (in == stdin) ? -1 : 0;
}
