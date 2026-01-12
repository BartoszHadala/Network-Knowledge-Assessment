#ifndef DEAMON_INIT_H
#define DEAMON_INIT_H

/**
 * Daemon initialization helper
 *
 * Perform a conventional UNIX daemonization sequence and basic process setup
 * for services that need to run in the background. The implementation in
 * `src/deamon_init.c` performs the following actions:
 *   - double-fork to detach from the controlling terminal
 *   - call `setsid()` to become a session leader
 *   - ignore `SIGHUP` in the intermediate process
 *   - change working directory to `/tmp`
 *   - close all file descriptors in range `[0, MAXFD)`, except the optional
 *     `socket` descriptor (useful for keeping a listening socket open)
 *   - redirect `stdin`, `stdout`, `stderr` to `/dev/null`
 *   - open the syslog with the provided program name and facility
 *   - change the process UID via `setuid(uid)`
 *
 * Example usage:
 * @code
 *     int listenfd = setup_listening_socket(...);
 *     if (daemon_init("mydaemon", LOG_DAEMON, some_uid, listenfd) < 0) {
 *         perror("daemon_init");
 *         exit(EXIT_FAILURE);
 *     }
 * @endcode
 *
 * Notes:
 * - Pass `-1` for `socket` if there is no descriptor to preserve.
 * - The caller must ensure appropriate privileges for `setuid()`.
 * - After daemonization, use `syslog()` for logging since stdio is redirected.
 *
 * @param pname Program name used for syslog
 * @param facility Syslog facility (e.g., `LOG_DAEMON`, `LOG_USER`)
 * @param uid UID to switch to (set to `0` to remain as root if desired)
 * @param socket File descriptor to keep open (or `-1` to close all fds)
 * @return 0 on success, -1 on failure
 */

#include <sys/types.h>
#include <sys/stat.h>
#include <syslog.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <fcntl.h>
#include <string.h>
#include <errno.h>

#define MAXFD 64

int daemon_init(const char *pname, int facility, uid_t uid, int socket);

#endif // DEAMON_INIT_H
