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
	const OsmoTRX *trx = getTRX();
	TransceiverManager *trxmgr = trx->getTRXmgr();
	return trxmgr->ARFCN(trx->getTN());
}

OsmoTS::OsmoTS(OsmoTRX &trx, unsigned int ts_nr, unsigned comb)
{
	assert(ts_nr < 8);
	mComb = comb;
	mTSnr = ts_nr;
	mTRX = &trx;
	mNLchan = 0;

	/* configure the radio timeslot combination */
	TransceiverManager *trxmgr = trx.getTRXmgr();
	ARFCNManager *radio = trxmgr->ARFCN(trx.getTN());
	radio->setSlot(ts_nr, comb);
}


void OsmoLogicalChannel::open()
{
	LOG(INFO);
	if (mSACCHL1)
		mSACCHL1->open();
	if (mL1)
		mL1->open();
}


void OsmoLogicalChannel::connect()
{
	/* connect L1 at lower end of OsmoSAPMux */
	mMux.downstream(mL1);

	/* connect SAPmux upper end to this logical channel */
	mMux.upstream(this);

	/* Tell L1FEC that the SAPMux is its upstream */
	if (mL1)
		mL1->upstream(&mMux);
}

/* This is where OsmoSAPMux inputs data received from L1FEC */
void OsmoLogicalChannel::writeLowSide(const L2Frame& frame)
{
	/* simply pass it through to the TreadMuxer, including
	 * a reference to us (the logical channel) */
	mTM->writeLowSide(frame, this);
}

ostream& GSM::operator<<(ostream& os, const OsmoLogicalChannel& lchan)
{
	unsigned int trx_nr, ts_nr, ss_nr;
	trx_nr = lchan.TS()->getTRX()->getTN();
	ts_nr = lchan.TS()->getTSnr();
	ss_nr = lchan.SSnr();

	/* Just dump something like "(TRX,TS,SS)" identifying the lchan */
	os << "(" << trx_nr << "," << ts_nr << "," << ss_nr << ")";
}

void OsmoLogicalChannel::signalNextWtime(GSM::Time &time)
{
	if (mTM)
		mTM->signalNextWtime(time);
}

void OsmoLogicalChannel::downstream(ARFCNManager* radio)
{
	assert(mL1);
	/* tell the L1 to which ARFCNmanager to transmit */
	mL1->downstream(radio);

	/* If we have a SACCH, configure it the same way */
	if (mSACCHL1)
		mSACCHL1->downstream(radio);
}

OsmoCCCHLchan::OsmoCCCHLchan(OsmoTS *osmo_ts, unsigned int ss_nr)
	:OsmoNDCCHLogicalChannel(osmo_ts, ss_nr)
{
	mL1 = new CCCHL1FEC(gCCCH[ss_nr]);
	connect();
}

OsmoBCCHLchan::OsmoBCCHLchan(OsmoTS *osmo_ts)
	:OsmoNDCCHLogicalChannel(osmo_ts, 0)
{
	assert(osmo_ts->getComb() == 5);

	mL1 = new CCCHL1FEC(gBCCHMapping);
	connect();
}

OsmoSDCCHLchan::OsmoSDCCHLchan(OsmoTS *osmo_ts, unsigned int ss_nr)
	:OsmoLogicalChannel(osmo_ts, ss_nr)
{
	unsigned int ts_nr = osmo_ts->getTSnr();
	const CompleteMapping *wMapping = NULL;

	/* we have to distinguish SDCCH/4 and SDCCH/8 mappings */
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
	mSACCHL1 = new SACCHL1FEC(ts_nr, wMapping->SACCH());
	connect();
}

OsmoTCHFACCHLchan::OsmoTCHFACCHLchan(OsmoTS *osmo_ts, unsigned int ss_nr)
	:OsmoLogicalChannel(osmo_ts, ss_nr)
{
	unsigned int ts_nr = osmo_ts->getTSnr();
	mL1 = new TCHFACCHL1FEC(ts_nr, gTCHF_T[ts_nr].LCH());
	mSACCHL1 = new SACCHL1FEC(ts_nr, gTCHF_T[ts_nr].SACCH());
	connect();
}


// vim: ts=4 sw=4

