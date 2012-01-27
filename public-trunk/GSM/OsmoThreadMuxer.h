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
#include "gsmL1prim.h"

#define SYS_WRITE 0
#define L1_WRITE 1
#define SYS_READ 2
#define L1_READ 3

/* Resolves multiple definition conflict between ortp and osmocom msgb's */
namespace Osmo {
	extern "C" {
		#include <osmocom/core/msgb.h>
	}
}

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
	int mSockFd[4];
	OsmoTRX *mTRX[1];
	unsigned int mNumTRX;

public:
	OsmoThreadMuxer()
		:mNumTRX(0)
	{
		createSockets();

		if(mSockFd[0] < 0 || mSockFd[1] < 0 || mSockFd[2] < 0 || mSockFd[3] < 0)
		{
			LOG(ERROR) << "Error creating socket files. RESTART!";
		}
		else
		{
			LOG(INFO) << "All 4 socket files created.";
		}

		startThreads();
	}

	OsmoTRX &addTRX(TransceiverManager &trx_mgr, unsigned int trx_nr) {
		/* for now we only support a single TRX */
		assert(mNumTRX == 0);
		OsmoTRX *otrx = new OsmoTRX(trx_mgr, trx_nr, this);
		mTRX[mNumTRX++] = otrx;
		return *otrx;
	}

	/* receive frame synchronously from L1Decoder->OsmoSAPMux and
	 * enqueue it towards osmo-bts */
	virtual void writeLowSide(const L2Frame& frame,
				  OsmoLogicalChannel *lchan);

	/* L1 informs us about the next TDMA time for which it needs
	 * data */
	virtual void signalNextWtime(GSM::Time &time,
				     OsmoLogicalChannel &lchan);

private:
	void createSockets();

	void startThreads();

	void recvSysMsg();

	const char* getTypeOfSysMsg(char* buffer);

	const char* getPath(int index)
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
	};

	friend void *RecvSysMsgLoopAdapter(OsmoThreadMuxer *TMux);
};

void *RecvSysMsgLoopAdapter(OsmoThreadMuxer *TMux);

};		// GSM

#endif /* OsmoThreadMuxer_H */
