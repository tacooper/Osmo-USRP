/*
* Copyright 2008 Free Software Foundation, Inc.
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




#include "Threads.h"
#include "Timeval.h"

#include <errno.h>

using namespace std;




Mutex gStreamLock;		///< Global lock to control access to cout and cerr.

void lockCout()
{
	gStreamLock.lock();
	Timeval entryTime;
	cout << entryTime << " " << pthread_self() << ": ";
}


void unlockCout()
{
	cout << dec << endl << flush;
	gStreamLock.unlock();
}


void lockCerr()
{
	gStreamLock.lock();
	Timeval entryTime;
	cerr << entryTime << " " << pthread_self() << ": ";
}

void unlockCerr()
{
	cerr << dec << endl << flush;
	gStreamLock.unlock();
}







Mutex::Mutex()
{
	pthread_mutexattr_init(&mAttribs);
	int s = pthread_mutexattr_settype(&mAttribs,PTHREAD_MUTEX_RECURSIVE);
	assert(s==0);
	pthread_mutex_init(&mMutex,&mAttribs);
}


Mutex::~Mutex()
{
	pthread_mutex_destroy(&mMutex);
	pthread_mutexattr_destroy(&mAttribs);
}




/** Block for the signal up to the cancellation timeout. */
void Signal::wait(Mutex& wMutex, unsigned timeout) const
{
	Timeval then(timeout);
	struct timespec waitTime = then.timespec();
	pthread_cond_timedwait(&mSignal,&wMutex.mMutex,&waitTime);
}

ThreadSemaphore::Result ThreadSemaphore::wait(unsigned timeoutMs)
{
	Timeval then(timeoutMs);
	struct timespec waitTime = then.timespec();
	int s;
	while ((s = sem_timedwait(&mSem,&waitTime)) == -1 && errno == EINTR)
		continue;

	if (s == -1)
	{
		if (errno == ETIMEDOUT)
		{
			return TSEM_TIMEOUT;
		}
		return TSEM_ERROR;
	}
	return TSEM_OK;
}

ThreadSemaphore::Result ThreadSemaphore::wait()
{
	int s;
	while ((s = sem_wait(&mSem)) == -1 && errno == EINTR)
		continue;

	if (s == -1)
	{
		return TSEM_ERROR;
	}
	return TSEM_OK;
}

ThreadSemaphore::Result ThreadSemaphore::trywait() 
{
	int s;
	while ((s = sem_trywait(&mSem)) == -1 && errno == EINTR)
		continue;

	if (s == -1)
	{
		if (errno == EAGAIN)
		{
			return TSEM_TIMEOUT;
		}
		return TSEM_ERROR;
	}
	return TSEM_OK;
}

ThreadSemaphore::Result ThreadSemaphore::post()
{
	if (sem_post(&mSem) != 0)
	{
		if (errno == EOVERFLOW)
		{
			return TSEM_OVERFLOW;
		}
		return TSEM_ERROR;
	}
	return TSEM_OK;
}

void Thread::start(void *(*task)(void*), void *arg)
{
	int s;
	assert(mThread==((pthread_t)0));

	s = pthread_attr_init(&mAttrib);
	assert(s == 0);
	s = pthread_attr_setstacksize(&mAttrib, mStackSize);
	assert(s == 0);
	s = pthread_create(&mThread, &mAttrib, task, arg);
	assert(s == 0);
}



// vim: ts=4 sw=4
