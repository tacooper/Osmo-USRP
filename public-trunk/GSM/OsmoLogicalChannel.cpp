/**@file Osmocom Logical Channel.  */

/*
* Copyright 2008, 2009, 2010 Free Software Foundation, Inc.
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



#include "OsmoLogicalChannel.h"
#include "OsmoThreadMuxer.h"

using namespace std;
using namespace GSM;


ARFCNManager *OsmoTS::getARFCNmgr()
{
	TransceiverManager *trxmgr = getTRX()->getTRXmgr();
	return trxmgr->ARFCN(getTSnr());
}

OsmoTS::OsmoTS(OsmoTRX &trx, unsigned int ts_nr, unsigned comb)
{
	assert(ts_nr < 8);
	mComb = comb;
	mTSnr = ts_nr;
	mTRX = &trx;
	mNLchan = 0;

	TransceiverManager *trxmgr = trx.getTRXmgr();
	ARFCNManager *radio = trxmgr->ARFCN(ts_nr);
	radio->setSlot(ts_nr, comb);
}


void OsmoLogicalChannel::open()
{
	LOG(INFO);
	if (mSACCHL1) mSACCHL1->open();
	if (mL1) mL1->open();
}


void OsmoLogicalChannel::connect()
{
	mMux.downstream(mL1);
	if (mL1) mL1->upstream(&mMux);
	//mMux.upstream(mL2[s],s);
	//if (mL2[s]) mL2[s]->downstream(&mMux);
}

void OsmoLogicalChannel::writeLowSide(const L2Frame& frame)
{
	mTM->writeLowSide(frame, this);
}

ostream& GSM::operator<<(ostream& os, const OsmoLogicalChannel& lchan)
{
	unsigned int trx_nr, ts_nr, ss_nr;
	trx_nr = lchan.TS()->getTRX()->getTN();
	ts_nr = lchan.TS()->getTSnr();
	ss_nr = lchan.SSnr();

	os << "(" << trx_nr << "," << ts_nr << "," << ss_nr << ")";
}


void OsmoLogicalChannel::downstream(ARFCNManager* radio)
{
	assert(mL1);
	mL1->downstream(radio);
	if (mSACCHL1) mSACCHL1->downstream(radio);
}

OsmoCCCHLchan::OsmoCCCHLchan(OsmoTS *osmo_ts, unsigned int ss_nr)
	:OsmoNDCCHLogicalChannel(osmo_ts, ss_nr)
{
	mL1 = new CCCHL1FEC(gCCCH[ss_nr]);
	connect();
}


OsmoSDCCHLchan::OsmoSDCCHLchan(OsmoTS *osmo_ts, unsigned int ss_nr)
	:OsmoLogicalChannel(osmo_ts, ss_nr)
{
	unsigned int ts_nr = osmo_ts->getTSnr();
	const CompleteMapping *wMapping = NULL;

	switch (osmo_ts->getComb()) {
	case 5:
		wMapping = &gSDCCH4[ss_nr];
		break;
	case 7:
		wMapping = &gSDCCH8[ss_nr];
		break;
	default:
		assert(0);
	}
	mL1 = new SDCCHL1FEC(ts_nr, wMapping->LCH());
	connect();
}

OsmoTCHFACCHLchan::OsmoTCHFACCHLchan(OsmoTS *osmo_ts, unsigned int ss_nr)
	:OsmoLogicalChannel(osmo_ts, ss_nr)
{
	unsigned int ts_nr = osmo_ts->getTSnr();
	mL1 = new TCHFACCHL1FEC(ts_nr, gTCHF_T[ts_nr].LCH());
	connect();
}


// vim: ts=4 sw=4

