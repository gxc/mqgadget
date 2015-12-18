#include "cmqc.h"

extern bool task_terminated;	/* exit when true, defined in main.c */

struct qstat {
	pthread_t thread;
	char qmname[MQ_Q_MGR_NAME_LENGTH];
	char qname[MQ_Q_NAME_LENGTH];
	long curdepth;
	long maxdepth;
	bool getable;
	bool putable;
};

PMQHCONN connect_q_mgr(const char *q_mgr_name, PMQHCONN HconnPtr);
PMQHOBJ open_mq_obj(MQHCONN Hconn, MQOD * ObjDescPtr, MQLONG OpenOptions,
		    PMQHOBJ HobjPtr);
void close_mq_obj(MQHCONN Hconn, PMQHOBJ HobjPtr, const char *obj_name);
void disconnect_q_mgr(PMQHCONN HconnPtr);

void *inq_q_stat(void *qinfo);
