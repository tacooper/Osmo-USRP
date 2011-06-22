/*
* Copyright 2010 Free Software Foundation, Inc.
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



#include <config.h>
#include <Globals.h>
#include <Sockets.h>
#include <Threads.h>
#include <Logger.h>
#include <CLIClient.h>

using namespace std;
using namespace CommandLine;

// Load configuration from a file.
ConfigurationTable gConfig("OpenBTS.config");


int main(int argc, char * argv[])
{
	srandom(time(NULL));
	COUT(endl << endl << gOpenBTSWelcome << endl);

	gLogInit("INFO");

//	if (gConfig.defines("Log.FileName")) {
//		gSetLogFile(gConfig.getStr("Log.FileName"));
//	}

	if (strcasecmp(gConfig.getStr("CLI.Type"),"TCP") == 0) {
		ConnectionSocketTCP sock(gConfig.getNum("CLI.TCP.Port"),
		                         gConfig.getStr("CLI.TCP.IP"));
		runCLIClient(&sock);
	} else if (strcasecmp(gConfig.getStr("CLI.Type"),"Unix") == 0) {
		ConnectionSocketUnix sock(gConfig.getStr("CLI.Unix.Path"));
		runCLIClient(&sock);
	} else {
		COUT(endl << "ERROR -- OpenBTS not configured to use remote CLI" << endl);
	}
}

// vim: ts=4 sw=4
