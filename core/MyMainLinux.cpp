// Initialize library and handle sketch functions like we want to

#include <stdio.h>
#include <csignal>
#include <cstdlib>
#include <unistd.h>
#include <fcntl.h>
#include <syslog.h>
#include <errno.h>
#include "log.h"
#include "MySensorsCore.h"

void handle_sigint(int sig)
{
	if (sig == SIGINT) {
		mys_log(LOG_NOTICE, "Received SIGINT\n\n");
	} else if (sig == SIGTERM) {
		mys_log(LOG_NOTICE, "Received SIGTERM\n\n");
	} else {
		return;
	}

	#ifdef MY_RF24_IRQ_PIN
		detachInterrupt(MY_RF24_IRQ_PIN);
	#endif

	#if defined(MY_GATEWAY_SERIAL)
		MY_SERIALDEVICE.end();
	#endif

	exit(0);
}

static int daemonize(void)
{
	pid_t pid, sid;

	/* Fork off the parent process */
	pid = fork();
	if (pid < 0) {
		mys_log(LOG_ERR, "fork: %s", strerror(errno));
		return -1;
	}
	/* If we got a good PID, then we can exit the parent process. */
	if (pid > 0) {
		exit(EXIT_SUCCESS);
	}

	/* At this point we are executing as the child process */

	/* Change the file mode mask */
	umask(0);

	/* Create a new SID for the child process */
	sid = setsid();
	if (sid < 0) {
		mys_log(LOG_ERR, "setsid: %s", strerror(errno));
		return -1;
	}

	/* Change the current working directory.  This prevents the current
	directory from being locked; hence not being able to remove it. */
	if ((chdir("/")) < 0) {
		mys_log(LOG_ERR, "chdir(\"/\"): %s", strerror(errno));
		return -1;
	}

	freopen( "/dev/null", "r", stdin);
	freopen( "/dev/null", "r", stdout);
	freopen( "/dev/null", "r", stderr);

	return 0;
}

void print_usage()
{
	printf("Usage: mysGateway [options]\n\n" \
			 "Options:\n" \
			 "  -h            Display a short summary of all program options.\n" \
			 "  -d            Enable debug.\n" \
			 "  -b            Become a daemon.\n");
}

int main(int argc, char *argv[])
{
	int opt, log_opts, debug = 0, foreground = 1;

	/* register the signal handler */
	signal(SIGINT, handle_sigint);
	signal(SIGTERM, handle_sigint);

	while ((opt = getopt(argc, argv, "hdb")) != -1) {
		switch (opt) {
			case 'h':
				print_usage();
				exit(0);
			case 'd':
				debug = 1;
				break;
			case 'b':
				foreground = 0;
				break;
			default:
				print_usage();
				exit(0);
		}
	}

	log_opts = LOG_CONS;
	if (foreground && isatty(STDIN_FILENO)) {
		// Also print syslog to stderror
		log_opts |= LOG_PERROR;
	}
	if (!debug) {
		setlogmask(LOG_UPTO (LOG_INFO));
	}
	openlog(NULL, log_opts, LOG_USER);

	if (!foreground && !debug) {
		if (daemonize() != 0) {
			exit(EXIT_FAILURE);
		}
	}

	mys_log(LOG_INFO, "Starting gateway...\n");
	mys_log(LOG_INFO, "Protocol version - %s\n", MYSENSORS_LIBRARY_VERSION);

	_begin(); // Startup MySensors library

	for (;;) {
		_process();  // Process incoming data
		if (loop) loop(); // Call sketch loop
	}
	return 0;
}
