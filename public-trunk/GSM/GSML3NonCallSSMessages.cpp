/** @file Messages for call independent Supplementary Service Control, GSM 04.80 2.2.  */
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



#include <iostream>
#include "GSML3NonCallSSMessages.h"
#include <Logger.h>


using namespace std;
using namespace GSM;


ostream& operator<<(ostream& os, const L3NonCallSSMessage& msg)
{
	msg.text(os);
	return os;
}

ostream& GSM::operator<<(ostream& os, L3NonCallSSMessage::MessageType val)
{
	switch (val) {
		case L3NonCallSSMessage::ReleaseComplete: 
			os << "ReleaseComplete"; break;
		case L3NonCallSSMessage::Facility:
			os << "Facility"; break;
		case L3NonCallSSMessage::Register:
			os << "Register"; break;
		default: os << hex << "0x" << (int)val << dec;
	}
	return os;
}

L3NonCallSSMessage * GSM::L3NonCallSSFactory(L3NonCallSSMessage::MessageType MTI)
{
	LOG(DEBUG) << "Factory MTI"<< (int)MTI;
	switch (MTI) {
		case L3NonCallSSMessage::Facility: return new L3NonCallSSFacilityMessage();
		case L3NonCallSSMessage::Register: return new L3NonCallSSRegisterMessage();
		case L3NonCallSSMessage::ReleaseComplete: return new L3NonCallSSReleaseCompleteMessage();
		default: {
			//LOG(NOTICE) << "no L3 NonCallSS factory support for message "<< MTI;
			return NULL;
		}
	}
}

L3NonCallSSMessage * GSM::parseL3NonCallSS(const L3Frame& source)
{
	// mask out bit #7 (1011 1111) so use 0xbf
	L3NonCallSSMessage::MessageType MTI = (L3NonCallSSMessage::MessageType)(0xbf & source.MTI());
	LOG(DEBUG) << "MTI= "<< (int)MTI;
	L3NonCallSSMessage *retVal = L3NonCallSSFactory(MTI);
	if (retVal==NULL) return NULL;
	retVal->TIValue(source.TIValue());
	retVal->TIFlag(source.TIFlag());
	retVal->parse(source);
	LOG(DEBUG) << "parse L3 Non Call SS Message" << *retVal;
	return retVal;
}

void L3NonCallSSMessage::write(L3Frame& dest) const
{
	// We override L3Message::write for the transaction identifier.
	size_t l3len = bitsNeeded();
	if (dest.size()!=l3len) dest.resize(l3len);
	size_t wp = 0;
	dest.writeField(wp,mTIFlag,1);
	dest.writeField(wp,mTIValue,3);
	dest.writeField(wp,PD(),4);
	dest.writeField(wp,MTI(),8);
	writeBody(dest,wp);
}

void L3NonCallSSMessage::text(ostream& os) const
{
	os << " MessageType = " <<(MessageType) MTI();
	os << " TI = (" << mTIFlag << "," << mTIValue << ")";
}

void L3NonCallSSFacilityMessage::writeBody( L3Frame &dest, size_t &wp ) const
{
	dest.writeField(wp,mL3NonCallSSComponent->length(),8);
	mL3NonCallSSComponent->write(dest, wp);
}

void L3NonCallSSFacilityMessage::parseBody( const L3Frame &src, size_t &rp )
{
	rp+=8;
	mL3NonCallSSComponent = parseL3NonCallSSComponent(src, rp);
}

void L3NonCallSSFacilityMessage::text(ostream& os) const
{
	L3NonCallSSMessage::text(os);
	os << " L3NonCallSSComponent = (" << *mL3NonCallSSComponent << ")";
}

void L3NonCallSSRegisterMessage::writeBody( L3Frame &dest, size_t &wp ) const
{
	dest.writeField(wp,0x1c,8);
	dest.writeField(wp,mL3NonCallSSComponent->length(),8);
	mL3NonCallSSComponent->write(dest, wp);
}

void L3NonCallSSRegisterMessage::parseBody( const L3Frame &src, size_t &rp )
{
	rp+=16;
	mL3NonCallSSComponent = parseL3NonCallSSComponent(src, rp);
	if((src.length() > mL3NonCallSSComponent->length()+2) && (src.readField(rp,8) == 0x7f))
	{ 
		rp+=8;
		mSSVersionIndicator.parseV(src, rp);
		mHaveSSVersionIndicator = true;
	}
}

size_t L3NonCallSSRegisterMessage::bodyLength() const 
{	
	size_t bodyLength;
	bodyLength = mL3NonCallSSComponent->length()+2;
	if(mHaveSSVersionIndicator) bodyLength+=3;
	return bodyLength;
}

void L3NonCallSSRegisterMessage::text(ostream& os) const
{
	L3NonCallSSMessage::text(os);
	os << " L3NonCallSSComponent = (" << *mL3NonCallSSComponent<< ")";
	if(mHaveSSVersionIndicator) os << " L3SSVersionIndicator = " << mSSVersionIndicator;
}

void L3NonCallSSReleaseCompleteMessage::writeBody( L3Frame &dest, size_t &wp ) const
{
	if(mHaveCause)
	{
		dest.writeField(wp,0x08,8);
		dest.writeField(wp,mCause.lengthV(),8);
		mCause.writeV(dest, wp);
	}
	if(mHaveL3NonCallSSComponent)
	{
		dest.writeField(wp,0x1c,8);
		dest.writeField(wp,mL3NonCallSSComponent->length(),8);
		mL3NonCallSSComponent->write(dest, wp);
	}
}

void L3NonCallSSReleaseCompleteMessage::parseBody( const L3Frame &src, size_t &rp )
{
	
	if (src.length()>2)
	{
		if(mCause.parseTLV(0x08, src,rp))
		{
			mHaveCause = true;
			if ((src.length() > mCause.lengthTLV()+2)&&(src.readField(rp,8) == 0x1c))
			{
				rp+=8;
				mL3NonCallSSComponent = parseL3NonCallSSComponent(src, rp);
				mHaveL3NonCallSSComponent = true;
			}
		}
		else if (src.readField(rp,8) == 0x1c)
		{
			rp+=8;
			mL3NonCallSSComponent = parseL3NonCallSSComponent(src, rp);
			mHaveL3NonCallSSComponent = true;
		}
	}
}

size_t L3NonCallSSReleaseCompleteMessage::bodyLength() const 
{	
	size_t bodyLength = 0;
	if(mHaveCause) bodyLength+=mCause.lengthTLV();
	if(mHaveL3NonCallSSComponent) bodyLength+=mL3NonCallSSComponent->length()+2;
	return bodyLength;
}

void L3NonCallSSReleaseCompleteMessage::text(ostream& os) const
{
	L3NonCallSSMessage::text(os);
	if(mHaveL3NonCallSSComponent) os << " L3NonCallSSComponent = (" << *mL3NonCallSSComponent << ")";
	if(mHaveCause) os << " L3Cause = " << mCause;
}

