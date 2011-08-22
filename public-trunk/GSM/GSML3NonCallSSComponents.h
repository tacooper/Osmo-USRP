/**@file Call independent Supplementary Service Control. Facility element components, GSM 04.80 3.6.1.  */
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



#ifndef GSML3NONCALLSSCOMPONENT_H
#define GSML3NONCALLSSCOMPONENT_H

#include "GSML3NonCallSSElements.h"
#include "assert.h"

namespace GSM {

/**
This is virtual base class for the facility element components, GSM 04.80 3.6.1.
*/
class L3NonCallSSComponent {

	protected:

	L3NonCallSSInvokeID mInvokeID;
	L3NonCallSSLengthField mComponentLength;

	public: 

	enum ComponentTypeTag {
		Invoke = 0xa1,
		ReturnResult = 0xa2,
		ReturnError = 0xa3,
		Reject = 0xa4
	};

	L3NonCallSSComponent (const L3NonCallSSInvokeID& wInvokeID = L3NonCallSSInvokeID())
		:mInvokeID(wInvokeID),
		mComponentLength(L3NonCallSSLengthField())
	{}

	/** Return the expected component body length in bytes */
	virtual size_t length() = 0;

	/** Return number of BITS needed to hold component. */
	size_t bitsNeeded() { return 8*length(); }

	/**
	The parse() method reads and decodes L3 NonCallSS facility component bits.
	This method invokes parseBody, assuming that the component header
	has already been read.
	*/
	virtual void parse(const L3Frame& source, size_t &rp);

	/**
	Write component data bits into a BitVector buffer.
	This method invokes writeBody.
	*/
	virtual void write(L3Frame& dest, size_t &wp);

	/** Return the ComponentTypeTag. */
	virtual ComponentTypeTag NonCallSSComponentTypeTag() const =0;

	const L3NonCallSSInvokeID& invokeID() const { return mInvokeID; }

	protected:

	/**
	Write the component body, a method defined in some subclasses.
	*/
	virtual void writeBody(L3Frame& dest, size_t &writePosition) =0;

	/**
	The parseBody() method starts processing at the first byte following the
	invokeID field in the component, which the caller indicates with the
	readPosition argument.
	*/
	virtual void parseBody(const L3Frame& source, size_t &readPosition) =0;

	public:

	/** Generate a human-readable representation of a component. */
	virtual void text(std::ostream& os) const;

};


std::ostream& operator<<(std::ostream& os, L3NonCallSSComponent::ComponentTypeTag CTT);

/**
Parse facility component into its object type.
@param source The L3 bits.
@return A pointer to a new component or NULL on failure.
*/
L3NonCallSSComponent* parseL3NonCallSSComponent(const L3Frame& source, size_t &rp);

/**
A Factory function to return a L3NonCallSSComponent of the specified CTT.
Returns NULL if the CTT is not supported.
*/
L3NonCallSSComponent* L3NonCallSSComponentFactory(L3NonCallSSComponent::ComponentTypeTag CTT);

std::ostream& operator<<(std::ostream& os, const GSM::L3NonCallSSComponent& msg);

/** Invoke component GSM 04.80 3.6.1 Table 3.3 */
class L3NonCallSSComponentInvoke : public L3NonCallSSComponent {

	private:

	//Mandatory
	L3NonCallSSOperationCode mOperationCode;
	//Optionally
	L3NonCallSSLinkedID  mLinkedID;
	L3NonCallSSParameters mParameters;
	bool mHaveLinkedID;
	bool mHaveParameters;

	public:

	L3NonCallSSComponentInvoke(
		const L3NonCallSSInvokeID& wInvokeID = 1,
		const L3NonCallSSOperationCode& wOperationCode = L3NonCallSSOperationCode())
		:L3NonCallSSComponent(wInvokeID),
		mOperationCode(wOperationCode),
		mHaveLinkedID(false),
		mHaveParameters(false)
	{}

	void linkedID (L3NonCallSSLinkedID LinkedID) {mLinkedID = LinkedID; mHaveLinkedID = true;}
	void parameters (L3NonCallSSParameters Parameters) {mParameters = Parameters; mHaveParameters = true;}

	const L3NonCallSSOperationCode& operationCode() const {return mOperationCode;}
	const L3NonCallSSLinkedID&  l3NonCallSSLinkedID() const {assert(mHaveLinkedID); return mLinkedID;}
	const L3NonCallSSParameters& l3NonCallSSParameters() const {assert (mHaveParameters); return mParameters;}

	bool haveL3NonCallSSLinkedID() const {return mHaveLinkedID;}
	bool haveL3NonCallSSParameters() const {return mHaveParameters;}

	ComponentTypeTag NonCallSSComponentTypeTag() const { return Invoke; }
	void writeBody( L3Frame &dest, size_t &wp );
	void parseBody( const L3Frame &src, size_t &rp );
	size_t length();
	void text(std::ostream&) const;

};

/**Return Result component GSM 04.80 3.6.1 Table 3.4 */
class L3NonCallSSComponentReturnResult : public L3NonCallSSComponent {

	private:

	//Optionally
	L3NonCallSSOperationCode mOperationCode;
	L3NonCallSSParameters mParameters;
	L3NonCallSSLengthField mSequenceLength;
	bool mHaveParameters;

	public:

	L3NonCallSSComponentReturnResult(const L3NonCallSSInvokeID& wInvokeID = 1)
		:L3NonCallSSComponent(wInvokeID),
		mSequenceLength(L3NonCallSSLengthField()),
		mHaveParameters(false)
	{}

	void operationCode (L3NonCallSSOperationCode OperationCode) {mOperationCode = OperationCode;}
	void parameters (L3NonCallSSParameters Parameters) {mParameters = Parameters; mHaveParameters = true;}

	const L3NonCallSSOperationCode& l3NonCallSSOperationCode() const {return mOperationCode;}
	const L3NonCallSSParameters& l3NonCallSSParameters() const {assert(mHaveParameters); return mParameters;}

	bool haveL3NonCallSSParameters() const {return mHaveParameters;}

	ComponentTypeTag NonCallSSComponentTypeTag() const { return ReturnResult; }
	void writeBody( L3Frame &dest, size_t &wp );
	void parseBody( const L3Frame &src, size_t &rp );
	size_t length();
	void text(std::ostream&) const;

};

/** Return Error component GSM 04.80 3.6.1 Table 3.5 */
class L3NonCallSSComponentReturnError : public L3NonCallSSComponent {

	private:

	//Mandatory
	L3NonCallSSErrorCode mErrorCode; 
	//Optionally
	L3NonCallSSParameters mParameters;
	bool mHaveParameters;

	public:

	L3NonCallSSComponentReturnError(
		const L3NonCallSSInvokeID& wInvokeID = 1,
		const L3NonCallSSErrorCode& wErrorCode = L3NonCallSSErrorCode())
		:L3NonCallSSComponent(wInvokeID),
		mErrorCode(wErrorCode),
		mHaveParameters(false)
	{}

	void parameters (L3NonCallSSParameters Parameters) {mParameters = Parameters; mHaveParameters = true;}

	const L3NonCallSSErrorCode& errorCode() const {return mErrorCode;}
	const L3NonCallSSParameters& l3NonCallSSParameters() const {assert(mHaveParameters); return mParameters;}
	ComponentTypeTag NonCallSSComponentTypeTag() const { return ReturnError; }

	bool haveL3NonCallSSParameters() const {return mHaveParameters;}

	void writeBody( L3Frame &dest, size_t &wp );
	void parseBody( const L3Frame &src, size_t &rp );
	size_t length();
	void text(std::ostream&) const;

};

/** Reject component GSM 04.80 3.6.1 Table 3.6 */
class L3NonCallSSComponentReject : public L3NonCallSSComponent {

	private:

	//Mandatory
	L3NonCallSSProblemCodeTag mProblemCodeTag;
	L3NonCallSSProblemCode mProblemCode;

	public:

	//Mandatory ProblemCodeTag and ProblemCode
	L3NonCallSSComponentReject(
		const L3NonCallSSInvokeID& wInvokeID = 1,
		const L3NonCallSSProblemCodeTag& wProblemCodeTag = L3NonCallSSProblemCodeTag(),
		const L3NonCallSSProblemCode& wProblemCode = L3NonCallSSProblemCode())
		:L3NonCallSSComponent (wInvokeID),
		mProblemCodeTag(wProblemCodeTag),
		mProblemCode(wProblemCode)
	{}


	const L3NonCallSSProblemCodeTag& problemCodeTag() const {return mProblemCodeTag;}
	const L3NonCallSSProblemCode& problemCode() const {return mProblemCode;}

	ComponentTypeTag NonCallSSComponentTypeTag() const { return Reject; }
	void writeBody( L3Frame &dest, size_t &wp );
	void parseBody( const L3Frame &src, size_t &rp );
	size_t length();
	void text(std::ostream&) const;

};

}
#endif
