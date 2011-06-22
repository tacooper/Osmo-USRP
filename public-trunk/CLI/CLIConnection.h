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


#ifndef OPENBTSCLICONNECTION_H
#define OPENBTSCLICONNECTION_H

#include <string>
#include <Sockets.h>


namespace CommandLine {

class CLIConnection {
public:

	/** Constructor */
	CLIConnection(ConnectionSocket *pSocket)
	: mSocket(pSocket)
	{
		assert(mSocket);
	}

	/** Return a reference to the actual socket. */
	ConnectionSocket *socket() { return mSocket; }

	/**
	  Send a block of data
	  @return true if sent successfully, false otherwise.
	*/
	bool sendBlock(const std::string &block);

	/**
	  Receive a block of data
	  @return true if received successfully, false otherwise.
	*/
	bool receiveBlock(std::string &block);

protected:

	ConnectionSocket *mSocket; ///< Actual socket for the connection

	/** Logs the error in a readable form and closes socket. */
	void onError(int errnum, const char *oper);

};

} // CLI



#endif
// vim: ts=4 sw=4
