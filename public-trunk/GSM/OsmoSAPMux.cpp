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



#include "OsmoSAPMux.h"
#include "GSMTransfer.h"
#include "GSML1FEC.h"
#include "OsmoLogicalChannel.h"

#include <Logger.h>


using namespace GSM;


void OsmoSAPMux::writeHighSide(const BitVector& vector)
{
	OBJLOG(DEEPDEBUG) << "OsmoSAPMux::writeHighSide " << vector;
	/* put it into the top side of the L2 FIFO */
	/* NOTE: packs vector bits into 23-byte L2Frame, adding filler if needed */
	mL2Q.write(new L2Frame(vector, DATA));
}

void OsmoSAPMux::writeLowSide(const L2Frame& frame, const GSM::Time time, 
	const float RSSI, const int TA)
{
	OBJLOG(DEEPDEBUG) << "OsmoSAPMux::writeLowSide SAP" << frame.SAPI() << " " 
		<< frame;
	assert(mLchan);
	/* simply pass it right through to the OsmoThreadMux */
	mLchan->writeLowSide(frame, time, RSSI, TA);
}

void OsmoSAPMux::signalNextWtime(GSM::Time &time)
{
	assert(mLchan);

	/*  Only SCH and TCH/FACCH should be signalled every time.
	 *  BCCH, CCCHs, SDCCH, and SACCH should be every 4 times. */
	if(mLchan->type() == SCHType || mLchan->type() == FACCHType ||
		mLchan->type() == TCHFType || mLchan->type() == TCHHType)
	{
		mLchan->signalNextWtime(time);
	}
	else
	{
		const int comb = mLchan->TS()->getComb();


		/*  Ensure only one RTS is sent at first frame of each block. */
		if(comb == 5)
		{
			switch(time.T3())
			{
				case 2:
				case 6:
				case 12:
				case 16:
				case 22:
				case 26:
				case 32:
				case 36:
				case 42:
				case 46:
					mLchan->signalNextWtime(time);
				default:
					return;
			}
		}
		else if(comb == 7)
		{
			switch(time.T3())
			{
				case 0:
				case 4:
				case 8:
				case 12:
				case 16:
				case 20:
				case 24:
				case 28:
				case 32:
				case 36:
				case 40:
				case 44:
					mLchan->signalNextWtime(time);
				default:
					return;
			}
		}
	}
}

void OsmoSAPMux::dispatch()
{
	/* blocking read from FIFO */
	L2Frame *frame = mL2Q.read();

	assert(mDownstream);

	/* blocking write to L1Encoder */
	mLock.lock();
	mDownstream->writeHighSide(*frame);
	mLock.unlock();
}

void GSM::OsmoSAPRoutine( OsmoSAPMux *osm )
{
	while (1) {
		osm->dispatch();
	}
}

void OsmoSAPMux::start()
{
	mQueueThread.start((void*(*)(void*))OsmoSAPRoutine,(void *)this);
}

// vim: ts=4 sw=4
