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
	mFCCH = NULL;
	mBCCH = NULL;
	mSCH = NULL;
	mRACH = NULL;
	mAGCH = NULL;
	mPCH = NULL;

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

/* This is where OsmoThreadMuxer inputs data received from osmo-bts */
void OsmoLogicalChannel::writeHighSide(const L2Frame& frame)
{
	OBJLOG(DEEPDEBUG) << "OsmoLogicalChannel::writeHighSide " << frame;
	/* simply pass it through to the OsmoSAPMux */
	mMux.writeHighSide(frame);
}

/* This is where OsmoSAPMux inputs data received from L1FEC */
void OsmoLogicalChannel::writeLowSide(const L2Frame& frame, 
	const GSM::Time time, const float RSSI, const int TA)
{
	/* simply pass it through to the OsmoThreadMuxer, including
	 * a reference to us (the logical channel) */
	mTM->writeLowSide(frame, time, RSSI, TA, this);
}

void OsmoTCHFACCHLchan::writeLowSide(const unsigned char* frame, 
	const GSM::Time time, const float RSSI, const int TA)
{
	mTM->writeLowSideTCH(frame, time, RSSI, TA, this);
}

ostream& GSM::operator<<(ostream& os, const OsmoLogicalChannel& lchan)
{
	unsigned int trx_nr, ts_nr, ss_nr;
	trx_nr = lchan.TS()->getTRX()->getTN();
	ts_nr = lchan.TS()->getTSnr();
	ss_nr = lchan.SSnr();

	/* Just dump something like "(hL2,TRX,TS,SS,Type)" identifying the lchan */
	os << "(" << lchan.getHL2() << "," << trx_nr << "," << ts_nr << "," << 
		ss_nr << "," << lchan.type() << ")";
}

void OsmoLogicalChannel::signalNextWtime(GSM::Time &time)
{
	if (mTM)
		mTM->signalNextWtime(time, *this);
}

void OsmoLogicalChannel::downstream(ARFCNManager* radio)
{
	assert(mL1);
	/* tell the L1 to which ARFCNmanager to transmit */
	mL1->downstream(radio);
}

OsmoCCCHLchan::OsmoCCCHLchan(OsmoTS *osmo_ts, unsigned int ss_nr)
	:OsmoLogicalChannel(osmo_ts, ss_nr)
{
	mType = CCCHType;
	mL1 = new CCCHL1FEC(gCCCH[ss_nr]);
	connect();
}

OsmoBCCHLchan::OsmoBCCHLchan(OsmoTS *osmo_ts)
	:OsmoLogicalChannel(osmo_ts, 0)
{
	assert(osmo_ts->getComb() == 5);

	mL1 = new BCCHL1FEC();
	connect();
}

OsmoSCHLchan::OsmoSCHLchan(OsmoTS *osmo_ts)
	:OsmoLogicalChannel(osmo_ts, 0)
{
	assert(osmo_ts->getComb() == 5);

	mL1 = new SCHL1FEC();
	connect();
}

OsmoRACHLchan::OsmoRACHLchan(OsmoTS *osmo_ts)
	:OsmoLogicalChannel(osmo_ts, 0)
{
	assert(osmo_ts->getComb() == 5);

	mL1 = new RACHL1FEC(gRACHC5Mapping);
	connect();
}

OsmoSACCHLchan::OsmoSACCHLchan(OsmoTS *osmo_ts, unsigned int ss_nr)
	:OsmoLogicalChannel(osmo_ts, ss_nr)
{
	unsigned int ts_nr = osmo_ts->getTSnr();
	const CompleteMapping *wMapping = NULL;

	/* we have to distinguish TCH/F, SDCCH/4, and SDCCH/8 mappings */
	switch (osmo_ts->getComb()) {
	case 1:
		wMapping = &gTCHF_T[ts_nr];
		break;
	case 5:
		wMapping = &gSDCCH4[ss_nr];
		break;
	case 7:
		wMapping = &gSDCCH8[ss_nr];
		break;
	default:
		assert(0);
	}
	mSACCHL1 = new SACCHL1FEC(ts_nr, wMapping->SACCH());
	mL1 = mSACCHL1;
	connect();

	mSiblingLchan = NULL;
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
	connect();
}

OsmoTCHFACCHLchan::OsmoTCHFACCHLchan(OsmoTS *osmo_ts, unsigned int ss_nr)
	:OsmoLogicalChannel(osmo_ts, ss_nr)
{
	unsigned int ts_nr = osmo_ts->getTSnr();
	mTCHL1 = new TCHFACCHL1FEC(ts_nr, gTCHF_T[ts_nr].LCH());
	mL1 = mTCHL1;
	connect();
}

// These have to go into the .cpp file to prevent an illegal forward reference.
void OsmoLogicalChannel::setPhy(float wRSSI, float wTimingError) {
	assert(mSACCHLchan);
	mSACCHLchan->setPhy(wRSSI, wTimingError);
}
void OsmoLogicalChannel::setPhy(const OsmoLogicalChannel& other) {
	assert(mSACCHLchan);
	mSACCHLchan->setPhy(*other.SACCH());
}
float OsmoLogicalChannel::RSSI() const { return mSACCHLchan->RSSI(); }
float OsmoLogicalChannel::timingError() const { return mSACCHLchan->timingError(); }
int OsmoLogicalChannel::actualMSPower() const { return mSACCHLchan->actualMSPower(); }
int OsmoLogicalChannel::actualMSTiming() const { return mSACCHLchan->actualMSTiming(); }

// vim: ts=4 sw=4

