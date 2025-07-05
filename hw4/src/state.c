#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <time.h>
#include "state.h"

int initialised=0;

size_t n_types;
FILE_TYPE *types [MAX_TYPES];
PRINTER printers[MAX_PRINTERS];
size_t n_printers;
JOB jobs[MAX_JOBS];
size_t n_jobs;
int next_job_id;

sig_atomic_t sigchld_flag=0;

static time_t now(void) {
	return time(NULL);
}

void state_init(void) {

	if (initialised) return;
	conversions_init();
	memset(types, 0, sizeof(types));
	memset(printers, 0, sizeof(printers));
	memset(jobs, 0, sizeof(jobs));
	next_job_id = 0;
	initialised = 1;
}

// Helper functions for reading and writing into type and printer arrays

FILE_TYPE *lookup_type(const char *name) {

	for (size_t i=0; i<n_types; i++) {
		if (strcmp(types[i]->name, name) == 0) return types[i];
	}

	return NULL;
}

int add_type(const char *name) {
	if (lookup_type(name) || n_types == MAX_TYPES) return -1;
	FILE_TYPE *t = define_type((char *)name);
	if (!t) return -1;
	types[n_types++] = t;
	return 0;
}

PRINTER *lookup_printer(const char *name) {
	for (size_t i=0; i<n_printers; i++) {
		if (printers[i].name && strcmp(printers[i].name, name) == 0) return &printers[i];
	}

	return NULL;
}

int add_printer(const char *name, const char *type) {
	if (lookup_printer(name) || n_printers == MAX_PRINTERS) return -1;
	PRINTER *p = &printers[n_printers++];
	memset(p, 0, sizeof(*p));

	p->id = n_printers-1;
	p->name = strdup(name);
	p->type = strdup(type);
	p->status = PRINTER_DISABLED;
	sf_printer_defined(p->name, p->type);
	return 0;
}

// Helper functions for getting a free slot, reading/writing into job array
static int get_free_slot(void) {
	for (size_t i=0; i<MAX_JOBS; i++) {
		if (jobs[i].file_name == NULL || jobs[i].status == JOB_DELETED) return (int)i;
	}

	return -1;
}

JOB *lookup_job (int id) {
	for (size_t i=0; i<n_jobs; i++) {
		if (jobs[i].id == id && jobs[i].status != JOB_DELETED) return &jobs[i];
	}

	return NULL;
}

int add_job(const char *file, const char *type, uint32_t eligible) {
	int slot = get_free_slot();
	if (slot < 0) return -1;
	JOB *j = &jobs[slot];
	memset(j, 0, sizeof(*j));

	j->id = next_job_id++;
	j->file_name = strdup(file);
	j->file_type = strdup(type);
	j->eligible = eligible;
	j->status = JOB_CREATED;
	j->creation_time = now();

	if ((size_t)(slot+1) > n_jobs) n_jobs = slot+1;

	sf_job_created(j->id, j->file_name, j->file_type);
	return j->id;
}

void delete_old_jobs(void) {
	time_t t = now();

	for (size_t i=0; i<n_jobs; i++) {
		JOB *j = &jobs[i];
		if ((j->status == JOB_FINISHED || j->status == JOB_ABORTED) && t - j->finish_time>=10) {
			j->status = JOB_DELETED;
			sf_job_deleted(j->id);
		}
	}
}

// Helper function to build a command list for a given path of conversion

static char **build_cmd_list(CONVERSION **path) {
	if (!path || !path[0]) {
		char **c = malloc(2*sizeof(char*));
		c[0] = "cat";
		c[1] = NULL;
		return c;
	}

	size_t n = 0;
	while (path[n]) ++n;

	char **c = malloc((n+1)*sizeof(char*));
	for (size_t i=0;i<n;i++)
		c[i] = path[i]->cmd_and_args[0];
	c[n] = NULL;
	return c;
}

// Using master process for conversion pipeline

static void master_wait_loop(void) {
	int status, rc = 0;
	while (wait(&status)>0) {  // Reaping all the child processes of multi-step conversion
		if (WIFSIGNALED(status) || (WIFEXITED(status) && WEXITSTATUS(status) != 0)) rc = 1; // Detect any failure
	}
	_exit(rc);
}

static void build_and_exec_pipeline(JOB *j, PRINTER *p, CONVERSION **path) {
	int fd_file = open(j->file_name, O_RDONLY);
	int fd_prn = presi_connect_to_printer(p->name, p->type, PRINTER_NORMAL);

	if (fd_file<0 || fd_prn<0) {
		if (fd_file >= 0) close(fd_file);
		if (fd_prn >= 0) close(fd_prn);
		j->status = JOB_ABORTED;
		j->finish_time = now();
		sf_job_status(j->id, JOB_ABORTED);
		sf_job_aborted(j->id, 1);
		return;
	}

	if (path == NULL) {                // Fastening the process of executing the job when no type conversion is required
		j->status = JOB_RUNNING;
		sf_printer_status(p->name, PRINTER_BUSY);
		char *cmds[] = {"cat", NULL};
		sf_job_started(j->id, p->name, 0, cmds);

		char buf[4096];
		ssize_t n;
		while ((n = read(fd_file, buf, sizeof(buf))) > 0) {
			write(fd_prn, buf, n);
		}

		close(fd_file);
		close(fd_prn);

		p->status = PRINTER_IDLE;
		sf_printer_status(p->name, PRINTER_IDLE);

		j->status = JOB_FINISHED;
		j->finish_time = now();
		sf_job_status(j->id, JOB_FINISHED);
		sf_job_finished(j->id, 0);

		try_dispatch();
		return;
	}

	pid_t m = fork();

	if (m<0) {
		close(fd_file);
		close(fd_prn);
		return;
	}

	if (m==0) {
		setpgid(0,0);

		if (!path || !path[0]) {
			pid_t c = fork();
			if (c == 0) {
				dup2(fd_file, STDIN_FILENO);
				dup2(fd_prn, STDOUT_FILENO);
				execvp("cat", (char *[]){"cat", NULL});
				_exit(127);
			}
		} else {
			int in_fd = fd_file;

			for (size_t idx=0; path[idx]; idx++) {
				int fds[2] = {-1, -1};
				int last = path[idx+1] == NULL;
				if (!last && pipe(fds) == -1) _exit(127);

				pid_t c = fork();
				if (c==0) {
					dup2(in_fd, STDIN_FILENO);
					dup2(last ? fd_prn : fds[1], STDOUT_FILENO);
					if (in_fd != fd_file) close(in_fd);
					if (!last) {
						close(fds[0]);
						close(fds[1]);
					}
					execvp(path[idx]->cmd_and_args[0], path[idx]->cmd_and_args);
					_exit(127);
				}
				if (!last) close(fds[1]);
				if (in_fd != fd_file) close(in_fd);
				in_fd = last ? -1 : fds[0];
			}
			if (in_fd != -1 && in_fd != fd_file) close(in_fd);
		}

		close(fd_file);
		close(fd_prn);
		master_wait_loop();
	}

	setpgid(m, m);
	j->pgid = m;
	j->printer = p;
	j->status = JOB_RUNNING;
	j->start_time = now();

	p->status = PRINTER_BUSY;
	p->pgid = m;
	sf_printer_status(p->name, PRINTER_BUSY);

	char **cmds = build_cmd_list(path);
	sf_job_status (j->id, JOB_RUNNING);
	sf_job_started(j->id, p->name, (int)m, cmds);

	free(cmds);
}

void try_dispatch(void) {
	for (size_t ji=0; ji<n_jobs; ji++) {
		JOB *j = &jobs[ji];

		if (j->status != JOB_CREATED) continue;

		for (size_t pi=0; pi<n_printers; pi++) {
			PRINTER *p = &printers[pi];

			if (p->status != PRINTER_IDLE) continue;
			if (!(j->eligible & (1u<<p->id))) continue;

			FILE_TYPE *from = lookup_type(j->file_type);
			FILE_TYPE *to = lookup_type(p->type);
			if (!from || !to) continue;

			CONVERSION **path = NULL;
			if (strcmp(j->file_type, p->type) != 0) {
				path = find_conversion_path(j->file_type, p->type);
				if (!path) continue;
			}

			build_and_exec_pipeline(j, p, path);
			if (path) free(path);
			return;
		}
	}
}