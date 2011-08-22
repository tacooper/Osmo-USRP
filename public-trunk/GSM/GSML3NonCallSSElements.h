/**@file Elements for call independent Supplementary Service Control, GSM 04.80 3.  */
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


#ifndef GSML3SSELEMENTS_H
#define GSML3SSELEMENTS_H

#include "GSML3Message.h"
#include <iostream>


namespace GSM {
	
/**
This a virtual class for L3 call independent Supplementary Service Control Elements
with length one octet.
*/
class L3NonCallSSElementOctetLength : public L3ProtocolElement {

	protected:

	uint64_t mValue;
	virtual void setValue() = 0;

	public:

	L3NonCallSSElementOctetLength(uint64_t wValue = 0)
		:L3ProtocolElement(),
		mValue(wValue)
	{}

	size_t lengthV() const { return  1; }
	void writeV( L3Frame& dest, size_t &wp) const;
	void parseV(const L3Frame&, size_t&, size_t) { abort(); }
	void parseV(const L3Frame& src, size_t& rp);

};


/**
This a virtual class for L3 call independent Supplementary Service Control Elements
with undefined length.
*/
class L3NonCallSSElementUndefLength : public L3ProtocolElement {

	protected:

	uint64_t mValue;

	public:

	L3NonCallSSElementUndefLength(uint64_t wValue = 0)
		:L3ProtocolElement(),
		mValue(wValue)
	{}

	size_t lengthV() const;
	void writeV( L3Frame& dest, size_t &wp) const;
	void writeV( L3Frame& dest, size_t &wp, size_t numOctets) const;
	void parseV(const L3Frame&, size_t&, size_t expectedLength); 
	void parseV(const L3Frame& src, size_t& rp){ abort(); }	
	void text(std::ostream& os) const {os << mValue;}
	uint64_t value() const { return mValue; }

};

/**
Classes for L3 call independent Supplementary Service Control Elements, GSM 04.80 3 and GSM 04.80 2.
*/

class L3NonCallSSComponentTypeTag : public L3NonCallSSElementOctetLength {

	public:
	
	/** GSM 04.80, Table 3.7 */
	enum ComponentTypeTag {
		Invoke = 0xa1,
		ReturnResult = 0xa2,
		ReturnError = 0xa3,
		Reject = 0xa4
	};

	private:

	ComponentTypeTag mComponentTypeTag;
	void setValue () {mComponentTypeTag = (ComponentTypeTag)mValue;}

	public:

	L3NonCallSSComponentTypeTag(ComponentTypeTag wComponentTypeTag = Invoke)
		:L3NonCallSSElementOctetLength(wComponentTypeTag),
		mComponentTypeTag(wComponentTypeTag)
	{}
	void text(std::ostream& os) const {os << mComponentTypeTag;}
	ComponentTypeTag NonCallSSComponentTypeTag() const {return mComponentTypeTag;}

};

class L3NonCallSSOperationCode : public L3NonCallSSElementOctetLength {

	public:

	enum OperationCode {
		ProcessUnstructuredSSRequest = 0x3b,
		UnstructuredSSRequest = 0x3c,
		UnstructuredSSNotify = 0x3d
	};

	private:

	OperationCode mOperationCode;
	void setValue () {mOperationCode = (OperationCode)mValue;}

	public:

	L3NonCallSSOperationCode(OperationCode wOperationCode = ProcessUnstructuredSSRequest)
		:L3NonCallSSElementOctetLength(wOperationCode),
		mOperationCode(wOperationCode)
	{}
	
	void text(std::ostream& os) const;
	OperationCode NonCallSSOperationCode() const {return mOperationCode;}

};

class L3NonCallSSErrorCode : public L3NonCallSSElementOctetLength {

	public:

	enum ErrorCode {
		SystemFailure = 0x22,
		DataMissing = 0x23,
		UnexpectedDataValue = 0x24,
		UnknownAlphabet = 0x47, 
		CallBarred = 0xd,
		AbsentSubscriber = 0x1b, 
		IllegalSubscriber = 0x9,
		IllegalEquipment = 0xc,
		UssdBusy = 0x48 
	};

	private:

	ErrorCode mErrorCode;
	void setValue () {mErrorCode = (ErrorCode)mValue;}

	public:

	L3NonCallSSErrorCode(ErrorCode wErrorCode = SystemFailure)
		:L3NonCallSSElementOctetLength(wErrorCode),
		mErrorCode(wErrorCode)
	{}
	
	void text(std::ostream& os) const {os << mErrorCode;}
	ErrorCode NonCallSSErrorCode() const {return mErrorCode;}

};

class L3NonCallSSProblemCodeTag : public L3NonCallSSElementOctetLength {

	public:

	/** GSM 04.80, Table 3.13 */
	enum ProblemCodeTag {
		GeneralProblemTag = 0x80,
		InvokeProblemTag = 0x81,
		ReturnResultProblemTag = 0x82,
		ReturnErrorProblemTag =0x83
	};

	private:

	ProblemCodeTag mProblemCodeTag;
	void setValue () {mProblemCodeTag = (ProblemCodeTag)mValue;}

	public:

	L3NonCallSSProblemCodeTag(ProblemCodeTag wProblemCodeTag = GeneralProblemTag)
		:L3NonCallSSElementOctetLength(wProblemCodeTag),
		mProblemCodeTag(wProblemCodeTag)
	{}

	void text(std::ostream& os) const {os << mProblemCodeTag;}
	ProblemCodeTag NonCallSSProblemCodeTag() const {return mProblemCodeTag;}

};

class L3NonCallSSProblemCode : public L3NonCallSSElementOctetLength {

	public:
	/** GSM 04.80, Table 3.14 - 3.17 */
	enum ProblemCode {
		DuplicateInvokeID = 0x0,
		UnrecognizedOperation = 0x1,
		MistypedParameter = 0x2,
		ResourceLimitation = 0x3,
		InitiatingRelease = 0x4,
		UnrecognizedLinkedID = 0x5,
		LinkedResponseUnexpected = 0x6,
		UnexpectedLinkedOperation = 0x7
	};

	private:

	ProblemCode mProblemCode;
	void setValue () {mProblemCode = (ProblemCode)mValue;}

	public:

	L3NonCallSSProblemCode(ProblemCode wProblemCode = DuplicateInvokeID)
		:L3NonCallSSElementOctetLength(wProblemCode),
		mProblemCode(wProblemCode)
	{}

	void text(std::ostream& os) const {os << mProblemCode;}
	ProblemCode NonCallSSProblemCode() const {return mProblemCode;}

};

class L3NonCallSSVersionIndicator : public L3NonCallSSElementOctetLength {

	public:
	
	enum VersionIndicator {
		Indicator1 = 0x0,
		Indicator2 = 0x1
	};

	private:

	VersionIndicator mVersionIndicator;
	void setValue () {mVersionIndicator = (VersionIndicator)mValue;}

	public:

	L3NonCallSSVersionIndicator(VersionIndicator wVersionIndicator = Indicator1)
		:L3NonCallSSElementOctetLength(wVersionIndicator),
		mVersionIndicator(wVersionIndicator)
	{}

	void text(std::ostream& os) const {os << mVersionIndicator;}
	VersionIndicator NonCallSSVersionIndicator() const {return mVersionIndicator;}

};

class L3NonCallSSInvokeID : public L3NonCallSSElementUndefLength {

	public:
	L3NonCallSSInvokeID (uint64_t wValue = 1)
		:L3NonCallSSElementUndefLength(wValue)
	{}

};

class L3NonCallSSLinkedID : public L3NonCallSSElementUndefLength {

	public:
	L3NonCallSSLinkedID (uint64_t wValue = 0)
		:L3NonCallSSElementUndefLength(wValue)
	{}

};

class L3NonCallSSCause : public L3NonCallSSElementUndefLength {

	public:
	L3NonCallSSCause (uint64_t wValue = 0)
		:L3NonCallSSElementUndefLength(wValue)
	{}

};

class L3NonCallSSLengthField : public L3ProtocolElement {

	protected:

	uint64_t mValue;

	public:

	L3NonCallSSLengthField(uint64_t wValue = 0)
		:L3ProtocolElement(),
		mValue(wValue)
	{}

	size_t lengthV() const;
	void writeV( L3Frame& dest, size_t &wp) const;
	void parseV(const L3Frame& src, size_t& rp);
	void parseV(const L3Frame&, size_t&, size_t) { abort(); }
	void text(std::ostream& os) const {os << mValue;}
	uint64_t value() const { return mValue; }
	void setValue (uint64_t value) {mValue = value;}

};

class L3NonCallSSParameters : public L3ProtocolElement {

	private:

	unsigned mDataCodingScheme;		///< data coding scheme
	char mData[184];	///< actual data, as a C string
	L3NonCallSSLengthField mSequenceLength;
	L3NonCallSSLengthField mDataLength;
	unsigned mAlertingPattern;
	bool mHaveAlertingPattern;

	public:

	L3NonCallSSParameters(unsigned wDataCodingScheme=0xF)
		:L3ProtocolElement(),
		mDataCodingScheme(wDataCodingScheme),
		mSequenceLength(L3NonCallSSLengthField()),
		mDataLength(L3NonCallSSLengthField()),
		mAlertingPattern(0),
		mHaveAlertingPattern(false)
	{ mData[0]='\0'; }

	/** Initize from a simple C string. */
	L3NonCallSSParameters(const char* text)
		:L3ProtocolElement(),
		mDataCodingScheme(0xF),
		mSequenceLength(L3NonCallSSLengthField()),
		mDataLength(L3NonCallSSLengthField()),
		mAlertingPattern(0),
		mHaveAlertingPattern(false)		
	{ strncpy(mData,text,182); 
	  mData[sizeof(mData) - 1] = '\0';}

	void DataCodingScheme(unsigned wDataCodingScheme) { mDataCodingScheme=wDataCodingScheme; }
	unsigned DataCodingScheme() { return mDataCodingScheme; }
	void alertingPattern (unsigned AlertingPattern) {mAlertingPattern = AlertingPattern; mHaveAlertingPattern = true;}
	const char* data() const { return mData; }
	const unsigned& alertingPattern() const {assert(mHaveAlertingPattern); return mAlertingPattern;}
	bool haveAlertingPattern() const {return mHaveAlertingPattern;}
	size_t lengthV() const { abort(); }
	size_t length();
	void parseV(const L3Frame&, size_t&, size_t) { abort(); }
	void parseV(const L3Frame&, size_t&);

	void writeV(L3Frame&, size_t&) const { abort(); }
	void write(L3Frame&, size_t&); 
	void text(std::ostream&) const;
};
}
#endif
