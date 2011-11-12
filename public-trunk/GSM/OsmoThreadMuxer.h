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
	int mSockFd[2];
	OsmoTRX *mTRX[1];
	unsigned int mNumTRX;

public:
	OsmoThreadMuxer() {
		int rc;

		rc = socketpair(AF_UNIX, SOCK_DGRAM, 0, mSockFd);
	}

	int getUserFd() {
		return mSockFd[1];
	}

	int addTRX(TransceiverManager &trx_mgr, unsigned int trx_nr) {
		/* for now we only support a single TRX */
		assert(mNumTRX == 0);
		OsmoTRX *otrx = new OsmoTRX(trx_mgr, trx_nr);
		mTRX[mNumTRX++] = otrx;
	}

	/* receive frame synchronously from L1Decoder->OsmoSAPMux and
	 * euqneue it towards osmo-bts */
	virtual void writeLowSide(const L2Frame& frame,
				  OsmoLogicalChannel *lchan);
};

};		// GSM

#endif /* OsmoThreadMuxer_H */
