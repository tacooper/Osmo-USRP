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


void OsmoSAPMux::writeHighSide(const L2Frame& frame)
{
	OBJLOG(DEEPDEBUG) << "OsmoSAPMux::writeHighSide " << frame;
	/* put it into the top side of the L2 FIFO */
	mL2Q.write(new L2Frame(frame));
}

void OsmoSAPMux::writeLowSide(const L2Frame& frame)
{
	OBJLOG(DEEPDEBUG) << "OsmoSAPMux::writeLowSide SAP" << frame.SAPI() << " " << frame;
	assert(mLchan);
	/* simply pass it right through to the OsmoThreadMux */
	mLchan->writeLowSide(frame);
}

void OsmoSAPMux::signalNextWtime(GSM::Time &time)
{
	assert(mLchan);
	mLchan->signalNextWtime(time);
}

void OsmoSAPMux::dispatch()
{
	L2Frame *frame;

	/* blocking read from FIFO */
	frame = mL2Q.read();

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
	OBJLOG(DEBUG) << "OsmoSAPMux";
	mQueueThread.start((void*(*)(void*))OsmoSAPRoutine,(void *)this);
}

// vim: ts=4 sw=4
