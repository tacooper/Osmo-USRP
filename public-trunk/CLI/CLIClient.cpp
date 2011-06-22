/*
* Copyright 2009, 2010 Free Software Foundation, Inc.
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

#include <string>

#include <config.h>
#include "CLIClient.h"
#include <Globals.h>
#include <Logger.h>
#include <CLI.h>

using namespace std;
using namespace CommandLine;

int CLIClientProcessor::process(const std::string &line, std::ostream& os)
{
	// Step 1 -- Send command
	if (!mConnection.sendBlock(line))
		return -1;

	// Step 2 -- Receive response
	std::string recvBlock;
	if (!mConnection.receiveBlock(recvBlock))
		return -1;

	// Step 3 -- Print out response
	os << recvBlock;
	os << endl;

	return 0;
}

void CommandLine::runCLIClient(ConnectionSocket *sock)
{
	// Connect to a server
	int res = sock->connect();
	if (res < 0) {
		int errsv = errno;
		LOG(WARN) << "sock.connect() failed with errno="
		          << errsv << " (0x" << hex << errsv << dec << "): "
		          << strerror(errsv);
		sock->close();
		return;
	}

	// Start the main loop
	CLIClientProcessor proc(sock);
	runCLI(&proc);
}

// vim: ts=4 sw=4
