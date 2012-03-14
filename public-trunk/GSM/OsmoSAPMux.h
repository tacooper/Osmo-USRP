/*
* Copyright 2011 Harald Welte <laforge@gnumonks.org>
* Copyright 2012 Thomas Cooper <tacooper@vt.edu>
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



#ifndef OsmoSAPMUX_H
#define OsmoSAPMUX_H

#include "GSMSAPMux.h"
#include "GSMTransfer.h"
#include "GSML1FEC.h"

#include <Logger.h>

namespace GSM {

class OsmoLogicalChannel;

/**
	A SAPMux is a multipexer the connects a single L1 to multiple L2s.
	A "service access point" in GSM/ISDN is analogous to port number in IP.
	GSM allows up to 4 SAPs, although only two are presently used.
	See GSM 04.05 5.2 for an introduction.
*/
class OsmoSAPMux : public SAPMux {

	protected:
	OsmoLogicalChannel *mLchan;
	L2FrameFIFO mL2Q;
	Thread mQueueThread;
	unsigned int mTCHcounter;
	unsigned int mSACCHcounter;

	public:

	OsmoSAPMux()
		:mLchan(NULL), mTCHcounter(3), mSACCHcounter(3) {
		start();
	}

	virtual ~OsmoSAPMux() {}

	virtual void writeHighSide(const L2Frame& frame);
	virtual void writeLowSide(const L2Frame& frame, const GSM::Time time, 
		const float RSSI, const int TA, const float FER);
	virtual void writeLowSideSACCH(const L2Frame& frame, const GSM::Time time, 
		const float RSSI, const int TA, const float FER, const int MSpower, 
		const int MStiming);
	virtual void writeLowSideTCH(const unsigned char* frame, 
		const GSM::Time time, const float RSSI, const int TA, const float FER);
	virtual void signalNextWtime(GSM::Time &time);

	void upstream(OsmoLogicalChannel *lchan) {
		assert(mLchan == NULL);
		mLchan = lchan;
	}

	void upstream( L2DL * wUpstream, unsigned wSAPI=0 )
		{ }
	/* use downstream() of SAPMux */

	/* actual dispatch routine, called in endless loop */
	void dispatch();

	/* start the dispatch thread */
	void start();
};

void OsmoSAPRoutine( OsmoSAPMux *osm );

}	// namespace GSM


#endif
// vim: ts=4 sw=4
