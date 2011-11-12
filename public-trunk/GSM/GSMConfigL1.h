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



#ifndef GSMCONFIGL1_H
#define GSMCONFIGL1_H

#include <vector>

#include <PowerManager.h>
#include <TRXManager.h>

#include "GSMCommon.h"


namespace GSM {


/**
	This object carries the top-level GSM air interface configuration.
	It serves as a central clearinghouse to get access to everything else in the GSM code.
*/
class GSMConfigL1 {

	protected:

	/**@name BSIC. */
	//@{
	unsigned mNCC;		///< network color code
	unsigned mBCC;		///< basestation color code
	//@}

	GSMBand mBand;		///< BTS operating band

	Clock mClock;		///< local copy of BTS master clock

	time_t mStartTime;

	public:

	/** All parameters come from gConfig. */
	GSMConfigL1();

	/** Get the current master clock value. */
	Time time() const { return mClock.get(); }

	/**@name Accessors. */
	//@{
	GSMBand band() const { return mBand; }
	unsigned BCC() const { return mBCC; }
	unsigned NCC() const { return mNCC; }
	GSM::Clock& clock() { return mClock; }
	//@}

	/** Return the BSIC, NCC:BCC. */
	unsigned BSIC() const { return (mNCC<<3) | mBCC; }

	/** Return number of seconds since starting. */
	time_t uptime() const { return ::time(NULL)-mStartTime; }

};



};	// GSM


#endif


// vim: ts=4 sw=4
