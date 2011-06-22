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

//#include <vector>
#include <list>

#include <config.h>
#include <CLIServer.h>
#include <CLIConnection.h>
#include <CLIParser.h>
#include <Globals.h>
#include <Logger.h>

using namespace std;
using namespace CommandLine;

namespace CommandLine {

typedef std::list<CLIServerConnectionThread *> ServerThreadsList;

/** Free all finished threads from the given list */
void clearFinishedThreads(ServerThreadsList &threadsList)
{
	ServerThreadsList::iterator it;
	for (it = threadsList.begin(); it != threadsList.end(); )
	{
		CLIServerConnectionThread *pThread = *it;
		if (pThread->state() == CLIServerConnectionThread::FINISHING) {
			pThread->join();
			delete pThread;
			// This moves iterator forward, so we don't need to increment it.
			it = threadsList.erase(it);
		} else {
			it++;
		}
	}
}

/** Stop all threads in the list */
void stopAllThreads(ServerThreadsList &threadsList)
{
	while (threadsList.begin() != threadsList.end())
	{
		// Pop an element from the list.
		CLIServerConnectionThread *pThread = threadsList.front();
		threadsList.pop_front();

		switch (pThread->state()) {
		case CLIServerConnectionThread::IDLE:
			delete pThread;
			break;

		case CLIServerConnectionThread::STARTING:
		case CLIServerConnectionThread::STARTED:
			pThread->stop();
			// no break;
		case CLIServerConnectionThread::FINISHING:
			pThread->join();
			delete pThread;
			break;
		}
	}
}

};

void CommandLine::runCLIServer(ConnectionServerSocket *serverSock)
{
	ServerThreadsList serverThreads;

	// Step 1 -- Start listening at the given interface/port
	if (!serverSock->bind()) {
		int errsv = errno;
		LOG(ERROR) << "serverSock->bind() failed with errno="
		           << errsv << " (0x" << hex << errsv << dec << ")";
		return;
	}

	// Step 2 -- Serve incoming connections
	while (1) {
		// Step 2.1 -- Wait for incoming connection
		ConnectionSocket *newConn = serverSock->accept();
		if (newConn == NULL)
		{
			int errsv = errno;
			if (errsv == EINVAL) {
				LOG(INFO) << "serverSock has been closed.";
			} else {
				LOG(INFO) << "serverSock->accept() failed with errno="
				          << errsv << " (0x" << hex << errsv << dec << ")";
			}
			break;
		}
		LOG(INFO) << "New incoming connection: " << *newConn;

		// Step 2.2 -- Add thread to our threads list and start it.
		CLIServerConnectionThread *ipServerThread
			= new CLIServerConnectionThread(newConn, serverSock);
		LOG(DEEPDEBUG) << "New server thread: " << hex << ipServerThread << dec;
		serverThreads.push_back(ipServerThread);
		ipServerThread->start();

		// Step 2.3 -- Clean up finished threads
		clearFinishedThreads(serverThreads);
	}

	// Step 3 -- Cleanup
	serverSock->close();
	stopAllThreads(serverThreads);
}


void *CLIServerConnectionThread::startFunc(void *data)
{
	CLIServerConnectionThread *threadObj = (CLIServerConnectionThread*)data;
	// Simply forward the call to member function
	threadObj->state(STARTED);
	threadObj->run();
	threadObj->state(FINISHING);
	return NULL;
}

void CLIServerConnectionThread::run()
{
	assert(mConnSock != NULL);
	CLIConnection conn(mConnSock);

	while (1) {
		// Receive a command
		std::string recvBlock;
		if (!conn.receiveBlock(recvBlock))
			break;

		// Process command
		ostringstream sendBlock;
		int res = gParser.process(recvBlock, sendBlock);
		if	(res < 0) {
			// Shutdown CLI server if process() returns -1
			mServerSock->close();
			break;
		}

		// Send result back to the client
		if (!conn.sendBlock(sendBlock.str()))
			break;
	}

	mConnSock->close();
}

// vim: ts=4 sw=4
