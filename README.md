# Unix-Based Printer Spooler in C

## Overview

This project implements a Unix-based printer spooler system in C. It is designed to simulate how real-world printer spooling works, with support for process handling, signal management, and I/O redirection using low-level POSIX system calls.

The spooler program, called `presi`, allows users to queue print jobs, define printer types, manage jobs and printers, and convert files using a dynamic pipeline of converters.

## Key Features

- Custom command-line interface (`presi>`) to interact with the system.
- Print job queuing and tracking with job states (created, running, paused, finished, aborted, deleted).
- Dynamic printer management: enable, disable, and monitor printer status.
- Type-based conversion pipelines for files (e.g., `txt -> pbm -> ascii`).
- Process creation using `fork()` and `execvp()`, with careful signal handling using `SIGCHLD`, `SIGSTOP`, `SIGCONT`, `SIGTERM`, etc.
- Full job scheduling and resource cleanup.
- Batch mode support using command files.
- Original code with extensive use of `dup`, `pipe`, and low-level I/O.
- Race-condition-free signal design using safe flag handling and `sigprocmask()`.

## Technologies Used

- C (ANSI C with POSIX extensions)
- Unix/Linux system calls (`fork`, `exec`, `dup`, `pipe`, `killpg`, `waitpid`, etc.)
- Signal handling (`SIGCHLD`, `SIGSTOP`, `SIGCONT`, `SIGTERM`)
- Custom shell-like input via `sf_readline()` (own implementation)

## Example Commands Supported

- `print filename.pdf`
- `cancel 3`
- `pause 4`
- `resume 4`
- `printer Alice ps`
- `conversion txt pbm pbmtext`
- `enable Alice`
- `disable Bob`
- `jobs`
- `printers`
- `quit`

## Project Structure

.
├── include/ # Header files (definitions, interfaces)
├── src/ # Source code (.c files)
├── tests/ # Unit tests
├── util/ # Printer simulation tools
├── spool/ # Output and log directory for virtual printers
└── Makefile # Build system

## How It Works

1. **Define file types** and printers.
2. **Register conversion pipelines** between types.
3. **Queue print jobs**, optionally assigning eligible printers.
4. System finds the shortest conversion pipeline and builds it dynamically.
5. Job is executed using a master process group and conversion subprocesses.
6. Job completion or failure is detected using `SIGCHLD` and logged.
7. Users can monitor or manage jobs in real-time.

## Sample Output

```bash
presi> printers
PRINTER: id=0, name=alice, type=ps, status=disabled

presi> jobs
JOB[0]: type=pdf, status=created, eligible=alice, file=foo.pdf
Known Limitations
Assumes valid file extensions.

Assumes all conversion tools are available on the system.

No GUI support (CLI only).

Future Improvements
Add support for prioritizing jobs.

GUI interface using ncurses or a web wrapper.

Persistent job queue via file system.