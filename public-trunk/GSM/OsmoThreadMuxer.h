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

#ifndef OsmoThreadMuxer_H
#define OsmoThreadMuxer_H

#include "OsmoLogicalChannel.h"
#include <TRXManager.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>

namespace GSM {

/* The idea of this monster is to provide an interface between the
 * heavily multi-threaded OpenBTS architecture and the single-threaded
 * osmo-bts architecture.
 *
 * In the Uplink Rx path, L1Decoder calls OsmoSAPMux:writeLowSide,
 * which directly passes the call through to * OsmoThreadMuxer:writeLowSide
 * 
 * At this point, the L2Frame needs to be converted from unpacked bits
 * to packed bits, and wrapped with some layer1 primitive header.  Next,
 * it is enqueued into a FIFO leading towards osmo-bts.  Whenever that
 * FIFO has data to be written, we signal this via the sock_fd.
 * (socketpair).  The other fd ends up in the select() loop of osmo-bts.
 *
 * In the Downling Tx path, the OsmoThreadMuxer thread FIXME
 */
class OsmoThreadMuxer {

protected:
	// mSockFd[0] == PathSysWrite
	// mSockFd[1] == PathL1Write
	// mSockFd[2] == PathSysRead
	// mSockFd[3] == PathL1Read
	int mSockFd[4];
	OsmoTRX *mTRX[1];
	unsigned int mNumTRX;

public:
	OsmoThreadMuxer()
		:mNumTRX(0)
	{
		/* Create directory for socket files if it doesn't exist already */
		int rc = mkdir("/dev/msgq/", S_ISVTX);

		/* Create path for Sys Write */
		rc = ::mkfifo(getPath(0), S_ISVTX);
		// if file exists, just continue anyways
		if(errno != EEXIST && rc < 0)
		{
			LOG(ERROR) << "0 mkfifo() returned: errno=" << strerror(errno);
		}
		mSockFd[0] = ::open(getPath(0), O_WRONLY);
		if(mSockFd[0] < 0)
		{
			if(mSockFd[0])
			{
				::close(mSockFd[0]);
			}
		}

		/* Create path for L1 Write */
		rc = ::mkfifo(getPath(1), S_ISVTX);
		// if file exists, just continue anyways
		if(errno != EEXIST && rc < 0)
		{
			LOG(ERROR) << "1 mkfifo() returned: errno=" << strerror(errno);
		}
		mSockFd[1] = ::open(getPath(1), O_WRONLY);
		if(mSockFd[1] < 0)
		{
			if(mSockFd[1])
			{
				::close(mSockFd[1]);
			}
		}

		/* Create path for Sys Read */
		rc = ::mkfifo(getPath(2), S_ISVTX);
		// if file exists, just continue anyways
		if(errno != EEXIST && rc < 0)
		{
			LOG(ERROR) << "2 mkfifo() returned: errno=" << strerror(errno);
		}
		mSockFd[2] = ::open(getPath(2), O_RDONLY);
		if(mSockFd[2] < 0)
		{
			if(mSockFd[2])
			{
				::close(mSockFd[2]);
			}
		}

		/* Create path for L1 Read */
		rc = ::mkfifo(getPath(3), S_ISVTX);
		// if file exists, just continue anyways
		if(errno != EEXIST && rc < 0)
		{
			LOG(ERROR) << "3 mkfifo() returned: errno=" << strerror(errno);
		}
		mSockFd[3] = ::open(getPath(3), O_RDONLY);
		if(mSockFd[3] < 0)
		{
			if(mSockFd[3])
			{
				::close(mSockFd[3]);
			}
		}

		LOG(INFO) << "All 4 socket files created.";
	}

	const char* getPath(int index)
	{
		switch(index)
		{
			case 0:		
				return "/dev/msgq/femtobts_dsp2arm"; // PathSysWrite
			case 1:
				return "/dev/msgq/gsml1_dsp2arm"; // PathL1Write
			case 2:
				return "/dev/msgq/femtobts_arm2dsp"; // PathSysRead
			case 3:
				return "/dev/msgq/gsml1_arm2dsp"; // PathL1Read
			default:
				assert(0);
		}
	};

	OsmoTRX &addTRX(TransceiverManager &trx_mgr, unsigned int trx_nr) {
		/* for now we only support a single TRX */
		assert(mNumTRX == 0);
		OsmoTRX *otrx = new OsmoTRX(trx_mgr, trx_nr, this);
		mTRX[mNumTRX++] = otrx;
		return *otrx;
	}

	/* receive frame synchronously from L1Decoder->OsmoSAPMux and
	 * euqneue it towards osmo-bts */
	virtual void writeLowSide(const L2Frame& frame,
				  OsmoLogicalChannel *lchan);

	/* L1 informs us about the next TDMA time for which it needs
	 * data */
	virtual void signalNextWtime(GSM::Time &time,
				     OsmoLogicalChannel &lchan);
};

};		// GSM

#endif /* OsmoThreadMuxer_H */
