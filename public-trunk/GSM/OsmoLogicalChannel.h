/**@file Logical Channel.  */

/*
* Copyright 2008, 2009, 2010 Free Software Foundation, Inc.
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




#ifndef OSMOLOGICALCHANNEL_H
#define OSMOLOGICALCHANNEL_H

#include <sys/types.h>
#include <pthread.h>
#include <iostream>


#include "GSML1FEC.h"
#include "GSMSAPMux.h"
#include "GSMTDMA.h"
#include "OsmoSAPMux.h"

#include <TRXManager.h>
#include <Logger.h>

class ARFCNManager;



namespace GSM {

class OsmoTRX;
class OsmoThreadMuxer;
class OsmoLogicalChannel;

/* virtual class from which we derive timeslots */
class OsmoTS {
protected:
	OsmoTRX *mTRX;
	unsigned int mTSnr;
	OsmoLogicalChannel *mLchan[8];
	unsigned int mNLchan;
	unsigned int mComb;
public:
	OsmoTS(OsmoTRX &trx, unsigned int ts_nr, unsigned comb);
	unsigned int getTSnr() const { return mTSnr; }
	unsigned int getComb() { return mComb; }
	const OsmoTRX *getTRX() const { return mTRX; }
	ARFCNManager *getARFCNmgr();
	OsmoLogicalChannel *getLchan(unsigned int nr) {
		assert(nr < 8);
		if (nr < mNLchan)
			return mLchan[nr];
		else
			return NULL;
	}
};

/* One TRX (8TS) */
class OsmoTRX {
protected:
	TransceiverManager *mTRXmgr;
	OsmoThreadMuxer *mThreadMux;
	unsigned int mTN;
	OsmoTS *mTS[8];
public:
	OsmoTRX(TransceiverManager &TRXmgr, unsigned int trx_nr) {
		mTRXmgr = &TRXmgr;
		mTN = trx_nr;
	}

	TransceiverManager *getTRXmgr() const { return mTRXmgr; }
	OsmoThreadMuxer *getThreadMux() const { return mThreadMux; }
	unsigned int getTN() const { return mTN; }

	OsmoTS *getTS(unsigned int nr) {
		assert(nr < 8);
		return mTS[nr];
	}

	OsmoLogicalChannel *getLchan(unsigned int ts_nr, unsigned int lchan_nr) {
		OsmoTS *ts = getTS(ts_nr);
		return ts->getLchan(lchan_nr);
	}
};

/**
	A complete logical channel.
	Includes processors for L1, L2, L3, as needed.
	The layered structure of GSM is defined in GSM 04.01 7, as well as many other places.
	The concept of the logical channel and the channel types are defined in GSM 04.03.
	This is virtual class; specific channel types are subclasses.
*/
//class OsmoLogicalChannel : public virtual LogicalChannelCommon {
class OsmoLogicalChannel {

protected:

	L1FEC *mL1;			///< L1 forward error correction
	OsmoSAPMux mMux;		///< service access point multiplex
	OsmoThreadMuxer *mTM;
	unsigned int mSS;	// sub-slot (logical channel within TS)
	OsmoTS *mTS;
	SACCHL1FEC *mSACCHL1;	///< The associated SACCH, if any.

public:

	/**
		Blank initializer just nulls the pointers.
		Specific sub-class initializers allocate new components as needed.
	*/
	OsmoLogicalChannel(OsmoTS *osmo_ts, unsigned int ss_nr)
		:mSACCHL1(NULL),
		mL1(NULL)
	{
		mTS = osmo_ts;
		mSS = ss_nr;
		/* resolve the thread muxer */
		const OsmoTRX *otrx = osmo_ts->getTRX();
		mTM = otrx->getThreadMux();
	}

	/** Connect an ARFCN manager to link L1FEC to the radio. */
	void downstream(ARFCNManager* radio);

	/**@name Accessors. */
	//@{
	SACCHL1FEC* SACCH() { return mSACCHL1; }
	const SACCHL1FEC* SACCH() const { return mSACCHL1; }
	const OsmoTS* TS() const { return mTS; }
	unsigned int SSnr() const { return mSS; }
	//@}


	/**@name Pass-throughs. */
	//@{

	/** Set L1 physical parameters from a RACH or pre-exsting channel. */
	virtual void setPhy(float wRSSI, float wTimingError) {
		assert(mSACCHL1);
		mSACCHL1->setPhy(wRSSI, wTimingError);
	}

	/* Set L1 physical parameters from an existing logical channel. */
	virtual void setPhy(const OsmoLogicalChannel& other) {
		assert(mSACCHL1);
		mSACCHL1->setPhy(*other.SACCH());
	}

	//@}


	/**@name L3 interfaces */
	//@{
	//
	virtual void writeLowSide(const L2Frame& frame);

	//@} // passthrough

	/**@name Channel stats from the physical layer */
	//@{
	/** RSSI wrt full scale. */
	virtual float RSSI() const { return mSACCHL1->RSSI(); }
	/** Uplink timing error. */
	virtual float timingError() const { return mSACCHL1->timingError(); }
	/** Actual MS uplink power. */
	virtual int actualMSPower() const { return mSACCHL1->actualMSPower(); }
	/** Actual MS uplink timing advance. */
	virtual int actualMSTiming() const { return mSACCHL1->actualMSTiming(); }
	//@}

	/** Return the channel type. */
	virtual ChannelType type() const =0;

	/**
		Make the channel ready for a new transaction.
		The channel is closed with primitives from L3.
	*/
	virtual void open();

	protected:

	/**
		Make the normal inter-layer connections.
		Should be called from inside the constructor after
		the channel components are created.
	*/
	virtual void connect();

	friend std::ostream& operator<<(std::ostream& os, const OsmoLogicalChannel& lchan);
};

//std::ostream& GSM::operator<<(std::ostream& os, const OsmoLogicalChannel& lchan);

/**
	Standalone dedicated control channel.
	GSM 04.06 4.1.3: "A dedicated control channel (DCCH) is a point-to-point
	bi-directional or uni-directional control channel. ... A SDCCH (Stand-alone
	DCCH) is a bi-directional DCCH whose allocation is not linked to the
	allocation of a TCH.  The bit rate of a SDCCH is 598/765 kbit/s. 
"
*/
class OsmoSDCCHLchan : public OsmoLogicalChannel {

	public:

	OsmoSDCCHLchan(OsmoTS *osmo_ts, unsigned int ss_nr);

	ChannelType type() const { return SDCCHType; }
};


/**
	Logical channel for NDCCHs that use Bbis format and a pseudolength.
	This is a virtual base class this is extended for CCCH & BCCH.
	See GSM 04.06 4.1.1, 4.1.3.
*/
class OsmoNDCCHLogicalChannel : public OsmoLogicalChannel {

	public:
	OsmoNDCCHLogicalChannel(OsmoTS *osmo_ts, unsigned int ss_nr) :
		OsmoLogicalChannel(osmo_ts, ss_nr) { };

};


/**
	Common control channel.
	The "uplink" component of the CCCH is the RACH.
	See GSM 04.03 4.1.2: "A common control channel is a point-to-multipoint
	bi-directional control channel. Common control channels are physically
	sub-divided into the common control channel (CCCH), the packet common control
	channel (PCCCH), and the Compact packet common control channel (CPCCCH)."
*/
class OsmoCCCHLchan : public OsmoNDCCHLogicalChannel {
	public:
	OsmoCCCHLchan(OsmoTS *osmo_ts, unsigned int ss_nr);

	ChannelType type() const { return CCCHType; }
};

class OsmoTCHFACCHLchan : public OsmoLogicalChannel {

	protected:

	TCHFACCHL1FEC * mTCHL1;

	public:

	OsmoTCHFACCHLchan(OsmoTS *osmo_ts, unsigned int ss_nr);

	ChannelType type() const { return FACCHType; }

	void sendTCH(const unsigned char* frame)
		{ assert(mTCHL1); mTCHL1->sendTCH(frame); }

	unsigned char* recvTCH()
		{ assert(mTCHL1); return mTCHL1->recvTCH(); }

	unsigned queueSize() const
		{ assert(mTCHL1); return mTCHL1->queueSize(); }

	bool radioFailure() const
		{ assert(mTCHL1); return mTCHL1->radioFailure(); }
};

//@}

/* timeslot in Combination I (TCH/F) */
class OsmoComb1TS : public OsmoTS {
protected:
public:
	OsmoComb1TS(OsmoTRX &trx, unsigned int ts_nr) :OsmoTS(trx, ts_nr, 1) {
		ARFCNManager* radio = getARFCNmgr();
		/* create logical channel */
		OsmoTCHFACCHLchan * chan = new OsmoTCHFACCHLchan(this, 0);
		chan->downstream(radio);
		chan->open();
		mLchan[0] = chan;
		mNLchan = 1;
	}
};

/* timeslot in Combination 5 (FCCH, SCH, CCCH, BCCH and 4*SDCCH/4) */
class OsmoComb5TS : public OsmoTS {
protected:
	SCHL1FEC mSCH;
	FCCHL1FEC mFCCH;
	RACHL1FEC mRACH;
	OsmoCCCHLchan *mCCCH[3];
public:
	OsmoComb5TS(OsmoTRX &trx, unsigned int tn)
		:OsmoTS(trx, tn, 5),
		mSCH(),
		mFCCH(),
		mRACH(gRACHC5Mapping) {
		ARFCNManager* radio = getARFCNmgr();

		mSCH.downstream(radio);
		mSCH.open();

		mFCCH.downstream(radio);
		mFCCH.open();

		mRACH.downstream(radio);
		mRACH.open();

		for (int i = 0; i < 3; i++) {
			mCCCH[i] = new OsmoCCCHLchan(this, i);
			mCCCH[i]->downstream(radio);
			mCCCH[i]->open();
		}

		for (int i = 0; i < 4; i++) {
			/* create logical channel */
			OsmoSDCCHLchan * chan = new OsmoSDCCHLchan(this, i);
			chan->downstream(radio);
			chan->open();
			mLchan[i] = chan;
			mNLchan++;

		}
	}
};

/* timeslot in Combination 7 (SDCCH/8) */
class OsmoComb7TS : public OsmoTS {
protected:
public:
	OsmoComb7TS(OsmoTRX &trx, unsigned int tn) :OsmoTS(trx, tn, 7) {
		for (unsigned int i = 0; i < 8; i++) {
			ARFCNManager* radio = getARFCNmgr();
			/* create logical channel */
			OsmoSDCCHLchan *chan = new OsmoSDCCHLchan(this, i);
			chan->downstream(radio);
			chan->open();
			mLchan[i] = chan;
			mNLchan++;
		}
	}
};



};		// GSM

#endif


// vim: ts=4 sw=4
