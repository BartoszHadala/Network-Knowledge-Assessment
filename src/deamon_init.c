#include "deamon_init.h"

int daemon_init(const char *pname, int facility, uid_t uid, int socket) {

	int		i, p;
	pid_t	pid;

	if ( (pid = fork()) < 0)
		return (-1);
	else if (pid)
		exit(0);			/* parent terminates */

	/* child 1 continues... */

	if (setsid() < 0)		/* become session leader */
		return (-1);

	// terminal closed, ignore
	signal(SIGHUP, SIG_IGN);

	if ( (pid = fork()) < 0)
		return (-1);
	else if (pid)
		exit(0);			/* child 1 terminates */

	/* child 2 continues... */

	chdir("/tmp");				/* change working directory  or chroot()*/
	umask(0);					/* clear file mode creation mask */

	// close off file descriptors
	for (i = 0; i < MAXFD; i++){
		if(socket != i )
			close(i);
	}

	/* redirect stdin, stdout, and stderr to /dev/null */
	if ( (p = open("/dev/null", O_RDONLY)) != 0 ) {
		/* stdin should be fd 0, if not - something went wrong */
		return (-1);
	}
	if ( open("/dev/null", O_RDWR) != 1 ) {
		/* stdout should be fd 1 */
		return (-1);
	}
	if ( open("/dev/null", O_RDWR) != 2 ) {
		/* stderr should be fd 2 */
		return (-1);
	}

	openlog(pname, LOG_PID, facility);

	if (setuid(uid) < 0) { /* change user */
		syslog(LOG_ERR, "setuid(%d) failed: %s", uid, strerror(errno));
		return (-1);
	}
	
	return (0);				/* success */
}