#include "cmqc.h"
#include "util.h"

/* connect to queue manager, return connection handle pointer */
void connect_q_mgr(char *q_mgr_name, MQHCONN * HconnPtr)
{
	MQLONG CompCode;
	MQLONG Reason;

	MQCONN(q_mgr_name, HconnPtr, &CompCode, &Reason);
	if ((CompCode != MQCC_OK) | (Reason != MQRC_NONE))
		err_exit("Failed to connect %s with reason code %d.\n",
			 q_mgr_name, Reason);
}

/*
 * open MQ object specified by `ObjDescPtr' with `OpenOptions'
 * return the pointer to the MQ object
 */
void open_mq_obj(MQHCONN Hconn, MQOD * ObjDescPtr, MQLONG OpenOptions,
		 MQHOBJ * HobjPtr)
{
	MQLONG CompCode;
	MQLONG Reason;

	MQOPEN(Hconn, ObjDescPtr, OpenOptions, HobjPtr, &CompCode, &Reason);
	if ((CompCode != MQCC_OK) || (Reason != MQRC_NONE))
		err_exit("Failed to open %s with reason code %d.\n",
			 ObjDescPtr->ObjectName, Reason);
}

/* close queue */
void close_mq_obj(MQHCONN Hconn, MQHOBJ * HobjPtr, const char *obj_name)
{
	MQLONG CompCode;
	MQLONG Reason;

	MQCLOSE(Hconn, HobjPtr, MQCO_NONE, &CompCode, &Reason);
	if ((CompCode != MQCC_OK) || (Reason != MQRC_NONE))
		err_exit("Failed to close %s with reason code %d.\n", obj_name,
			 Reason);
}

/* disconnect from queue manager */
void disconnect_q_mgr(MQHCONN * HconnPtr)
{
	MQLONG CompCode;
	MQLONG Reason;

	MQDISC(HconnPtr, &CompCode, &Reason);
	if ((CompCode != MQCC_OK) || (Reason != MQRC_NONE))
		err_exit("Failed to disconnect with reason code %d.\n", Reason);
}
