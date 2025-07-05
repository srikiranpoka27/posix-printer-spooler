#include <signal.h>
#include <sys/wait.h>
#include <unistd.h>
#include <time.h>
#include "state.h"


static void sigchld_hdl(int sig) {
	(void)sig;
	sigchld_flag = 1;
}

// Installing SIGCHLD handler

void install_sig_handlers(void) {
	struct sigaction sa = {0};
	sa.sa_handler = sigchld_hdl;
	sa.sa_flags = SA_RESTART;
	sigemptyset(&sa.sa_mask);
	sigaction(SIGCHLD, &sa, NULL);
}

static void reap_children(void) {         // Reap all child status changes and update jobs, printers

	// Blocking SIGCHLD during cleanup to avoid race conditions
	sigset_t block, old;
	sigemptyset(&block);
	sigaddset(&block, SIGCHLD);
	sigprocmask(SIG_BLOCK, &block, &old);

	int status;
	pid_t pid;

	while ((pid = waitpid(-1, &status, WNOHANG | WUNTRACED | WCONTINUED)) > 0) {

		for (size_t i = 0; i<n_jobs; i++) {
			JOB *j = &jobs[i];

			if (getpgid(pid) != j->pgid) continue;

			PRINTER *p = j->printer;

			if (WIFSTOPPED(status)) {

				j->status = JOB_PAUSED;
				sf_job_status(j->id, JOB_PAUSED);

			} else if (WIFCONTINUED(status)) {

				j->status = JOB_RUNNING;
				sf_job_status(j->id, JOB_RUNNING);

			} else {

				j->finish_time = time(NULL);
				p->status = PRINTER_IDLE;
				sf_printer_status(p->name, PRINTER_IDLE);

				if (WIFEXITED(status) && WEXITSTATUS(status) == 0) {

					j->status = JOB_FINISHED;
					sf_job_status (j->id, JOB_FINISHED);
					sf_job_finished(j->id, status);


				} else {

					j->status = JOB_ABORTED;
					sf_job_status(j->id, JOB_ABORTED);
					sf_job_aborted(j->id, status);

				}
			}
		}
	}

	delete_old_jobs();
	try_dispatch();
	sigprocmask(SIG_SETMASK, &old, NULL); // Restoring signal mask
}

void sig_hook(void) {
	if (!sigchld_flag) return;

	sigchld_flag = 0;
	reap_children();
}