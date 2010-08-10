/**@file Elements for call independent Supplementary Service Control, GSM 04.80 3.  */
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




#include "GSML3NonCallSSElements.h"
#include <Logger.h>

using namespace std;
using namespace GSM;


void L3NonCallSSElementOctetLength::writeV( L3Frame& dest, size_t &wp ) const
{
	dest.writeField(wp,mValue,8);
}

void L3NonCallSSElementOctetLength::parseV( const L3Frame& src, size_t &rp)
{
	mValue = src.readField(rp, 8);
	setValue();
}


size_t L3NonCallSSElementUndefLength::lengthV() const 
{
	size_t size = sizeof(mValue);
	uint64_t tmp = 0;
	do 
	{
		size--;
		tmp = mValue>>(size*8);

	} while ((tmp == 0) &&(size>0));
	size++;
	return size; 
}

void L3NonCallSSElementUndefLength::writeV( L3Frame& dest, size_t &wp ) const
{
		dest.writeField(wp,mValue,lengthV()*8);
}

void L3NonCallSSElementUndefLength::writeV( L3Frame& dest, size_t &wp, size_t numBits) const
{
		dest.writeField(wp,mValue,numBits);
}

void L3NonCallSSElementUndefLength::parseV( const L3Frame& src, size_t &rp,  size_t numOctets)
{
	 mValue = src.readField(rp, numOctets*8);
}


void L3NonCallSSOperationCode::text(ostream& os) const
{
	switch (mOperationCode){
		case ProcessUnstructuredSSRequest:
			os << "ProcessUnstructuredSSRequest"; break;
		case UnstructuredSSRequest:
			os << "UnstructuredSSRequest"; break;
		case UnstructuredSSNotify:
			os << "UnstructuredSSNotify"; break;
	}
}

size_t L3NonCallSSLengthField::lengthV() const 
{
	size_t length = 0;	
	if (mValue <= 0x7f) length = 1; 
	else
	{
		uint64_t tmp = mValue;
		do
		{
			length++;
			tmp>>=8;
		} while(tmp!=0);
		length++;
	}
	return length;
}

void L3NonCallSSLengthField::writeV( L3Frame& dest, size_t &wp ) const
{
	if (mValue <= 0x7f) { dest.writeField(wp,mValue,8); }
	else
	{
		uint64_t tmp = mValue;
		size_t numOctets = 0;
		do
		{
			numOctets++;
			tmp>>=8;
		} while(tmp!=0);
		dest.writeField(wp,numOctets|0x80,8);
		dest.writeField(wp,mValue,numOctets*8);
	}
}

void L3NonCallSSLengthField::parseV( const L3Frame& source, size_t &rp)
{
	uint64_t tmp = source.readField(rp, 8);
	if ((tmp>>7)==0) mValue = tmp;
	else mValue = source.readField(rp, (tmp&0x7f)*8);
}

size_t L3NonCallSSParameters::length()
{
	unsigned numChar = strlen(mData);
	unsigned ussdStringLength = (numChar*7)/8;
	if ((numChar*7)%8!=0) ussdStringLength +=1;
	mDataLength.setValue(ussdStringLength);
	unsigned sequenceLength = mDataLength.value()+mDataLength.lengthV()+4; 
	if (mHaveAlertingPattern) sequenceLength+=0x3;
	mSequenceLength.setValue(sequenceLength);
	return mSequenceLength.value()+mSequenceLength.lengthV()+1;
}



void L3NonCallSSParameters::parseV(const L3Frame& src, size_t& rp)
{
	rp+=8;
	mSequenceLength.parseV(src, rp);
	rp+=16;
	mDataCodingScheme = src.readField(rp,8);
	rp+=8;
	mDataLength.parseV(src, rp);
	unsigned numChar = mDataLength.value()*8/7;
	BitVector chars(src.tail(rp));
	chars.LSB8MSB();
	size_t crp=0;
	for (int i=0; i<numChar; i++) 
	{
		char gsm = chars.readFieldReversed(crp,7);
		mData[i] = decodeGSMChar(gsm);
	}
	mData[numChar]='\0';
	if (crp%8) crp += 8 - crp%8;
	rp += crp;
	if ((mSequenceLength.value()-0x4-mDataLength.lengthV()-mDataLength.value())!=0)
		if(src.readField(rp,8)==0x4)
		{
			rp+=8;
			mAlertingPattern = src.readField(rp,8);
			mHaveAlertingPattern = true;
		}
}


void L3NonCallSSParameters::write(L3Frame& dest, size_t& wp)
{
	dest.writeField(wp,0x30,8); //Sequence Tag GSM 04.80 3.6.5.
	unsigned numChar = strlen(mData);
	unsigned ussdStringLength = (numChar*7)/8;
	if ((numChar*7)%8!=0) ussdStringLength +=1;
	mDataLength.setValue(ussdStringLength);
	unsigned sequenceLength = mDataLength.value()+mDataLength.lengthV()+4; 
	if (mHaveAlertingPattern) sequenceLength+=0x3;
	mSequenceLength.setValue(sequenceLength);
	mSequenceLength.writeV(dest, wp); //Sequence Length
	dest.writeField(wp,0x4,8); // ANS1 Octet String Tag	
	dest.writeField(wp,0x1,8); // String Length
	dest.writeField(wp,mDataCodingScheme,8); // DataCodingScheme
	dest.writeField(wp,0x4,8); // ANS1 Octet String Tag
	mDataLength.writeV(dest, wp); // String Length
	//USSD String	
	BitVector chars = dest.tail(wp);
	chars.zero();
	for (int i=0; i<numChar; i++) {
		char gsm = encodeGSMChar(mData[i]);
		dest.writeFieldReversed(wp,gsm,7);
	}
	chars.LSB8MSB();
	if (mHaveAlertingPattern) 
	{
		dest.writeField(wp,0x4,8); // ANS1 Octet String Tag	
		dest.writeField(wp,0x1,8); // String Length
		dest.writeField(wp,mAlertingPattern,8); // Alerting Pattern
	}
}



void L3NonCallSSParameters::text(ostream& os) const
{
	os << " DataCodingScheme = " << hex << "0x" << mDataCodingScheme << dec;
	os << " Data = " << mData;
	if (mHaveAlertingPattern) os << " AlertingPattern = " << mAlertingPattern;
}
