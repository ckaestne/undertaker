#if ENABLE_FEATURE_REMOTE_LOG
#endif

#ifdef SYSLOGD_MARK
#endif

#if ENABLE_FEATURE_IPC_SYSLOG
#    if CONFIG_FEATURE_IPC_SYSLOG_BUFFER_SIZE < 4
#    endif
#else
#endif /* FEATURE_IPC_SYSLOG */

#ifdef SYSLOGD_WRLOCK
#endif

#if ENABLE_FEATURE_ROTATE_LOGFILE
#    ifdef SYSLOGD_WRLOCK
#    endif
#endif

#if ENABLE_FEATURE_REMOTE_LOG
#endif

#if ENABLE_FEATURE_SYSLOGD_DUP
#else
#endif

/*
 * check-name: coverage analysis for busybox syslogd
 * check-command: undertaker -v -j coverage -O stdout -m  busybox-syslogd.model $file | grep -v '^ENABLE'
 * check-output-start
I: Set configuration space regex to '^(ENABLE_|CONFIG_)[^ ]*$'
I: loaded rsf model for busybox-syslogd
I: Using busybox-syslogd as primary model
I: Entries in missingSet: 0
I: Found 3 assignments
I: In all assignments the following symbols are equally set
CONFIG_FEATURE_REMOTE_LOG=1
CONFIG_FEATURE_ROTATE_LOGFILE=1
CONFIG_SYSLOGD=1
I: All differences in the assignments
I: Config 0
CONFIG_FEATURE_IPC_SYSLOG=1
CONFIG_FEATURE_IPC_SYSLOG_BUFFER_SIZE=1
CONFIG_FEATURE_SYSLOGD_DUP=1
I: Config 1
CONFIG_FEATURE_IPC_SYSLOG=0
CONFIG_FEATURE_IPC_SYSLOG_BUFFER_SIZE=0
CONFIG_FEATURE_SYSLOGD_DUP=1
I: Config 2
CONFIG_FEATURE_IPC_SYSLOG=0
CONFIG_FEATURE_IPC_SYSLOG_BUFFER_SIZE=0
CONFIG_FEATURE_SYSLOGD_DUP=0
 * check-output-end
 */
