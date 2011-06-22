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


#ifndef OPENBTSCLISERVER_H
#define OPENBTSCLISERVER_H

#include <string>
#include <map>
#include <iostream>
#include <Threads.h>
#include "CLIConnection.h"

class ConnectionServerSocket;

namespace CommandLine {

void runCLIServer(ConnectionServerSocket *serverSock);

class CLIServerConnectionThread {
public:

	enum State {
		IDLE,      ///< Thread is not started
		STARTING,  ///< Thread start has been just requested
		STARTED,   ///< Thread has entered its main processing loop
		FINISHING  ///< Thread has exited from its main processing loop
	};

	/** Constructor */
	CLIServerConnectionThread(ConnectionSocket *connSock,
	                          ConnectionServerSocket *serverSock)
	: mConnSock(connSock)
	, mServerSock(serverSock)
	, mState(IDLE)
	{}

	/** Destructor */
	~CLIServerConnectionThread()
	{
		assert(state() == IDLE);
		delete mConnSock;
	}

	/** Request thread to start. */
	void start() { assert(state() == IDLE); state(STARTING); mThread.start(startFunc, this); }

	/** Request thread to stop. */
	void stop() { mConnSock->close(); }

	/** Wait for the thread to finish. */
	void join() { mThread.join(); state(IDLE); }

	/** Get internal state of the thread */
	State state() const
	{
		mStateMutex.lock();
		State retval = mState;
		mStateMutex.unlock();
		return retval;
	}

protected:

	Thread mThread;   ///< Actual thread instance. We're proxying requests to it.
	ConnectionSocket * const mConnSock; ///< Connection to a client socket.
	                  ///< It's thread-safe to access it as long as it's const.
	ConnectionServerSocket * const mServerSock; ///< Listening server socket.
	                  ///< It's thread-safe to access it as long as it's const.
	mutable Mutex mStateMutex; ///< A mutex to protect mState variable.
	State mState;     ///< State of the thread - is it running or what.


	/** Static function to pass to Thread::start() */
	static void *startFunc(void *data);

	/** Main entry for the thread */
	void run();

	/** Change internal state of the thread. */
	void state(State newState)
	{
		mStateMutex.lock();
		mState = newState;
		mStateMutex.unlock();
	}

};

} // CLI



#endif
// vim: ts=4 sw=4
