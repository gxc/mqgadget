#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdbool.h>
#include <pthread.h>
#include <libgen.h>
#include <limits.h>

#include "mqgadget.h"
#include "util.h"
/* TO-DO: add signal handler for SIGINT (Ctrl + c) sigaction(): set task_terminated to true */
char *program_invocation_short_name;
bool task_terminated = false;

/* show usage and exit */
void usage(int status)
{
	FILE *fp;
	if (status == EXIT_SUCCESS)
		fp = stdout;

	else
		fp = stderr;
	fprintf(fp, "\
Usage: %s  [ -s <seconds> ] [ -c <count> ] -m <QMGR> <QL> [ <QL2> ... ]\n\
      Show statuses of local queues including CURDEPTH, \
percent of MAXDEPTH, GET and PUT attributes.\n\
\n\
\t-s <seconds> : refresh interval (default 1 second)\n\
\t-c <count>   : times it will refresh (default 1, max %d)\n\
\t-m <QMGR>    : queue manager name (required and case-insensitive)\n\
\t   <QL>      : local queue name (at least one, case-insensitive)\n\
\t-h           : show this\n\
      by xcguo  Nov 2015\n\
\n\
", program_invocation_short_name, INT_MAX);
	exit(status);
}

int main(int argc, char *argv[])
{
	char qmgrname[MQ_Q_MGR_NAME_LENGTH];
	struct qstat *queues;	/* queue info array */
	struct qstat *qp;	/* `queues' walker */
	int interval;		/* refresh interval */
	int ncount;		/* refresh times */
	int nqueue;		/* number of queues to monitor */
	bool is_refreshable;
	int err;
	int i;
	int c;
	ncount = 1;
	interval = 1;
	memset(qmgrname, 0x00, MQ_Q_MGR_NAME_LENGTH);
	program_invocation_short_name = basename(argv[0]);
	while ((c = getopt(argc, argv, "m:s:c:h")) != -1) {
		switch (c) {
		case 'm':
			strncpy(qmgrname, optarg, MQ_Q_MGR_NAME_LENGTH);
			break;
		case 's':
			interval = parse_positive_int(optarg);
			break;
		case 'c':
			ncount = parse_positive_int(optarg);
			break;
		case 'h':
			usage(EXIT_SUCCESS);
			break;
		case '?':
			usage(EXIT_FAILURE);
			fprintf(stderr, "Show usage\n");
			break;
		default:
			abort();
		}
	}
	nqueue = argc - optind;
	if (*qmgrname == '\0' || nqueue < 1)
		usage(EXIT_FAILURE);
	queues = calloc((size_t) nqueue, sizeof(struct qstat));
	if (!queues)
		err_exit("Not enough memory available.\n");
	strupr(qmgrname, MQ_Q_MGR_NAME_LENGTH);
	for (qp = queues; optind < argc; qp++, optind++) {
		strncpy(qp->qmname, qmgrname, MQ_Q_MGR_NAME_LENGTH);
		strncpy(qp->qname, argv[optind], MQ_Q_NAME_LENGTH);
		strupr(qp->qname, MQ_Q_NAME_LENGTH);
	}
	for (qp = queues; qp < queues + nqueue; qp++) {
		err = pthread_create(&qp->thread, NULL, inq_q_stat, qp);
		if (err)
			err_exit("thread for %s creation error\n", qp->qname);
	}

	/* showcase */
	is_refreshable = ncount > 1 && (is_term("vt100")
					|| is_term("linux"));
	usleep(25000L);		/* wait 1/40 second for inquiring threads */
	for (i = 0; i < ncount; i++) {
		if (is_refreshable)
			puts("[1J");	/* clear screen */
		puts("QNAME       CURDEPTH  PERCENT   GET       PUT");
		puts("----------- --------- --------- --------- ---------");
		for (qp = queues; qp < queues + nqueue; qp++) {
			printf("%-12s%-10ld%-10.2f%-10s%-10s\n", qp->qname,
			       qp->curdepth,
			       qp->maxdepth ? qp->curdepth * 100.0f /
			       qp->maxdepth : 0.0f,
			       qp->getable ? "ENABLED" : "DISABLED",
			       qp->putable ? "ENABLED" : "DISABLED");
		}
		printf("Count: %ld of %d\n", i + 1L, ncount);
		if (i != ncount)
			sleep(interval);
	}
	task_terminated = true;
	for (qp = queues; qp < queues + nqueue; qp++) {
		err = pthread_join(qp->thread, NULL);
		if (err)
			err_exit("thread for %s join error\n", qp->qname);
	}
	free(queues);
	return 0;
}
