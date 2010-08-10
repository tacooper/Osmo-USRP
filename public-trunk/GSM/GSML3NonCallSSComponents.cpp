/**@file Call independent Supplementary Service Control. Facility element components, GSM 04.80 3.6.1.  */

/*
* Copyright 2008, 2009 Free Software Foundation, Inc.
*
* This software is distributed under the terms of the GNU Public License.
* See the COPYING file in the main directory for details.
*
* This use of this software may be subject to additional restrictions.
* See the LEGAL file in the main directory for details.

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.

*/


#include <iostream>
#include "GSML3NonCallSSComponents.h"
#include <Logger.h>



using namespace std;
using namespace GSM;


ostream& GSM::operator<<(ostream& os, const L3NonCallSSComponent& msg)
{
	msg.text(os);
	return os;
}

ostream& GSM::operator<<(std::ostream& os, L3NonCallSSComponent::ComponentTypeTag val)
{
	switch (val) {
		case L3NonCallSSComponent::Invoke: 
			os << "Invoke"; break;
		case L3NonCallSSComponent::ReturnResult:
			os << "ReturnResult"; break;
		case L3NonCallSSComponent::ReturnError:
			os << "ReturnError"; break;
		case L3NonCallSSComponent::Reject:
			os << "Reject"; break;
		default: os << hex << "0x" << (int)val << dec;
	}

	return os;
}

L3NonCallSSComponent * GSM::L3NonCallSSComponentFactory(L3NonCallSSComponent::ComponentTypeTag CTT)
{
	switch (CTT) {
		case L3NonCallSSComponent::Invoke: return new L3NonCallSSComponentInvoke();
		case L3NonCallSSComponent::ReturnResult: return new L3NonCallSSComponentReturnResult();
		case L3NonCallSSComponent::ReturnError: return new L3NonCallSSComponentReturnError();
		case L3NonCallSSComponent::Reject: return new L3NonCallSSComponentReject();
		default: {
			//LOG(NOTICE) << "no L3 NonCallSSComponent factory support for message "<< CTT;
			return NULL;
		}
	}
}

L3NonCallSSComponent * GSM::parseL3NonCallSSComponent (const L3Frame& source, size_t &rp)
{
	L3NonCallSSComponent::ComponentTypeTag CTT = (L3NonCallSSComponent::ComponentTypeTag)source.readField(rp, 8);
	LOG(DEBUG) << "Component Type Tag = "<<(int)CTT;

	L3NonCallSSComponent *retVal = L3NonCallSSComponentFactory(CTT);
	if (retVal==NULL) return NULL;

	retVal->parse(source,rp);
	LOG(DEBUG) << "parse L3 Non Call SS Component " << *retVal;
	return retVal;
}

void L3NonCallSSComponent::write(L3Frame& dest, size_t &wp)
{
	dest.writeField(wp,NonCallSSComponentTypeTag(),8);//Component type tag
	writeBody(dest,wp);
}

void L3NonCallSSComponent::parse(const L3Frame& source, size_t &rp)
{
	mComponentLength.parseV(source,rp);//ComponentLength
	rp+=16;//InvokeID Tag and InvokeIDLength
	mInvokeID.parseV(source, rp, 1);//InvokeID
	parseBody(source, rp);
}

void L3NonCallSSComponent::text(ostream& os) const
{
	os << " ComponentTypeTag = " << NonCallSSComponentTypeTag();
	os << " InvokeID = " << mInvokeID;
}


void L3NonCallSSComponentInvoke::writeBody( L3Frame &dest, size_t &wp )
{
	this->length();	
	mComponentLength.writeV(dest, wp);//ComponentLength
	dest.writeField(wp,0x2,8);//InvokeID tag
	dest.writeField(wp,0x1,8);//InvokeIDLength
	mInvokeID.writeV(dest,wp,8);//InvokeID	
	if (mHaveLinkedID)
	{
		dest.writeField(wp,0x80,8);//LinkedID tag
		dest.writeField(wp,0x1,8);//LinkedIDLength
		mLinkedID.writeV(dest, wp, 8);//LinkedID
	}
	dest.writeField(wp,0x2,8);//Operation Cod tag
	dest.writeField(wp,0x1,8);//Operation Cod length 
	mOperationCode.writeV(dest, wp); //Operation Cod
	if(mHaveParameters)
	{
		mParameters.write(dest, wp);//Parameters
	}
}

void L3NonCallSSComponentInvoke::parseBody( const L3Frame &source, size_t &rp )
{
	if(source.readField(rp, 8) == 0x80)
	{
		rp+=8;//LinkedID tag and LinkedIDLength
		mLinkedID.parseV(source, rp, 1);//LinkedID
		mHaveLinkedID = true;
	}
	rp+=8;//OperationCodTag and OperationCodLength
	mOperationCode.parseV(source, rp);//OperationCode
	if((mComponentLength.value()>9)&&(source.readField(rp, 8) == 0x30))
	{
		rp-=8;
		mParameters.parseV(source, rp);//Parameters
		mHaveParameters = true;
	}
}

size_t L3NonCallSSComponentInvoke::length()
{
	uint64_t ComponentLength = 0x6;
	if (mHaveLinkedID) { ComponentLength += 0x3; }
	if(mHaveParameters) { ComponentLength += mParameters.length(); }
	mComponentLength.setValue(ComponentLength);
	return mComponentLength.value()+mComponentLength.lengthV()+0x1;
}

void L3NonCallSSComponentInvoke::text(std::ostream& os) const
{
	L3NonCallSSComponent::text(os);

	if (mHaveLinkedID) os << " LinkedID = " << mLinkedID;
	os << " OperationCode = " << mOperationCode;
	if (mHaveParameters) os << " Parameters = (" << mParameters<< ")";
}

void L3NonCallSSComponentReturnResult::writeBody( L3Frame &dest, size_t &wp )
{
	this->length();
	mComponentLength.writeV(dest, wp);//ComponentLength
	dest.writeField(wp,0x2,8);//InvokeID tag
	dest.writeField(wp,0x1,8);//InvokeIDLength
	mInvokeID.writeV(dest,wp,8);//InvokeID	
	if(mHaveParameters)
	{
		dest.writeField(wp,0x30,8); //Sequence tag
		mSequenceLength.writeV(dest, wp);//SequenceLength
		dest.writeField(wp,0x2,8);//Operation Cod tag
		dest.writeField(wp,0x1,8);//Operation Cod length
		mOperationCode.writeV(dest, wp); //Operation Cod
		mParameters.write(dest, wp);//Parameters
	}
}

void L3NonCallSSComponentReturnResult::parseBody( const L3Frame &source, size_t &rp )
{
	if ((mComponentLength.value() > 0x3)&&(source.readField(rp, 8) == 0x30))
	{
		mSequenceLength.parseV(source,rp);//SequenceLength
		rp+=16;//OperationCodTag and OperationCodLength
		mOperationCode.parseV(source, rp);//OperationCode
		mParameters.parseV(source, rp);//Parameters
		mHaveParameters = true;
	}
}

size_t L3NonCallSSComponentReturnResult::length()
{
	uint64_t ComponentLength = 0x3;
	if(mHaveParameters)
	{
		
		mSequenceLength.setValue(mParameters.length()+0x3);
		ComponentLength += mSequenceLength.value() + mSequenceLength.lengthV() + 0x1;
	}
	mComponentLength.setValue(ComponentLength);
	return mComponentLength.value()+mComponentLength.lengthV()+0x1;
}

void L3NonCallSSComponentReturnResult::text(std::ostream& os) const
{
	L3NonCallSSComponent::text(os);
	if (mHaveParameters)
	{
		os << " OperationCode = " << mOperationCode;
		os << " Parameters = (" << mParameters << ")";
	}
}

void L3NonCallSSComponentReturnError::writeBody( L3Frame &dest, size_t &wp )
{
	this->length();	
	mComponentLength.writeV(dest, wp);//ComponentLength
	dest.writeField(wp,0x2,8);//InvokeID tag
	dest.writeField(wp,0x1,8);//InvokeIDLength
	mInvokeID.writeV(dest,wp,8);//InvokeID		
	dest.writeField(wp,0x2,8);//Error Cod tag
	dest.writeField(wp,0x1,8);//Error Cod length
	mErrorCode.writeV(dest, wp); //Error Cod
	if(mHaveParameters)
	{
		mParameters.write(dest, wp);//Parameters
	}
}

void L3NonCallSSComponentReturnError::parseBody( const L3Frame &source, size_t &rp )
{
	rp+=16;//ErrorCodTag and ErrorCodLength
	mErrorCode.parseV(source, rp);//ErrorCode
	if((mComponentLength.value() > 0x6)&&(source.readField(rp, 8) == 0x30))
	{
		rp-=8;
		mParameters.parseV(source, rp);//Parameters
		mHaveParameters = true;
	}
}

size_t L3NonCallSSComponentReturnError::length()
{
	uint64_t ComponentLength = 0x6;
	if(mHaveParameters) { ComponentLength += mParameters.length();}
	mComponentLength.setValue(ComponentLength);
	return mComponentLength.value()+mComponentLength.lengthV()+0x1;
}

void L3NonCallSSComponentReturnError::text(std::ostream& os) const
{
	L3NonCallSSComponent::text(os);
	os << " ErrorCode = " << mErrorCode;
	if (mHaveParameters) os << " Parameters = " << mParameters;
}

void L3NonCallSSComponentReject::writeBody( L3Frame &dest, size_t &wp )
{
	this->length();
	mComponentLength.writeV(dest, wp);//ComponentLength	
	dest.writeField(wp,0x6,8);//ComponentLength
	dest.writeField(wp,0x2,8);//InvokeID tag
	dest.writeField(wp,0x1,8);//InvokeIDLength
	mInvokeID.writeV(dest,wp,8);//InvokeID	
	mProblemCodeTag.writeV(dest, wp);//Problem Code Tag
	dest.writeField(wp,0x1,8);//Problem Code length
	mProblemCode.writeV(dest, wp); //Problem Code Cod
}

void L3NonCallSSComponentReject::parseBody( const L3Frame &source, size_t &rp )
{
	mProblemCodeTag.parseV(source, rp);//ProblemCodeTag
	rp+=8;//Problem Code length
	mProblemCode.parseV(source, rp);//ProblemCode
}

size_t L3NonCallSSComponentReject::length()
{
	mComponentLength.setValue(0x6);
	return mComponentLength.value()+mComponentLength.lengthV()+0x1;
}


void L3NonCallSSComponentReject::text(std::ostream& os) const
{
	L3NonCallSSComponent::text(os);
	os << " ProblemCodeTag = " << mProblemCodeTag;
	os << " ProblemCode = " << mProblemCode;
}
