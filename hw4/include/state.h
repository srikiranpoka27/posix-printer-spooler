#ifndef STATE_H
#define STATE_H

#include <stdint.h>
#include <stdio.h>
#include <time.h>
#include <sys/types.h>
#include <signal.h>
#include "presi.h"
#include "conversions.h"

struct printer {
	int id;
	char *name;
	char *type;
	PRINTER_STATUS status;
	pid_t pgid;
	void *other;
};

struct job {
	int id;
	char *file_name;
	char *file_type;
	uint32_t eligible;
	JOB_STATUS status;
	pid_t pgid;
	time_t creation_time;
	time_t start_time;
	time_t finish_time;
	struct printer *printer;
	void *other;
};

#define MAX_TYPES     32

extern sig_atomic_t sigchld_flag;

extern size_t n_types;
extern FILE_TYPE *types [MAX_TYPES];
extern PRINTER printers[MAX_PRINTERS];
extern size_t n_printers;
extern JOB jobs[MAX_JOBS];
extern size_t n_jobs;
extern int next_job_id;

void state_init(void);

FILE_TYPE *lookup_type(const char *name);
PRINTER *lookup_printer(const char *name);
JOB *lookup_job(int id);

void install_sig_handlers(void);

int add_type(const char *name);
int add_printer(const char *name, const char *type);
int add_job(const char *file, const char *type, uint32_t eligible);

void delete_old_jobs(void);

void try_dispatch(void);

void sig_hook(void);

#endif