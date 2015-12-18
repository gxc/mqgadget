#include <unistd.h>
#include <stdbool.h>
#include <string.h>

#include "mqgadget.h"
#include "util.h"

#define INQUIRY_INTERVAL 500000

/**
 * inq_q_stat(): threaded inquiry of queue statuses
 * The results are set to members of `qinfo'.
 */
void *inq_q_stat(void *qinfo)
{
	MQHCONN Hconn;
	MQHOBJ Hobj;
	MQOD ObjDesc = { MQOD_DEFAULT };
	MQLONG OpenOptions;
	MQLONG CompCode;
	MQLONG Reason;
	MQLONG SelectorCount = 4;
	MQLONG IntAttrCount = 4;
	MQLONG Selectors[4] = {
		MQIA_CURRENT_Q_DEPTH,
		MQIA_MAX_Q_DEPTH,
		MQIA_INHIBIT_GET,
		MQIA_INHIBIT_PUT
	};
	MQLONG IntAttrs[4];

	struct qstat *qsp = (struct qstat *)qinfo;
	ObjDesc.ObjectType = MQOT_Q;
	strncpy(ObjDesc.ObjectName, qsp->qname, MQ_Q_NAME_LENGTH);
	/* MQOO_SET is a mask for error: MQRC_SELECTOR_NOT_FOR_TYPE 2068 */
	OpenOptions = MQOO_INQUIRE | MQOO_SET | MQOO_FAIL_IF_QUIESCING;

	connect_q_mgr(qsp->qmname, &Hconn);
	open_mq_obj(Hconn, &ObjDesc, OpenOptions, &Hobj);

	/* inquire queue statues once every INQUIRY_INTERVAL microseconds
	   and refresh the records */
	while (!task_terminated) {
		MQINQ(Hconn, Hobj, SelectorCount, Selectors, IntAttrCount,
		      IntAttrs, 0, NULL, &CompCode, &Reason);
		if (CompCode != MQCC_OK)
			err_exit
			    ("Failed to inquire queue(%s) with reason code %d.\n",
			     qsp->qname, Reason);

		qsp->curdepth = IntAttrs[0];
		qsp->maxdepth = IntAttrs[1];
		qsp->getable = IntAttrs[2] ? false : true;
		qsp->putable = IntAttrs[3] ? false : true;

		usleep(INQUIRY_INTERVAL);
	}

	close_mq_obj(Hconn, &Hobj, qsp->qname);
	disconnect_q_mgr(&Hconn);

	return NULL;
}
