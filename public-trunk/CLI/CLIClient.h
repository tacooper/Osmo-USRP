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


#ifndef OPENBTSCLICLIENT_H
#define OPENBTSCLICLIENT_H

#include <string>
#include <iostream>
#include <CLIConnection.h>
#include <CLIParserBase.h>

namespace CommandLine {

class CLIClientProcessor : public ParserBase {
public:

	CLIClientProcessor(ConnectionSocket *pSock)
	: mConnection(pSock)
	{};

	~CLIClientProcessor() {};

	/**
		Process a command line.
		@return 0 on success, -1 on exit request, error codes otherwise
	*/
	virtual int process(const std::string &line, std::ostream& os);

protected:

	CLIConnection mConnection; ///< Network connection to a server

};

// Run CLI client
void runCLIClient(ConnectionSocket *sock);

} // CLI



#endif
// vim: ts=4 sw=4
