#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdbool.h>
#include <pthread.h>
#include <libgen.h>
#include <limits.h>

#include "cmqc.h"
#include "util.h"

#define INQUIRY_INTERVAL 500000

extern char *program_invocation_short_name;
static bool done = false;	/* exit when true */

struct qstat {
	pthread_t thread;
	char qmname[MQ_Q_MGR_NAME_LENGTH];
	char qname[MQ_Q_NAME_LENGTH];
	long curdepth;
	long maxdepth;
	bool getable;
	bool putable;
};

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

/**
 * inq_q_stat(): threaded inquiry of queue statuses
 * The results are set to members of `qinfo'.
 */
static void *inq_q_stat(void *qinfo)
{
	MQHCONN Hconn;
	MQHOBJ Hobj;
	MQOD ObjDesc = { MQOD_DEFAULT };
	MQLONG OpenOptions;
	MQLONG CompCode;
	MQLONG Reason;
	MQLONG SelectorCount = 4L;
	MQLONG IntAttrCount = 4L;
	MQLONG Selectors[4] = {
		MQIA_CURRENT_Q_DEPTH,
		MQIA_MAX_Q_DEPTH,
		MQIA_INHIBIT_GET,
		MQIA_INHIBIT_PUT
	};
	MQLONG IntAttrs[4];

	struct qstat *qsp = (struct qstat *)qinfo;

	/* connect to queue manager */
	MQCONN(qsp->qmname, &Hconn, &CompCode, &Reason);
	if ((CompCode != MQCC_OK) | (Reason != MQRC_NONE)) {
		err_exit("Failed to connect %s with reason code %ld.\n",
			 qsp->qmname, Reason);
	}

	/* open queue for inquiring */
	ObjDesc.ObjectType = MQOT_Q;
	strncpy(ObjDesc.ObjectName, qsp->qname, MQ_Q_NAME_LENGTH);
	/* MQOO_SET is a mask for error: MQRC_SELECTOR_NOT_FOR_TYPE 2068 */
	OpenOptions = MQOO_INQUIRE | MQOO_SET | MQOO_FAIL_IF_QUIESCING;
	MQOPEN(Hconn, &ObjDesc, OpenOptions, &Hobj, &CompCode, &Reason);
	if ((CompCode != MQCC_OK) || (Reason != MQRC_NONE)) {
		err_exit("Failed to open %s with reason code %ld.\n",
			 qsp->qname, Reason);
	}

	/* inquire queue statues once every INQUIRY_INTERVAL microseconds
	   and refresh the records */
	while (!done) {
		MQINQ(Hconn, Hobj, SelectorCount, Selectors, IntAttrCount,
		      IntAttrs, 0L, NULL, &CompCode, &Reason);
		if (CompCode != MQCC_OK) {
			err_exit
			    ("Failed to inquire queue(%s) with reason code %ld.\n",
			     qsp->qname, Reason);
		}

		qsp->curdepth = IntAttrs[0];
		qsp->maxdepth = IntAttrs[1];
		qsp->getable = IntAttrs[2] ? false : true;
		qsp->putable = IntAttrs[3] ? false : true;
		usleep(INQUIRY_INTERVAL);
	}

	/* close queue */
	MQCLOSE(Hconn, &Hobj, MQCO_NONE, &CompCode, &Reason);
	if ((CompCode != MQCC_OK) || (Reason != MQRC_NONE)) {
		err_exit("Failed to close %s with reason code %ld.\n",
			 qsp->qname, Reason);
	}

	/* disconnect */
	MQDISC(&Hconn, &CompCode, &Reason);
	if ((CompCode != MQCC_OK) || (Reason != MQRC_NONE)) {
		err_exit("Failed to disconnect %s with reason code %ld.\n",
			 qsp->qmname, Reason);
	}

	return NULL;
}

int main(int argc, char *argv[])
{
	char qmgrname[MQ_Q_MGR_NAME_LENGTH];
	struct qstat *queues;	/* queue info array */
	struct qstat *qp;	/* `queues' walker */
	int interval;		/* refresh interval */
	int ncount;		/* refresh times */
	int nqueue;		/* number of queues to monitor */
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
			interval = get_int_arg(optarg);
			break;
		case 'c':
			ncount = get_int_arg(optarg);
			break;
		case 'h':
			usage(EXIT_SUCCESS);
			break;
		case '?':
			usage(EXIT_FAILURE);
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
	for (i = 0; i < ncount; i++) {
		if (i == 0)	/* wait 0.1 second for inquiring */
			usleep(100000L);
		if (ncount > 1 && is_vt100())
			puts("[1J");	/* clear screen */
		puts("QNAME       CURDEPTH  PERCENT   GET       PUT");
		puts("----------- --------- --------- --------- ---------");
		for (qp = queues; qp < queues + nqueue; qp++) {
			printf("%-12s%-10ld%-10.2f%-10s%-10s\n",
			       qp->qname,
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

	done = true;
	for (qp = queues; qp < queues + nqueue; qp++) {
		err = pthread_join(qp->thread, NULL);
		if (err)
			err_exit("thread for %s join error\n", qp->qname);
	}
	free(queues);

	return 0;
}
