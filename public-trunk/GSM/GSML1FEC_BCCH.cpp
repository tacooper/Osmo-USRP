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


#define NDEBUG


#include "GSML1FEC.h"
#include "GSMCommon.h"
#include "GSMSAPMux.h"
#include "GSMConfig.h"
#include "GSMTDMA.h"
#include "GSMTAPDump.h"
#include <TRXManager.h>
#include <Logger.h>
#include <assert.h>
#include <math.h>

using namespace std;
using namespace GSM;





void BCCHL1Encoder::generate()
{
	OBJLOG(DEEPDEBUG) << "BCCHL1Encoder " << mNextWriteTime;
	// BCCH mapping, GSM 05.02 6.3.1.3
	// Since we're not doing GPRS or VGCS, it's just SI1-4 over and over.
	switch (mNextWriteTime.TC()) {
		case 0: writeHighSide(gBTS.SI1Frame()); return;
		case 1: writeHighSide(gBTS.SI2Frame()); return;
		case 2: writeHighSide(gBTS.SI3Frame()); return;
		case 3: writeHighSide(gBTS.SI4Frame()); return;
		case 4: writeHighSide(gBTS.SI3Frame()); return;
		case 5: writeHighSide(gBTS.SI2Frame()); return;
		case 6: writeHighSide(gBTS.SI3Frame()); return;
		case 7: writeHighSide(gBTS.SI4Frame()); return;
		default: assert(0);
	}
}
