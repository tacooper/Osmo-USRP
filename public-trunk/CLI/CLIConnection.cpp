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

#include <config.h>
#include "CLIConnection.h"
#include <Logger.h>
//#include <Globals.h>

using namespace std;
using namespace CommandLine;

bool CLIConnection::sendBlock(const std::string &block)
{
	// Step 1 -- Prepare block
	std::string::size_type blockLen = block.length();
	uint32_t sendBlockLen = htonl(blockLen);
	int bytesSent;
	LOG(DEEPDEBUG) << "Sending: (" << blockLen << "/0x" << hex << blockLen << dec
	               << ") \"" << block << "\"";

	// Step 2 -- Send block length
	bytesSent = mSocket->write((const char *)&sendBlockLen, 4);
	if (bytesSent < 4) {
		onError(errno, "mSocket.write()");
		return false;
	}

	// Step 3 -- Send block data
	bytesSent = mSocket->write(block.data(), blockLen);
	if (bytesSent < blockLen) {
		onError(errno, "mSocket.write()");
		return false;
	}

	return true;
}

bool CLIConnection::receiveBlock(std::string &block)
{
	uint32_t recvDataLen;
	int bytesReceived;

	// Step 1 -- Read length of the block
	bytesReceived = mSocket->read((char*)&recvDataLen, 4);
	if (bytesReceived < 4) {
		onError(errno, "mSocket.read()");
		return false;
	}
	recvDataLen = ntohl(recvDataLen);

	// Step 2 -- prepare buffer
	block.resize(recvDataLen);

	// Step 3 -- Receive actual data
	bytesReceived = mSocket->read((char*)block.data(), recvDataLen);
	if (bytesReceived < recvDataLen) {
		onError(errno, "mSocket.read()");
		return false;
	}
//	readBuf[bytesReceived>=sizeof(readBuf)?sizeof(readBuf)-1:bytesReceived]= '\0';
	LOG(DEEPDEBUG) << "Received: (" << recvDataLen << "/0x" << hex << recvDataLen << dec
	               << ") \"" << block << "\"";

	return true;
}

void CLIConnection::onError(int errnum, const char *oper)
{
	if (errnum == 0) {
		LOG(INFO) << "Connection has been closed by remote party for socket " << *mSocket;
	} else {
		LOG(WARN) << "Socket=" << *mSocket << " operation " << oper
		          << " failed with errno=" << errnum << " (0x"
		          << hex << errnum << dec << "): "
		          << strerror(errnum);
	}
	mSocket->close();
}

// vim: ts=4 sw=4
