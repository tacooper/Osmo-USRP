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




#include "Sockets.h"
#include "Threads.h"
#include "Logger.h"
#include "Configuration.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <vector>

using namespace std;


ConfigurationTable gConfig;

static const int gNumToSend = 3;
static const int gIpBindPort = 22239;
static const char *gIpBindHost = "127.0.0.1";
//static const char *gIpBindHost = "0.0.0.0";
static const char *gUnixSocketPath = "/tmp/testUnixSock";

void reportError(const std::string &what, int errsv)
{
	LOG(ERROR) << what << " failed with errno="
	           << errsv << " (0x" << hex << errsv << dec << "): "
	           << strerror(errsv);
}

void *ipServerClose(void *data)
{
	ConnectionServerSocket *pServer = (ConnectionServerSocket*)data;

	sleep(1);
	pServer->close();

	return NULL;
}

bool clientConnect(ConnectionSocket *conn)
{
	int res = conn->connect();
	if (res < 0) {
		reportError("conn->connect()", errno);
		conn->close();
		return false;
	}

	return true;
}

bool clientWrite(ConnectionSocket *conn, const char *text)
{
	std::string sendBuf = text;
	LOG(INFO) << "Client sending: \"" << sendBuf << "\"";
	uint32_t sendBufLength = htonl(sendBuf.length());
	int bytesSent;
	bytesSent = conn->write((const char *)&sendBufLength, 4);
	if (bytesSent < 4) {
		reportError("conn->write()", errno);
		conn->close();
		return false;
	}
	bytesSent = conn->write(sendBuf.data(), sendBuf.length());
	if (bytesSent < sendBuf.length()) {
		reportError("conn->write()", errno);
		conn->close();
		return false;
	}

	return true;
}

bool clientRead(ConnectionSocket *conn)
{
	uint32_t recvDataLen;
	char readBuf[1024];
	int bytesReceived;
	bytesReceived = conn->read(readBuf, 4);
	if (bytesReceived < 4) {
		reportError("conn->read()", errno);
		conn->close();
		return false;
	}
	recvDataLen = ntohl(*(uint32_t*)readBuf);
	if (recvDataLen > 1024) {
		recvDataLen = 1024;
	}

	bytesReceived = conn->read(readBuf, recvDataLen);
	if (bytesReceived < recvDataLen) {
		reportError("conn->read()", errno);
		conn->close();
		return false;
	}
	readBuf[bytesReceived>=sizeof(readBuf)?sizeof(readBuf)-1:bytesReceived]= '\0';
	LOG(INFO) << "Client received: \"" << readBuf << "\"";

	return true;
}

void *ipClient(void *)
{
	for (int i=0; i<gNumToSend; i++)
	{
		ConnectionSocketTCP conn(gIpBindPort, gIpBindHost);

		if (!clientConnect(&conn)) continue;
		if (!clientWrite(&conn, "ping")) continue;
		if (!clientRead(&conn)) continue;
		conn.close();

		usleep(500*1000);
	}

	// Shutdown server

	ConnectionSocketTCP conn(gIpBindPort, gIpBindHost);
	if (!clientConnect(&conn)) return NULL;
	if (!clientWrite(&conn, "exit")) return NULL;
	conn.close();

	return NULL;
}

void *unixClient(void *)
{
	for (int i=0; i<gNumToSend; i++)
	{
		ConnectionSocketUnix conn(gUnixSocketPath);

		if (!clientConnect(&conn)) continue;
		if (!clientWrite(&conn, "ping")) continue;
		if (!clientRead(&conn)) continue;
		conn.close();

		usleep(500*1000);
	}

	// Shutdown server

	ConnectionSocketUnix conn(gUnixSocketPath);
	if (!clientConnect(&conn)) return NULL;
	if (!clientWrite(&conn, "exit")) return NULL;
	conn.close();

	return NULL;
}

struct ConnectionPair {
	ConnectionPair(ConnectionServerSocket *serv, ConnectionSocket *conn)
	: serverSocket(serv)
	, connectionSocket(conn)
	{};

	ConnectionServerSocket *serverSocket;
	ConnectionSocket *connectionSocket;
};


void *serverConnThread(void *data)
{
	ConnectionPair *pConnPair = (ConnectionPair*)data;
	ConnectionSocket *pConn = pConnPair->connectionSocket;
	ConnectionServerSocket *pServerSock = pConnPair->serverSocket;
	delete pConnPair;

	uint32_t recvDataLen;
	char readBuf[1024];
	int bytesReceived;
	bytesReceived = pConn->read(readBuf, 4);
	if (bytesReceived < 4) {
		reportError("pConn->read()", errno);
		pConn->close();
		return NULL;
	}
	recvDataLen = ntohl(*(uint32_t*)readBuf);
	if (recvDataLen > 1024) {
		recvDataLen = 1024;
	}

	bytesReceived = pConn->read(readBuf, recvDataLen);
	if (bytesReceived < recvDataLen) {
		reportError("pConn->read()", errno);
		pConn->close();
		return NULL;
	}
	readBuf[bytesReceived>=sizeof(readBuf)?sizeof(readBuf)-1:bytesReceived]= '\0';
	LOG(INFO) << "Server received: \"" << readBuf << "\"";

	// Shutdown server is requested so.
	if (strcmp(readBuf, "exit")==0) {
		pConn->close();
		pServerSock->close();
		return NULL;
	}

	std::string sendBuf = "pong";
	LOG(INFO) << "Server sending: \"" << sendBuf << "\"";
	uint32_t sendBufLength = htonl(sendBuf.length());
	int bytesSent;
	bytesSent = pConn->write((const char *)&sendBufLength, 4);
	if (bytesSent < 4) {
		reportError("pConn->write()", errno);
		pConn->close();
		return NULL;
	}
	bytesSent = pConn->write(sendBuf.data(), sendBuf.length());
	if (bytesSent < sendBuf.length()) {
		reportError("pConn->write()", errno);
		pConn->close();
		return NULL;
	}

	// Wait for remote side to close connection to avoid TIME_WAIT on server side.
	// Refer to http://hea-www.harvard.edu/~fine/Tech/addrinuse.html
	bytesReceived = pConn->read(readBuf, 1);
	pConn->close();

	// This should be done in thread's destructor
	delete pConn;

	return NULL;
}

void serverBind(ConnectionServerSocket *serverSock)
{
	if (!serverSock->bind()) {
		reportError("serverSock->bind()", errno);
		return;
	}
}

void serverRun(ConnectionServerSocket *serverSock)
{
	std::vector<Thread *> serverThreads;

	while (1) {
		ConnectionSocket *newConn = serverSock->accept();
		if (newConn == NULL)
		{
			int errsv = errno;
			if (errsv == EINVAL) {
				LOG(INFO) << "serverSock has been closed.";
			} else {
				reportError("serverSock->accept()", errsv);
			}
			break;
		}
		LOG(INFO) << "New incoming connection " << newConn;

		Thread *ipServerThread = new Thread();
		LOG(INFO) << "New server thread: " << hex << ipServerThread << dec;
		serverThreads.push_back(ipServerThread);
		ipServerThread->start(serverConnThread, new ConnectionPair(serverSock, newConn));
	}

	// Stop server
	serverSock->close();

	// Clean up
	Thread *ipServerThread;
	while (serverThreads.size()) {
		ipServerThread = serverThreads.back();
		LOG(INFO) << "Removing server thread: " << hex << ipServerThread << dec;
		ipServerThread->join();
		delete ipServerThread;
		serverThreads.pop_back();
	}
}

int main(int argc, char * argv[] )
{
	gLogInit("DEBUG");

	LOG(INFO) << "Testing TCP sockets";
	{
		ConnectionServerSocketTCP serverSock(gIpBindPort, gIpBindHost);
		Thread readerThreadIP;

		serverBind(&serverSock);
		readerThreadIP.start(ipClient, NULL);
		serverRun(&serverSock);
		readerThreadIP.join();
	}

	LOG(INFO) << "Testing UNIX sockets";
	{
		ConnectionServerSocketUnix serverSock(gUnixSocketPath);
		Thread readerThreadIP;

		serverBind(&serverSock);
		readerThreadIP.start(unixClient, NULL);
		serverRun(&serverSock);
		readerThreadIP.join();
	}
}

// vim: ts=4 sw=4
