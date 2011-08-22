/**@file Messages for call independent Supplementary Service Control, GSM 04.80 2.2. */
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



#ifndef GSML3NONCALLSSMESSAGES_H
#define GSML3NONCALLSSMESSAGES_H

#include "GSMCommon.h"
#include "GSML3Message.h"
#include "GSML3NonCallSSElements.h"
#include "GSML3NonCallSSComponents.h"



namespace GSM { 

/**
This a virtual class for L3  Messages for call independent Supplementary Service Control.
These messages are defined in GSM 04.80 2.2.
*/
class L3NonCallSSMessage : public L3Message {

	private:

	unsigned mTIValue;		///< short transaction ID, GSM 04.07 11.2.3.1.3.
	unsigned mTIFlag;		///< 0 for originating side, see GSM 04.07 11.2.3.1.3.

	public:

	/** GSM 04.80, Table 3.1 */
	enum MessageType {
		/**@name Clearing message */
		//@{
		ReleaseComplete=0x2a,
		//@}
		/**@name Miscellaneous message group */
		//@{
		Facility=0x3a,
		Register=0x3b
		//@}
	};

	L3NonCallSSMessage(unsigned wTIFlag, unsigned wTIValue)
		:L3Message(),mTIValue(wTIValue),mTIFlag(wTIFlag)
	{}

	/** Override the write method to include transaction identifiers in header. */
	void write(L3Frame& dest) const;
	L3PD PD() const { return L3NonCallSSPD; }
	unsigned TIValue() const { return mTIValue; }
	void TIValue(unsigned wTIValue) { mTIValue = wTIValue; }
	unsigned TIFlag() const { return mTIFlag; }
	void TIFlag(unsigned wTIFlag) { mTIFlag = wTIFlag; }
	void text(std::ostream&) const;

};

std::ostream& operator<<(std::ostream& os, const GSM::L3NonCallSSComponent& msg);
std::ostream& operator<<(std::ostream& os, L3NonCallSSMessage::MessageType MTI);



/**
Parse a complete L3 call independent supplementary service control message into its object type.
@param source The L3 bits.
@return A pointer to a new message or NULL on failure.
*/
L3NonCallSSMessage* parseL3NonCallSS(const L3Frame& source);

/**
A Factory function to return a L3NonCallSSMessage of the specified MTI.
Returns NULL if the MTI is not supported.
*/
L3NonCallSSMessage* L3NonCallSSFactory(L3NonCallSSMessage::MessageType MTI);
	
/** Facility Message GSM 04.80 2.3. */
class L3NonCallSSFacilityMessage : public L3NonCallSSMessage {

	private:
	
	L3NonCallSSComponent* mL3NonCallSSComponent;

	public:

	L3NonCallSSFacilityMessage(unsigned wTIFlag = 0, unsigned wTIValue = 7, 
		L3NonCallSSComponent* wL3NonCallSSComponent = NULL)
		:L3NonCallSSMessage(wTIFlag,wTIValue),
		mL3NonCallSSComponent (wL3NonCallSSComponent)
	{}

	L3NonCallSSComponent* l3NonCallSSComponent() const {return mL3NonCallSSComponent;}	
	int MTI() const { return Facility; }
	void writeBody( L3Frame &dest, size_t &wp ) const;
	void parseBody( const L3Frame &src, size_t &rp );
	size_t bodyLength() const {return mL3NonCallSSComponent->length()+1;}
	void text(std::ostream& os) const;

};

/** Register Message GSM 04.80 2.4. */
class L3NonCallSSRegisterMessage : public L3NonCallSSMessage {

	private:

	L3NonCallSSComponent* mL3NonCallSSComponent;
	L3NonCallSSVersionIndicator mSSVersionIndicator;
	bool mHaveSSVersionIndicator;
	
	public:
	
	L3NonCallSSRegisterMessage(unsigned wTIFlag = 0, unsigned wTIValue = 7, 
		L3NonCallSSComponent* wL3NonCallSSComponent = NULL)
		:L3NonCallSSMessage(wTIFlag,wTIValue),
		mL3NonCallSSComponent (wL3NonCallSSComponent),
		mHaveSSVersionIndicator(false)
	{}

	L3NonCallSSComponent* l3NonCallSSComponent() const {return mL3NonCallSSComponent;}
	const L3NonCallSSVersionIndicator& SSVersionIndicator() const {assert(mHaveSSVersionIndicator); return mSSVersionIndicator;}
	bool haveL3NonCallSSVersionIndicator() const {return mHaveSSVersionIndicator;}	
	int MTI() const { return Register; }
	void writeBody( L3Frame &dest, size_t &wp ) const;
	void parseBody( const L3Frame &src, size_t &rp );
	size_t bodyLength() const;
	void text(std::ostream&) const;

};

/** Release Complete Message GSM 04.80 2.5. */
class L3NonCallSSReleaseCompleteMessage : public L3NonCallSSMessage {

	private:

	L3NonCallSSComponent* mL3NonCallSSComponent;
	bool mHaveL3NonCallSSComponent;
	L3NonCallSSCause mCause;
	bool mHaveCause;

	public:

	L3NonCallSSReleaseCompleteMessage(unsigned wTIFlag = 0, unsigned wTIValue = 7)
		:L3NonCallSSMessage(wTIFlag,wTIValue),
		mHaveL3NonCallSSComponent(false),
		mHaveCause(false)
	{}

	void component (L3NonCallSSComponent* Component) {mL3NonCallSSComponent = Component; mHaveL3NonCallSSComponent = true;}
	void cause (L3NonCallSSCause Cause) {mCause = Cause; mHaveCause = true;}

	L3NonCallSSComponent* l3NonCallSSComponent() const {assert(mHaveL3NonCallSSComponent); return mL3NonCallSSComponent;}
	bool haveL3NonCallSSComponent() const {return mHaveL3NonCallSSComponent;}
	const L3NonCallSSCause& l3NonCallSSCause() const {assert(mHaveCause); return mCause;}
	bool haveL3NonCallSSCause() const {return mHaveCause;}
	int MTI() const { return ReleaseComplete; }
	void writeBody( L3Frame &dest, size_t &wp ) const;
	void parseBody( const L3Frame &src, size_t &rp );
	size_t bodyLength() const;
	void text(std::ostream&) const;

};

}
#endif
