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
#include "gsmL1prim.h"



using namespace std;
using namespace GSM;


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

void OsmoThreadMuxer::recvSysMsg()
{
	const size_t SYS_PRIM_LEN = sizeof(FemtoBts_Prim_t);

	int fd = mSockFd[SYS_READ];
	char buffer[SYS_PRIM_LEN];

	int len = ::read(fd, (void*)buffer, SYS_PRIM_LEN);

	if(len == SYS_PRIM_LEN) // good frame
	{
		LOG(INFO) << "SYS_READ read() received good frame\ntype=" << 
			getTypeOfSysMsg(buffer) << "\nlen=" << len << " buffer(hex)=";

		for(int i = 0; i < len; i++)
		{
			printf("%x ", (unsigned char)buffer[i]);
		}

		printf("\n");

		//handleSysMsg(buffer);
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

const char *OsmoThreadMuxer::getTypeOfSysMsg(char *buffer)
{
	assert(buffer[12]);

	switch(buffer[12])
	{
		case FemtoBts_PrimId_SystemInfoReq:
			return "SYSTEM-INFO.req";
		case FemtoBts_PrimId_SystemInfoCnf:
			return "SYSTEM-INFO.conf";
		case FemtoBts_PrimId_SystemFailureInd:
			return "SYSTEM-FAILURE.ind";
		case FemtoBts_PrimId_ActivateRfReq:
			return "ACTIVATE-RF.req";
		case FemtoBts_PrimId_ActivateRfCnf:
			return "ACTIVATE-RF.conf";
		case FemtoBts_PrimId_DeactivateRfReq:
			return "DEACTIVATE-RF.req";
		case FemtoBts_PrimId_DeactivateRfCnf:
			return "DEACTIVATE-RF.conf";
		case FemtoBts_PrimId_SetTraceFlagsReq:
			return "SET-TRACE-FLAGS.req";
		case FemtoBts_PrimId_RfClockInfoReq:
			return "RF-CLOCK-INFO.req";
		case FemtoBts_PrimId_RfClockInfoCnf:
			return "RF-CLOCK-INFO.conf";
		case FemtoBts_PrimId_RfClockSetupReq:
			return "RF-CLOCK-SETUP.req";
		case FemtoBts_PrimId_RfClockSetupCnf:
			return "RF-CLOCK-SETUP.conf";
		case FemtoBts_PrimId_Layer1ResetReq:
			return "LAYER1-RESET.req";
		case FemtoBts_PrimId_Layer1ResetCnf:
			return "LAYER1-RESET.conf";
		default:
			return "WARNING: Invalid type!";
	}
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
		LOG(ERROR) << SYS_WRITE << " mkfifo() returned: errno=" << 
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
		LOG(ERROR) << L1_WRITE << " mkfifo() returned: errno=" << 
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
		LOG(ERROR) << SYS_READ << " mkfifo() returned: errno=" << 
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
		LOG(ERROR) << L1_READ << " mkfifo() returned: errno=" << 
			strerror(errno);
	}
	mSockFd[L1_READ] = ::open(getPath(L1_READ), O_RDONLY);
	if(mSockFd[L1_READ] < 0)
	{
		::close(mSockFd[L1_READ]);
	}
}

// vim: ts=4 sw=4
