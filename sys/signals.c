#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <execinfo.h>
#include <sys/wait.h>
#include "types.h"
#include "logger/log.h"
#include "vmq/inc/vd_msg_queue.h"

C_CODE_BEGIN

static void show_trace(void)
{
	void *func[32];
	char **symbols;
	int32_t i, n;

	n = backtrace(func, numberof(func));
	symbols = backtrace_symbols(func, n);
	if (!symbols) {
		CRIT("Get trace symbols failed!");
		return;
	}

	CRIT("Total %d func symbols.", n);
	CRIT("%-16s%-16s%s", "index", "pid", "symbol");
	for (i = 0; i < n; ++i) {
		CRIT("%-16d%-16d%s", i, getpid(), symbols[i]);
	}
	free(symbols);
}

/**
 * free_all - free all source when process exit
 *
 * @author cyj (2016/3/30)
 */
static void free_all(void)
{
	ve_msg_del();
}

static void sigsegv(int dummy)
{
	CRIT("Caught SIGSEGV.");
	CRIT("Dumping core in /tmp");

	/* Put dump file to ramdisk */
	chdir("/tmp");

	/* Free resource */
	free_all();

	/* Set the next SIGSEGV to default handler,
	 * so when the next signal arrived, the program will exit immediately
	 */
	signal(dummy,SIG_DFL);

	show_trace();

	exit(1);
}

static void sighup(int dummy)
{
	(void)dummy;
	CRIT("Caught SIGHUP.");
}

static void sigterm(int dummy)
{
	(void)dummy;
	CRIT("Caught SIGTERM.");
	free_all();
	exit(1);
}

static void sigint(int dummy)
{
	(void)dummy;
	CRIT("Caught SIGINT.");
	free_all();
	exit(1);
}

static void sigabrt(int32_t dummy)
{
	(void)dummy;
	CRIT("Caught SIGABRT.");
	show_trace();
	free_all();
	exit(1);
}

static void sigttin(int dummy)
{
	CRIT("Caught SIGTTIN.");
	(void)dummy;
}

static void common_sig_handler(int sig)
{
	CRIT("Caught SIGNAL! sig=%d", sig);
}

void signals_init(void)
{
	struct sigaction sa;

	sa.sa_flags = 0;

	sigemptyset(&sa.sa_mask);
	sigaddset(&sa.sa_mask, SIGSEGV);
	sigaddset(&sa.sa_mask, SIGTERM);
	sigaddset(&sa.sa_mask, SIGABRT);
	sigaddset(&sa.sa_mask, SIGHUP);
	sigaddset(&sa.sa_mask, SIGINT);

	sa.sa_handler = sigsegv;
	sigaction(SIGSEGV, &sa, NULL);

	sa.sa_handler = sigabrt;
	sigaction(SIGABRT, &sa, NULL);

	sa.sa_handler = sigterm;
	sigaction(SIGTERM, &sa, NULL);

	sa.sa_handler = sigint;
	sigaction(SIGINT, &sa, NULL);

	sa.sa_handler = sighup;
	sigaction(SIGHUP, &sa, NULL);

	sa.sa_handler = SIG_IGN;
	sigaction(SIGPIPE, &sa, NULL);

	sa.sa_handler = sigttin;
	sigaction(SIGTTIN, &sa, NULL);

	int sig_ob[] =
	{
		//SIGHUP, /*1	 Hangup (POSIX).  */
		//SIGINT, //*2	 Interrupt (ANSI).  */
		SIGQUIT, //*3	 Quit (POSIX).  */
		SIGILL, //*	4	 Illegal instruction (ANSI).  */
		SIGTRAP, //*5	/* Trace trap (POSIX).  */
		//SIGABRT, //*6	/* Abort (ANSI).  */
		SIGIOT, //*6	/* IOT trap (4.2 BSD).  */
		SIGBUS, //*7	/* BUS error (4.2 BSD).  */
		SIGFPE, //*8	/* Floating-point exception (ANSI).  */
		SIGKILL, //*9	/* Kill, unblockable (POSIX).  */
		SIGUSR1, //*10	/* User-defined signal 1 (POSIX).  */
		//SIGSEGV, //*11	/* Segmentation violation (ANSI).  */
		SIGUSR2, //*12	/* User-defined signal 2 (POSIX).  */
		//SIGPIPE, //*13	/* Broken pipe (POSIX).  */
		SIGALRM, /*14	 Alarm clock (POSIX).  */
		//SIGTERM, /*15	/* Termination (ANSI).  */
		SIGSTKFLT, //*16	/* Stack fault.  */
		//SIGCHLD, //*17	/* Child status has changed (POSIX).  */
		SIGCONT, //*18	/* Continue (POSIX).  */
		SIGSTOP, //*19	/* Stop, unblockable (POSIX).  */
		SIGTSTP, //*20	/* Keyboard stop (POSIX).  */
		//SIGTTIN, //*21	/* Background read from tty (POSIX).  */
		SIGTTOU, //*22	/* Background write to tty (POSIX).  */
		SIGURG, //*23	/* Urgent condition on socket (4.2 BSD).  */
		SIGXCPU, //*24	/* CPU limit exceeded (4.2 BSD).  */
		SIGXFSZ, //*25	/* File size limit exceeded (4.2 BSD).  */
		SIGVTALRM, //*26	/* Virtual alarm clock (4.2 BSD).  */
		SIGPROF, //*27	/* Profiling alarm clock (4.2 BSD).  */
		SIGWINCH, //*28	/* Window size change (4.3 BSD, Sun).  */
		SIGIO, //*29	/* I/O now possible (4.2 BSD).  */
		SIGPWR, //*30	/* Power failure restart (System V).  */
		SIGSYS, //*31	/* Bad system call.  */
	};

	for (unsigned int i = 0; i < sizeof(sig_ob) / sizeof(int); i++)
	{
        sa.sa_handler = common_sig_handler;
		sigaction(sig_ob[i], &sa, NULL); //信号类型
	}
}


C_CODE_END
