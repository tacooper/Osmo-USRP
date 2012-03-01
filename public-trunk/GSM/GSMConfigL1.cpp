/*
* Copyright 2008, 2009, 2010 Free Software Foundation, Inc.
* Copyright 2012 Thomas Cooper <tacooper@vt.edu>
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



#include <Globals.h>
#include <Logger.h>

#include "GSMConfigL1.h"

using namespace std;
using namespace GSM;

GSMConfigL1::GSMConfigL1()
	:mBand((GSMBand)gConfig.getNum("GSM.Band")),
	mStartTime(::time(NULL))
{
	// BSIC components
	mNCC = gConfig.getNum("GSM.NCC");
	LOG_ASSERT(mNCC<8);
	mBCC = gConfig.getNum("GSM.BCC");
	LOG_ASSERT(mBCC<8);
}
