/*
* Copyright 2011 Harald Welte <laforge@gnumonks.org>
*
* This software is distributed under the terms of the GNU Affero Public License.
* See the COPYING file in the main directory for details.
*
* This use of this software may be subject to additional restrictions.
* See the LEGAL file in the main directory for details.

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU Affero General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU Affero General Public License for more details.

    You should have received a copy of the GNU Affero General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.

*/


#include "GSMTransfer.h"
#include "OsmoLogicalChannel.h"
#include "OsmoThreadMuxer.h"
#include <Logger.h>
#include "Interthread.h"
#include <Globals.h>
#include "GSMConfigL1.h"

#define msgb_l1prim(msg) ((GsmL1_Prim_t *)(msg)->l1h)
#define msgb_sysprim(msg) ((FemtoBts_Prim_t *)(msg)->l1h)

using namespace GSM;

/* Static helper functions for Osmo::msgb */
namespace Osmo
{
	static struct msgb *l1p_msgb_alloc()
	{
		struct msgb *msg = msgb_alloc(sizeof(GsmL1_Prim_t), "l1_prim");

		if(msg)
		{
			msg->l1h = msgb_put(msg, sizeof(GsmL1_Prim_t));
		}

		return msg;
	}

	static struct msgb *sysp_msgb_alloc()
	{
		struct msgb *msg = msgb_alloc(sizeof(FemtoBts_Prim_t), "sys_prim");

		if(msg)
		{
			msg->l1h = msgb_put(msg, sizeof(FemtoBts_Prim_t));
		}

		return msg;
	}
}

void OsmoThreadMuxer::writeLowSide(const L2Frame& frame,
				   struct OsmoLogicalChannel *lchan)
{
	OBJLOG(INFO) << "OsmoThreadMuxer::writeLowSide" << lchan << " " << frame;
	/* resolve SAPI, SS, TS, TRX numbers */
	/* build primitive that we can put into the up-queue */
}

void OsmoThreadMuxer::signalNextWtime(GSM::Time &time,
				      OsmoLogicalChannel &lchan)
{
	OBJLOG(DEBUG) << lchan << " " << time;
}

void OsmoThreadMuxer::startThreads()
{
	Thread recvSysMsgThread;
	recvSysMsgThread.start((void*(*)(void*))RecvSysMsgLoopAdapter, this);

	Thread recvL1MsgThread;
	recvL1MsgThread.start((void*(*)(void*))RecvL1MsgLoopAdapter, this);

	Thread sendTimeIndThread;
	sendTimeIndThread.start((void*(*)(void*))SendTimeIndLoopAdapter, this);
}

void *GSM::RecvSysMsgLoopAdapter(OsmoThreadMuxer *TMux)
{
	while(true)
	{
		TMux->recvSysMsg();

		pthread_testcancel();
	}
	return NULL;
}

void *GSM::RecvL1MsgLoopAdapter(OsmoThreadMuxer *TMux)
{
	while(true)
	{
		TMux->recvL1Msg();

		pthread_testcancel();
	}
	return NULL;
}

void *GSM::SendTimeIndLoopAdapter(OsmoThreadMuxer *TMux)
{
	while(true)
	{
		if(TMux->mRunningTimeInd)
		{
			TMux->buildMphTimeInd((uint32_t) gBTSL1.time().FN());
			sleep(1);
		}

		pthread_testcancel();
	}
	return NULL;
}

void OsmoThreadMuxer::recvSysMsg()
{
	const size_t PRIM_LEN = sizeof(FemtoBts_Prim_t);

	const int fd = mSockFd[SYS_READ];
	char buffer[PRIM_LEN];

	const int len = ::read(fd, (void*)buffer, PRIM_LEN);

	if(len == PRIM_LEN) // good frame
	{
		LOG(INFO) << "SYS_READ read() received good frame\nlen=" << len << 
			" buffer(hex)=";

		for(int i = 0; i < len; i++)
		{
			printf("%x ", (unsigned char)buffer[i]);
		}

		printf("\n");

		handleSysMsg(buffer);
	}
	else if(len == 0) // no frame
	{
	}
	else if(len < 0) // read() error
	{
		LOG(ERROR) << "Abort! SYS_READ read() returned: errno=" << 
			strerror(errno);

		abort();
	}
	else // bad frame
	{
		buffer[len] = '\0';

		LOG(ALARM) << "Bad frame, SYS_READ read() received bad frame, len=" << 
			len << " buffer=" << buffer;
	}
}

void OsmoThreadMuxer::recvL1Msg()
{
	const size_t PRIM_LEN = sizeof(GsmL1_Prim_t);

	const int fd = mSockFd[L1_READ];
	char buffer[PRIM_LEN];

	const int len = ::read(fd, (void*)buffer, PRIM_LEN);

	if(len == PRIM_LEN) // good frame
	{
		LOG(INFO) << "L1_READ read() received good frame\nlen=" << len << 
			" buffer(hex)=";

		for(int i = 0; i < len; i++)
		{
			printf("%x ", (unsigned char)buffer[i]);
		}

		printf("\n");

		handleL1Msg(buffer);
	}
	else if(len == 0) // no frame
	{
	}
	else if(len < 0) // read() error
	{
		LOG(ERROR) << "Abort! L1_READ read() returned: errno=" << 
			strerror(errno);

		abort();
	}
	else // bad frame
	{
		buffer[len] = '\0';

		LOG(ALARM) << "Bad frame, L1_READ read() received bad frame, len=" << 
			len << " buffer=" << buffer;
	}
}

void OsmoThreadMuxer::handleSysMsg(const char *buffer)
{
	struct Osmo::msgb *msg = Osmo::msgb_alloc_headroom(2048, 128, "l1_fd");

	msg->l1h = msg->data;
	memcpy(msg->l1h, buffer, sizeof(FemtoBts_Prim_t));
	Osmo::msgb_put(msg, sizeof(FemtoBts_Prim_t));

	FemtoBts_Prim_t *prim = msgb_sysprim(msg);

	printf("Received SYS message type = %s\n", 
		Osmo::get_value_string(Osmo::femtobts_sysprim_names, prim->id));

	switch(prim->id)
	{
		case FemtoBts_PrimId_SystemInfoReq:
			processSystemInfoReq();
			break;
		case FemtoBts_PrimId_ActivateRfReq:
			processActivateRfReq();
			break;
		case FemtoBts_PrimId_DeactivateRfReq:
			processDeactivateRfReq(msg);
			break;
		case FemtoBts_PrimId_SetTraceFlagsReq:
			// no action required
			break;
		case FemtoBts_PrimId_Layer1ResetReq:
			processLayer1ResetReq();
			break;
		default:
			LOG(ERROR) << "Invalid SYS prim type!";
	}

	Osmo::msgb_free(msg);
}

void OsmoThreadMuxer::handleL1Msg(const char *buffer)
{
	struct Osmo::msgb *msg = Osmo::msgb_alloc_headroom(2048, 128, "l1_fd");

	msg->l1h = msg->data;
	memcpy(msg->l1h, buffer, sizeof(GsmL1_Prim_t));
	Osmo::msgb_put(msg, sizeof(GsmL1_Prim_t));

	GsmL1_Prim_t *prim = msgb_l1prim(msg);

	printf("Received L1 message type = %s\n", 
		Osmo::get_value_string(Osmo::femtobts_l1prim_names, prim->id));

	switch(prim->id)
	{
		case GsmL1_PrimId_MphInitReq:
			processMphInitReq();
			break;
		case GsmL1_PrimId_MphConnectReq:
			processMphConnectReq(msg);
			break;
		case GsmL1_PrimId_MphActivateReq:
			processMphActivateReq(msg);
			break;
		default:
			LOG(ERROR) << "Invalid L1 prim type!";
	}

	Osmo::msgb_free(msg);
}

void OsmoThreadMuxer::processSystemInfoReq()
{
	struct Osmo::msgb *send_msg = Osmo::sysp_msgb_alloc();

	FemtoBts_Prim_t *sysp = msgb_sysprim(send_msg);
	FemtoBts_SystemInfoCnf_t *cnf = &sysp->u.systemInfoCnf;

	sysp->id = FemtoBts_PrimId_SystemInfoCnf;

	cnf->rfBand.gsm850 = 0;
	cnf->rfBand.gsm900 = 0;
	cnf->rfBand.dcs1800 = 0;
	cnf->rfBand.pcs1900 = 0;

	/* Determine preset band of operation */
	//TODO: determine which bands are supported, not preset
	const int band = gConfig.getNum("GSM.Band");
	switch(band)
	{
		case GSM850:
			cnf->rfBand.gsm850 = 1;
			break;
		case EGSM900:
			cnf->rfBand.gsm900 = 1;
			break;
		case DCS1800:
			cnf->rfBand.dcs1800 = 1;
			break;
		case PCS1900:
			cnf->rfBand.pcs1900 = 1;
			break;
		default:
			assert(0);
	}

	cnf->dspVersion.major = 0;
	cnf->dspVersion.minor = 0;
	cnf->dspVersion.build = 0;
	cnf->fpgaVersion.major = 0;
	cnf->fpgaVersion.minor = 0;
	cnf->fpgaVersion.build = 0;

	sendSysMsg(send_msg);
}

/* ignored input values:
	uint16_t u12ClkVc (no use)
*/
void OsmoThreadMuxer::processActivateRfReq()
{
	struct Osmo::msgb *send_msg = Osmo::sysp_msgb_alloc();

	FemtoBts_Prim_t *sysp = msgb_sysprim(send_msg);
	FemtoBts_ActivateRfCnf_t *cnf = &sysp->u.activateRfCnf;

	sysp->id = FemtoBts_PrimId_ActivateRfCnf;

	cnf->status = GsmL1_Status_Success;

	sendSysMsg(send_msg);
}

void OsmoThreadMuxer::processDeactivateRfReq(struct Osmo::msgb *recv_msg)
{
	/* Process received REQ message */
	FemtoBts_Prim_t *sysp_req = msgb_sysprim(recv_msg);
	FemtoBts_DeactivateRfReq_t *req = &sysp_req->u.deactivateRfReq;

	printf("REQ message status = %s\n", 
		Osmo::get_value_string(Osmo::femtobts_l1status_names, req->status));

	/* Build CNF message to send */
	struct Osmo::msgb *send_msg = Osmo::sysp_msgb_alloc();

	FemtoBts_Prim_t *sysp_cnf = msgb_sysprim(send_msg);
	FemtoBts_DeactivateRfCnf_t *cnf = &sysp_cnf->u.deactivateRfCnf;

	sysp_cnf->id = FemtoBts_PrimId_DeactivateRfCnf;

	cnf->status = GsmL1_Status_Success;

	sendSysMsg(send_msg);
}

void OsmoThreadMuxer::processLayer1ResetReq()
{
	struct Osmo::msgb *send_msg = Osmo::sysp_msgb_alloc();

	FemtoBts_Prim_t *sysp = msgb_sysprim(send_msg);
	FemtoBts_Layer1ResetCnf_t *cnf = &sysp->u.layer1ResetCnf;

	sysp->id = FemtoBts_PrimId_Layer1ResetCnf;

	cnf->status = GsmL1_Status_Success;

	sendSysMsg(send_msg);
}

/* TODO: in
	[to tune/setTSC, radio must be off. also TSC is usually set to BCC.]
	uint16_t u16Arfcn: radio->tune()
	uint8_t u8NbTsc: radio->setTSC()
*/
/* ignored input values:
	GsmL1_DevType_t devType (no use)
	int freqBand (already set in .config file)
	uint16_t u16BcchArfcn (no use)
	float fRxPowerLevel (already set somewhere else)
	float fTxPowerLevel (already set somewhere else)
*/
void OsmoThreadMuxer::processMphInitReq()
{
	struct Osmo::msgb *send_msg = Osmo::l1p_msgb_alloc();

	GsmL1_Prim_t *l1p = msgb_l1prim(send_msg);
	GsmL1_MphInitCnf_t *cnf = &l1p->u.mphInitCnf;
	
	l1p->id = GsmL1_PrimId_MphInitCnf;

	cnf->status = GsmL1_Status_Success;
	cnf->hLayer1 = mL1id;

	sendL1Msg(send_msg);
}

/* ignored input values:
	uint8_t u8Tn (no use)
	GsmL1_LogChComb_t logChComb (already set in .config file)
*/
void OsmoThreadMuxer::processMphConnectReq(struct Osmo::msgb *recv_msg)
{
	/* Process received REQ message */
	GsmL1_Prim_t *l1p_req = msgb_l1prim(recv_msg);
	GsmL1_MphConnectReq_t *req = &l1p_req->u.mphConnectReq;

	/* Check if L1 reference is correct */
	assert(mL1id == req->hLayer1);

	/* Build CNF message to send */
	struct Osmo::msgb *send_msg = Osmo::l1p_msgb_alloc();

	GsmL1_Prim_t *l1p = msgb_l1prim(send_msg);
	GsmL1_MphConnectCnf_t *cnf = &l1p->u.mphConnectCnf;
	
	l1p->id = GsmL1_PrimId_MphConnectCnf;

	cnf->status = GsmL1_Status_Success;

	sendL1Msg(send_msg);
}

/* ignored input values:
	GsmL1_LogChComb_t logChPrm (no use)
	GsmL1_SubCh_t subCh (no use)
	GsmL1_Dir_t dir (no use)
	float fBFILevel (no use)
	uint8_t u8Tn (no use, passed through)
*/
void OsmoThreadMuxer::processMphActivateReq(struct Osmo::msgb *recv_msg)
{
	/* Process received REQ message */
	GsmL1_Prim_t *l1p_req = msgb_l1prim(recv_msg);
	GsmL1_MphActivateReq_t *req = &l1p_req->u.mphActivateReq;

	/* Check if L1 reference is correct */
	assert(mL1id == req->hLayer1);

	/* Store reference to L2 for this SAPI */
	mL2id[req->sapi] = req->hLayer2;

	/* Start sending MphTimeInd messages if SCH is activated */
	if(req->sapi == GsmL1_Sapi_Sch)
	{
		mRunningTimeInd = true;
	}

	printf("REQ message SAPI = %s\n", 
		Osmo::get_value_string(Osmo::femtobts_l1sapi_names, req->sapi));

	/* Build CNF message to send */
	struct Osmo::msgb *send_msg = Osmo::l1p_msgb_alloc();

	GsmL1_Prim_t *l1p = msgb_l1prim(send_msg);
	GsmL1_MphActivateCnf_t *cnf = &l1p->u.mphActivateCnf;
	
	l1p->id = GsmL1_PrimId_MphActivateCnf;

	cnf->u8Tn = req->u8Tn;
	cnf->sapi = req->sapi;
	cnf->status = GsmL1_Status_Success;

	sendL1Msg(send_msg);
}

void OsmoThreadMuxer::buildMphTimeInd(uint32_t time)
{
	struct Osmo::msgb *send_msg = Osmo::l1p_msgb_alloc();

	GsmL1_Prim_t *l1p = msgb_l1prim(send_msg);
	GsmL1_MphTimeInd_t *ind = &l1p->u.mphTimeInd;
	
	l1p->id = GsmL1_PrimId_MphTimeInd;

	ind->u32Fn = time;

	sendL1Msg(send_msg);
}

void OsmoThreadMuxer::sendSysMsg(struct Osmo::msgb *msg)
{
	const int PRIM_LEN = sizeof(FemtoBts_Prim_t);
	const int MSG_LEN = Osmo::msgb_l1len(msg);

	if(MSG_LEN != PRIM_LEN)
	{
		LOG(ERROR) << "SYS message lengths do not match! MSG_LEN=" << MSG_LEN <<
			" PRIM_LEN=" << PRIM_LEN;
	}
	else
	{
		int rc = ::write(mSockFd[SYS_WRITE], msg->l1h, MSG_LEN);

		if(rc == MSG_LEN)
		{
			FemtoBts_Prim_t *prim = msgb_sysprim(msg);

			LOG(INFO) << "SYS_WRITE write() sent good frame\ntype=" <<
				Osmo::get_value_string(Osmo::femtobts_sysprim_names, prim->id)
				<< "\nlen=" << MSG_LEN << " buffer(hex)=";

			for(int i = 0; i < MSG_LEN; i++)
			{
				printf("%x ", (unsigned char)msg->l1h[i]);
			}

			printf("\n");
		}
		else if(rc > 0)
		{
			LOG(ERROR) << 
				"SYS_WRITE write() lengths do not match! MSG_LEN=" << 
				MSG_LEN << " rc=" << rc;
		}
		else
		{
			LOG(ERROR) << "SYS_WRITE write() returned: errno=" << 
				strerror(errno);
		}
	}

	Osmo::msgb_free(msg);
}

void OsmoThreadMuxer::sendL1Msg(struct Osmo::msgb *msg)
{
	const int PRIM_LEN = sizeof(GsmL1_Prim_t);
	const int MSG_LEN = Osmo::msgb_l1len(msg);

	if(MSG_LEN != PRIM_LEN)
	{
		LOG(ERROR) << "L1 message lengths do not match! MSG_LEN=" << MSG_LEN <<
			" PRIM_LEN=" << PRIM_LEN;
	}
	else
	{
		int rc = ::write(mSockFd[L1_WRITE], msg->l1h, MSG_LEN);

		if(rc == MSG_LEN)
		{
			GsmL1_Prim_t *prim = msgb_l1prim(msg);

			LOG(INFO) << "L1_WRITE write() sent good frame\ntype=" <<
				Osmo::get_value_string(Osmo::femtobts_l1prim_names, prim->id)
				<< "\nlen=" << MSG_LEN << " buffer(hex)=";

			for(int i = 0; i < MSG_LEN; i++)
			{
				printf("%x ", (unsigned char)msg->l1h[i]);
			}

			printf("\n");
		}
		else if(rc > 0)
		{
			LOG(ERROR) << 
				"L1_WRITE write() lengths do not match! MSG_LEN=" << 
				MSG_LEN << " rc=" << rc;
		}
		else
		{
			LOG(ERROR) << "L1_WRITE write() returned: errno=" << 
				strerror(errno);
		}
	}

	Osmo::msgb_free(msg);
}

void OsmoThreadMuxer::createSockets()
{
	/* Create directory to hold socket files */
	int rc = mkdir("/dev/msgq/", S_ISVTX);
	// if directory exists, just continue anyways
	if(errno != EEXIST && rc < 0)
	{
		LOG(ERROR) << "mkdir() returned: errno=" << strerror(errno);
	}

	/* Create path for Sys Write */
	rc = ::mkfifo(getPath(SYS_WRITE), S_ISVTX);
	// if file exists, just continue anyways
	if(errno != EEXIST && rc < 0)
	{
		LOG(ERROR) << "SYS_WRITE mkfifo() returned: errno=" << 
			strerror(errno);
	}
	mSockFd[SYS_WRITE] = ::open(getPath(SYS_WRITE), O_WRONLY);
	if(mSockFd[SYS_WRITE] < 0)
	{
		::close(mSockFd[SYS_WRITE]);
	}

	/* Create path for L1 Write */
	rc = ::mkfifo(getPath(L1_WRITE), S_ISVTX);
	// if file exists, just continue anyways
	if(errno != EEXIST && rc < 0)
	{
		LOG(ERROR) << "L1_WRITE mkfifo() returned: errno=" << 
			strerror(errno);
	}
	mSockFd[L1_WRITE] = ::open(getPath(L1_WRITE), O_WRONLY);
	if(mSockFd[L1_WRITE] < 0)
	{
		::close(mSockFd[L1_WRITE]);
	}

	/* Create path for Sys Read */
	rc = ::mkfifo(getPath(SYS_READ), S_ISVTX);
	// if file exists, just continue anyways
	if(errno != EEXIST && rc < 0)
	{
		LOG(ERROR) << "SYS_READ mkfifo() returned: errno=" << 
			strerror(errno);
	}
	mSockFd[SYS_READ] = ::open(getPath(SYS_READ), O_RDONLY);
	if(mSockFd[SYS_READ] < 0)
	{
		::close(mSockFd[SYS_READ]);
	}

	/* Create path for L1 Read */
	rc = ::mkfifo(getPath(L1_READ), S_ISVTX);
	// if file exists, just continue anyways
	if(errno != EEXIST && rc < 0)
	{
		LOG(ERROR) << "L1_READ mkfifo() returned: errno=" << 
			strerror(errno);
	}
	mSockFd[L1_READ] = ::open(getPath(L1_READ), O_RDONLY);
	if(mSockFd[L1_READ] < 0)
	{
		::close(mSockFd[L1_READ]);
	}
}

const char* OsmoThreadMuxer::getPath(const int index)
{
	switch(index)
	{
		case SYS_WRITE:		
			return "/dev/msgq/femtobts_dsp2arm"; // PathSysWrite
		case L1_WRITE:
			return "/dev/msgq/gsml1_dsp2arm"; // PathL1Write
		case SYS_READ:
			return "/dev/msgq/femtobts_arm2dsp"; // PathSysRead
		case L1_READ:
			return "/dev/msgq/gsml1_arm2dsp"; // PathL1Read
		default:
			assert(0);
	}
}

// vim: ts=4 sw=4
