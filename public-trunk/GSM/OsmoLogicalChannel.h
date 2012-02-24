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
class OsmoBCCHLchan;
class OsmoSCHLchan;
class OsmoRACHLchan;
class OsmoCCCHLchan;
class OsmoSACCHLchan;

/* virtual class from which we derive timeslots */
class OsmoTS {
protected:
	OsmoTRX *mTRX;
	unsigned int mTSnr;
	FCCHL1FEC *mFCCH;
	OsmoBCCHLchan *mBCCH;
	OsmoSCHLchan *mSCH;
	OsmoRACHLchan *mRACH;
	OsmoCCCHLchan *mCCCH[3];
	OsmoCCCHLchan *mAGCH;
	OsmoCCCHLchan *mPCH;
	/* Only used for SDCCH/TCH */
	OsmoLogicalChannel *mLchan[8];
	unsigned int mNLchan;
	unsigned int mComb;
public:
	OsmoTS(OsmoTRX &trx, unsigned int ts_nr, unsigned comb);
	unsigned int getTSnr() const { return mTSnr; }
	unsigned int getComb() const { return mComb; }
	const OsmoTRX *getTRX() const { return mTRX; }
	ARFCNManager *getARFCNmgr();
	OsmoLogicalChannel *getLchan(unsigned int nr) {
		assert(nr < 8);
		if (nr < mNLchan)
			return mLchan[nr];
		else
			return NULL;
	}

	FCCHL1FEC *getFCCHL1()
	{
		return mFCCH;
	}

	OsmoBCCHLchan *getBCCHLchan()
	{
		return mBCCH;
	}

	OsmoSCHLchan *getSCHLchan()
	{
		return mSCH;
	}

	OsmoRACHLchan *getRACHLchan()
	{
		return mRACH;
	}

	OsmoCCCHLchan *getAGCHLchan()
	{
		return mAGCH;
	}

	OsmoCCCHLchan *getPCHLchan()
	{
		return mPCH;
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
	OsmoTRX(TransceiverManager &TRXmgr, unsigned int trx_nr,
		OsmoThreadMuxer *thread_mux)
	{
		mTRXmgr = &TRXmgr;
		mTN = trx_nr;
		mThreadMux = thread_mux;

		for(int i = 0; i < 8; i++)
		{
			mTS[i] = NULL;
		}
	}

	TransceiverManager *getTRXmgr() const { return mTRXmgr; }
	OsmoThreadMuxer *getThreadMux() const { return mThreadMux; }
	unsigned int getTN() const { return mTN; }

	void addTS(OsmoTS &ts)
	{
		unsigned int ts_nr = ts.getTSnr();
		assert(ts_nr < 8);
		/* Do not overwrite previously configured TS */
		assert(mTS[ts_nr] == NULL);
		mTS[ts_nr] = &ts;
	}

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
	OsmoSACCHLchan *mSACCHLchan;	///< The associated SACCH, if any.
	int mHL2; //hLayer2 reference, initialized by osmo-bts

public:

	/**
		Blank initializer just nulls the pointers.
		Specific sub-class initializers allocate new components as needed.
	*/
	OsmoLogicalChannel(OsmoTS *osmo_ts, unsigned int ss_nr)
		:mL1(NULL),
		mSACCHLchan(NULL)
	{
		mTS = osmo_ts;
		mSS = ss_nr;
		/* resolve the thread muxer */
		const OsmoTRX *otrx = osmo_ts->getTRX();
		mTM = otrx->getThreadMux();

		mHL2 = -1; //uninitialized value
	}

	/** Connect an ARFCN manager to link L1FEC to the radio. */
	void downstream(ARFCNManager* radio);

	/* Link a SACCH to this Lchan */
	void setSACCHLchan(OsmoSACCHLchan* chan)
	{
		assert(mSACCHLchan == NULL);
		mSACCHLchan = chan;
	}

	/* Only to be called by SACCH Lchan */
	virtual OsmoLogicalChannel *getSiblingLchan() { assert(0); }

	/**@name Accessors. */
	//@{
	L1FEC* getL1() { return mL1; }
	OsmoSACCHLchan* SACCH() { return mSACCHLchan; }
	const OsmoSACCHLchan* SACCH() const { return mSACCHLchan; }
	const OsmoTS* TS() const { return mTS; }
	unsigned int SSnr() const { return mSS; }
	int getHL2() const { return mHL2; }
	//@}

	void clearHL2() { mHL2 = -1; }
	void initHL2(const int hLayer2)
	{
		assert(mHL2 == -1);
		mHL2 = hLayer2;
	}
	bool hasHL2() const
	{
		if(mHL2 == -1)
		{
			LOG(ERROR) << "Lchan " << *this << " has no hLayer2 initialized!";
			return false;
		}

		return true;
	}

	virtual void writeHighSide(const L2Frame& frame);
	virtual void writeLowSide(const L2Frame& frame, const GSM::Time time, 
		const float RSSI, const int TA);
	virtual void signalNextWtime(GSM::Time &time);
	
	/* Only used by TCHFACCH Lchan */
	virtual void sendTCH(const unsigned char* frame) { }
	virtual void writeLowSide(const unsigned char* frame, const GSM::Time time, 
		const float RSSI, const int TA) { }

	/**@name Pass-throughs. */
	//@{

	/** Set L1 physical parameters from a RACH or pre-exsting channel. */
	virtual void setPhy(float wRSSI, float wTimingError);

	/* Set L1 physical parameters from an existing logical channel. */
	virtual void setPhy(const OsmoLogicalChannel& other);

	//@} // passthrough

	/**@name Channel stats from the physical layer */
	//@{
	/** RSSI wrt full scale. */
	virtual float RSSI() const;
	/** Uplink timing error. */
	virtual float timingError() const;
	/** Actual MS uplink power. */
	virtual int actualMSPower() const;
	/** Actual MS uplink timing advance. */
	virtual int actualMSTiming() const;
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

std::ostream& operator<<(std::ostream& os, const OsmoLogicalChannel& lchan);

class OsmoSACCHLchan : public OsmoLogicalChannel {
	protected:

	SACCHL1FEC *mSACCHL1;
	OsmoLogicalChannel *mSiblingLchan;

	public:

	OsmoSACCHLchan(OsmoTS *osmo_ts, unsigned int ss_nr);

	ChannelType type() const { return SACCHType; }

	/* Link a Lchan to this SACCH*/
	void setSiblingLchan(OsmoLogicalChannel *chan)
	{
		assert(mSiblingLchan == NULL);
		mSiblingLchan = chan;

		LOG(INFO) << "sibling " << *chan << " set to " << *this;
	}

	virtual OsmoLogicalChannel *getSiblingLchan()
	{
		return mSiblingLchan;
	}

	/* Do not link a SACCH to another SACCH */
	virtual void setSACCHLchan(OsmoSACCHLchan* chan) { assert(0); }

	void setPhy(float RSSI, float timingError) { mSACCHL1->setPhy(RSSI,timingError); }
	void setPhy(const OsmoSACCHLchan& other) { mSACCHL1->setPhy(*other.mSACCHL1); }
};

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
	Common control channel.
	The "uplink" component of the CCCH is the RACH.
	See GSM 04.03 4.1.2: "A common control channel is a point-to-multipoint
	bi-directional control channel. Common control channels are physically
	sub-divided into the common control channel (CCCH), the packet common control
	channel (PCCCH), and the Compact packet common control channel (CPCCCH)."
*/
class OsmoCCCHLchan : public OsmoLogicalChannel {
	private:
	ChannelType mType;

	public:
	OsmoCCCHLchan(OsmoTS *osmo_ts, unsigned int ss_nr);

	ChannelType type() const { return mType; }
	void setType(const ChannelType type)
	{
		assert(type == CCCHType || type == AGCHType || type == PCHType);
		mType = type;
	}
};

class OsmoBCCHLchan : public OsmoLogicalChannel {
	public:
	OsmoBCCHLchan(OsmoTS *osmo_ts);

	ChannelType type() const { return BCCHType; }
};

class OsmoSCHLchan : public OsmoLogicalChannel {
	public:
	OsmoSCHLchan(OsmoTS *osmo_ts);

	ChannelType type() const { return SCHType; }
};

class OsmoRACHLchan : public OsmoLogicalChannel {
	public:
	OsmoRACHLchan(OsmoTS *osmo_ts);

	ChannelType type() const { return RACHType; }
};

class OsmoTCHFACCHLchan : public OsmoLogicalChannel {

	protected:

	TCHFACCHL1FEC * mTCHL1;

	public:

	OsmoTCHFACCHLchan(OsmoTS *osmo_ts, unsigned int ss_nr);

	/* No TCHHType supported */
	ChannelType type() const { return TCHFType; }

	virtual void sendTCH(const unsigned char* frame)
		{ assert(mTCHL1); mTCHL1->sendTCH(frame); }

	virtual void writeLowSide(const unsigned char* frame, const GSM::Time time, 
		const float RSSI, const int TA);
};

//@}

/* timeslot in Combination I (TCH/F) */
class OsmoComb1TS : public OsmoTS {
protected:
public:
	OsmoComb1TS(OsmoTRX &trx, unsigned int ts_nr) :OsmoTS(trx, ts_nr, 1) {
		/* Add TS to TRX */
		trx.addTS(*this);

		ARFCNManager* radio = getARFCNmgr();
		/* create logical channel */
		OsmoTCHFACCHLchan * chan = new OsmoTCHFACCHLchan(this, 0);
		chan->downstream(radio);
		mLchan[0] = chan;
		mNLchan = 1;

		/* create associated SACCH */
		OsmoSACCHLchan * achan = new OsmoSACCHLchan(this, 0);
		achan->downstream(radio);

		/* link TCH/FACCH and SACCH */
		chan->setSACCHLchan(achan);
		achan->setSiblingLchan(chan);
	}
};

/* timeslot in Combination 5 (FCCH, SCH, CCCH, BCCH and 4*SDCCH/4) */
class OsmoComb5TS : public OsmoTS {
protected:
public:
	OsmoComb5TS(OsmoTRX &trx, unsigned int tn)
		:OsmoTS(trx, tn, 5)
	{
		/* Add TS to TRX */
		trx.addTS(*this);

		ARFCNManager* radio = getARFCNmgr();

		mBCCH = new OsmoBCCHLchan(this);
		mBCCH->downstream(radio);

		mSCH = new OsmoSCHLchan(this);
		mSCH->downstream(radio);

		mFCCH = new FCCHL1FEC();
		mFCCH->downstream(radio);

		mRACH = new OsmoRACHLchan(this);
		mRACH->downstream(radio);

		for (int i = 0; i < 3; i++) {
			mCCCH[i] = new OsmoCCCHLchan(this, i);
			mCCCH[i]->downstream(radio);
		}

		/* Setup 1 AGCH and PCH each from the CCCH pool */
		mAGCH = mCCCH[0];
		mAGCH->setType(AGCHType);
		mPCH = mCCCH[2];
		mPCH->setType(PCHType);

		for (int i = 0; i < 4; i++) {
			/* create logical channel */
			OsmoSDCCHLchan * chan = new OsmoSDCCHLchan(this, i);
			chan->downstream(radio);
			mLchan[i] = chan;
			mNLchan++;

			/* create associated SACCH */
			OsmoSACCHLchan * achan = new OsmoSACCHLchan(this, i);
			achan->downstream(radio);

			/* link SDCCH and SACCH */
			chan->setSACCHLchan(achan);
			achan->setSiblingLchan(chan);
		}
	}
};

/* timeslot in Combination 7 (SDCCH/8) */
class OsmoComb7TS : public OsmoTS {
protected:
public:
	OsmoComb7TS(OsmoTRX &trx, unsigned int tn) :OsmoTS(trx, tn, 7) {
		/* Add TS to TRX */
		trx.addTS(*this);

		for (unsigned int i = 0; i < 8; i++) {
			ARFCNManager* radio = getARFCNmgr();
			/* create logical channel */
			OsmoSDCCHLchan *chan = new OsmoSDCCHLchan(this, i);
			chan->downstream(radio);
			mLchan[i] = chan;
			mNLchan++;

			/* create associated SACCH */
			OsmoSACCHLchan * achan = new OsmoSACCHLchan(this, i);
			achan->downstream(radio);

			/* link SDCCH and SACCH */
			chan->setSACCHLchan(achan);
			achan->setSiblingLchan(chan);
		}
	}
};



};		// GSM

#endif


// vim: ts=4 sw=4
