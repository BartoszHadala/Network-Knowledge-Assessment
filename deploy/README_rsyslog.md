Setup instructions for routing LOG_LOCAL0 to /var/log/mydaemon.log

1. Copy the rsyslog config (requires root):

   sudo cp deploy/rsyslog-mydaemon.conf /etc/rsyslog.d/99-mydaemon.conf

2. Create the log file and set permissions:

   sudo touch /var/log/mydaemon.log
   sudo chown syslog:adm /var/log/mydaemon.log
   sudo chmod 0640 /var/log/mydaemon.log

3. (Optional) Install the logrotate config (requires root):

   sudo cp deploy/logrotate-mydaemon /etc/logrotate.d/mydaemon

4. Restart rsyslog so the new rule takes effect:

   sudo systemctl restart rsyslog

5. Verify logging: in one terminal run:

   sudo tail -f /var/log/mydaemon.log

   In another terminal, generate a test log line (from any user):

   logger -p local0.info -t mydaemon "test message"

   You should see the message appear in /var/log/mydaemon.log.

Notes:
- The daemon uses openlog(argv[0], ...) as ident. If you want logs to appear under a specific tag (e.g. "mydaemon"), either run the daemon with that name or modify the daemon to call openlog("mydaemon", ...).
- If your system uses journald and rsyslog forwards journal messages, the logs may additionally appear in `journalctl`.
- `copytruncate` in logrotate avoids needing to restart the daemon when rotating logs. If your daemon can safely reopen its logfile, prefer `create` without `copytruncate` and send a signal (e.g. SIGHUP) to reopen.
