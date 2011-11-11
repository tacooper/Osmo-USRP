/*
* Copyright 2011 Harald Welte <laforge@gnumonks.org>
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



#include "OsmoSAPMux.h"
#include "GSMTransfer.h"
#include "GSML1FEC.h"

#include <Logger.h>


using namespace GSM;


void OsmoSAPMux::writeHighSide(const L2Frame& frame)
{
	// The SAP may or may not be present, depending on the channel type.
	OBJLOG(DEEPDEBUG) << "OsmoSAPMux::writeHighSide " << frame;
}



void OsmoSAPMux::writeLowSide(const L2Frame& frame)
{
	OBJLOG(DEEPDEBUG) << "OsmoSAPMux::writeLowSide SAP" << frame.SAPI() << " " << frame;
}

// vim: ts=4 sw=4
