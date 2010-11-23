/*
* Copyright 2008, 2010 Free Software Foundation, Inc.
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
#include <unistd.h>
#include <fcntl.h>
#include <cstdio>
#include <sys/select.h>

#include "Threads.h"
#include "Sockets.h"
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>






bool resolveAddress(struct sockaddr_in *address, const char *host, unsigned short port)
{
	// FIXME -- Need to ignore leading/trailing spaces in hostname.
	struct hostent *hp;
#ifdef HAVE_GETHOSTBYNAME2_R
	struct hostent hostData;
	char tmpBuffer[2048];
	int h_errno;

	// There are different flavors of gethostbyname_r(), but
	// latest Linux use the following form:
	if (gethostbyname2_r(host, AF_INET, &hostData, tmpBuffer, sizeof(tmpBuffer), &hp, &h_errno)!=0) {
		CERR("WARNING -- gethostbyname() failed for " << host << ", " << hstrerror(h_errno));
		return false;
	}
#else
	static Mutex sGethostbynameMutex;
	// gethostbyname() is NOT thread-safe, so we should use a mutex here.
	// Ideally it should be a global mutex for all non thread-safe socket
	// operations and it should protect access to variables such as
	// global h_errno.
	sGethostbynameMutex.lock();
	hp = gethostbyname(host);
	sGethostbynameMutex.unlock();
#endif
	if (hp==NULL) {
		CERR("WARNING -- gethostbyname() failed for " << host << ", " << hstrerror(h_errno));
		return false;
	}
	if (hp->h_addrtype != AF_INET) {
		CERR("WARNING -- gethostbyname() resolved " << host << " to something other then AF)INET");
		return false;
	}
	address->sin_family = hp->h_addrtype;
	assert(sizeof(address->sin_addr) == hp->h_length);
	memcpy(&(address->sin_addr), hp->h_addr_list[0], hp->h_length);
	address->sin_port = htons(port);
	return true;
}



DatagramSocket::DatagramSocket()
{
	memset(mDestination, 0, sizeof(mDestination));
}





void DatagramSocket::nonblocking()
{
	fcntl(mSocketFD,F_SETFL,O_NONBLOCK);
}

void DatagramSocket::blocking()
{
	fcntl(mSocketFD,F_SETFL,0);
}

void DatagramSocket::close()
{
	::close(mSocketFD);
}


DatagramSocket::~DatagramSocket()
{
	close();
}





int DatagramSocket::write( const char * message, size_t length )
{
	assert(length<=MAX_UDP_LENGTH);
	int retVal = sendto(mSocketFD, message, length, 0,
		(struct sockaddr *)mDestination, addressSize());
	if (retVal == -1 ) perror("DatagramSocket::write() failed");
	return retVal;
}

int DatagramSocket::writeBack( const char * message, size_t length )
{
	assert(length<=MAX_UDP_LENGTH);
	int retVal = sendto(mSocketFD, message, length, 0,
		(struct sockaddr *)mSource, addressSize());
	if (retVal == -1 ) perror("DatagramSocket::write() failed");
	return retVal;
}



int DatagramSocket::write( const char * message)
{
	size_t length=strlen(message)+1;
	return write(message,length);
}

int DatagramSocket::writeBack( const char * message)
{
	size_t length=strlen(message)+1;
	return writeBack(message,length);
}



int DatagramSocket::send(const struct sockaddr* dest, const char * message, size_t length )
{
	assert(length<=MAX_UDP_LENGTH);
	int retVal = sendto(mSocketFD, message, length, 0, dest, addressSize());
	if (retVal == -1 ) perror("DatagramSocket::send() failed");
	return retVal;
}

int DatagramSocket::send(const struct sockaddr* dest, const char * message)
{
	size_t length=strlen(message)+1;
	return send(dest,message,length);
}





int DatagramSocket::read(char* buffer)
{
	socklen_t temp_len = sizeof(mSource);
	int length = recvfrom(mSocketFD, (void*)buffer, MAX_UDP_LENGTH, 0,
	    (struct sockaddr*)&mSource,&temp_len);
	if ((length==-1) && (errno!=EAGAIN)) {
		perror("DatagramSocket::read() failed");
		throw SocketError();
	}
	return length;
}


int DatagramSocket::read(char* buffer, unsigned timeout)
{
	fd_set fds;
	FD_ZERO(&fds);
	FD_SET(mSocketFD,&fds);
	struct timeval tv;
	tv.tv_sec = timeout/1000;
	tv.tv_usec = (timeout%1000)*1000;
	int sel = select(mSocketFD+1,&fds,NULL,NULL,&tv);
	if (sel<0) {
		perror("DatagramSocket::read() select() failed");
		throw SocketError();
	}
	if (sel==0) return -1;
	return read(buffer);
}






UDPSocket::UDPSocket(unsigned short wSrcPort)
	:DatagramSocket()
{
	open(wSrcPort);
}


UDPSocket::UDPSocket(unsigned short wSrcPort,
          	 const char * wDestIP, unsigned short wDestPort )
	:DatagramSocket()
{
	open(wSrcPort);
	destination(wDestPort, wDestIP);
}



void UDPSocket::destination( unsigned short wDestPort, const char * wDestIP )
{
	resolveAddress((sockaddr_in*)mDestination, wDestIP, wDestPort );
}


void UDPSocket::open(unsigned short localPort)
{
	// create
	mSocketFD = socket(AF_INET,SOCK_DGRAM,0);
	if (mSocketFD<0) {
		perror("socket() failed");
		throw SocketError();
	}
	// Set "close on exec" flag to avoid open sockets inheritance by
	// child processes, like 'transceiver'.
	int flags = fcntl(mSocketFD, F_GETFD);
	if (flags >= 0) fcntl(mSocketFD, F_SETFD, flags | FD_CLOEXEC);


	// bind
	struct sockaddr_in address;
	size_t length = sizeof(address);
	bzero(&address,length);
	address.sin_family = AF_INET;
	address.sin_addr.s_addr = INADDR_ANY;
	address.sin_port = htons(localPort);
	if (bind(mSocketFD,(struct sockaddr*)&address,length)<0) {
		perror("bind() failed");
		throw SocketError();
	}
}



unsigned short UDPSocket::port() const
{
	struct sockaddr_in name;
	socklen_t nameSize = sizeof(name);
	int retVal = getsockname(mSocketFD, (struct sockaddr*)&name, &nameSize);
	if (retVal==-1) throw SocketError();
	return ntohs(name.sin_port);
}





UDDSocket::UDDSocket(const char* localPath, const char* remotePath)
	:DatagramSocket()
{
	if (localPath!=NULL) open(localPath);
	if (remotePath!=NULL) destination(remotePath);
}



void UDDSocket::open(const char* localPath)
{
	// create
	mSocketFD = socket(AF_UNIX,SOCK_DGRAM,0);
	if (mSocketFD<0) {
		perror("socket() failed");
		throw SocketError();
	}
	// Set "close on exec" flag to avoid open sockets inheritance by
	// child processes, like 'transceiver'.
	int flags = fcntl(mSocketFD, F_GETFD);
	if (flags >= 0) fcntl(mSocketFD, F_SETFD, flags | FD_CLOEXEC);

	// bind
	struct sockaddr_un address;
	size_t length = sizeof(address);
	bzero(&address,length);
	address.sun_family = AF_UNIX;
	strcpy(address.sun_path,localPath);
	unlink(localPath);
	if (bind(mSocketFD,(struct sockaddr*)&address,length)<0) {
		perror("bind() failed");
		throw SocketError();
	}
}



void UDDSocket::destination(const char* remotePath)
{
	struct sockaddr_un* unAddr = (struct sockaddr_un*)mDestination;
	strcpy(unAddr->sun_path,remotePath);
}


ConnectionSocket::ConnectionSocket(int af, int type, int protocol)
: mSocketFD(-1)
{
	// Create the socket
	mSocketFD = ::socket(af, type, protocol);
	if (mSocketFD < 0) {
//		int errsv = errno;
//		LOG(ERROR) << "socket() failed with errno=" << errsv;
		mSocketFD = -1;
	}
	// Set "close on exec" flag to avoid open sockets inheritance by
	// child processes, like 'transceiver'.
	int flags = fcntl(mSocketFD, F_GETFD);
	if (flags >= 0) fcntl(mSocketFD, F_SETFD, flags | FD_CLOEXEC);
}

ConnectionSocket::~ConnectionSocket()
{
	close();
}

int ConnectionSocket::write( const char * buffer, int length)
{
	// MSG_NOSIGNAL does not allow send() to emit signals on error, it will
	// just return result.
	ssize_t bytesSent = ::send(mSocketFD, buffer, length, MSG_NOSIGNAL);
	if (bytesSent != length) {
//		int errsv = errno;
//		LOG(ERROR) << "send() has sent " << bytesSent << "bytes instead of "
//		           << length << " bytes, errno=" << errsv;
	}
	return bytesSent;
}

int ConnectionSocket::read(char* buffer, size_t length)
{
	// MSG_NOSIGNAL does not allow recv() to emit signals on error, it will
	// just return result.
	int bytesRead = ::recv(mSocketFD, buffer, length, MSG_NOSIGNAL);
	return bytesRead;
}

void ConnectionSocket::nonblocking()
{
	fcntl(mSocketFD,F_SETFL,0);
}

void ConnectionSocket::blocking()
{
	fcntl(mSocketFD,F_SETFL,0);
}

void ConnectionSocket::close()
{
	// shutdown() forces all selects which are blocked on this socket to return
	::shutdown(mSocketFD, SHUT_RDWR);
	::close(mSocketFD);
}

std::ostream& operator<<(std::ostream& os, const ConnectionSocket& conn)
{
	os << "(socket=" << conn.mSocketFD << ")";
	// TODO:: Add host/port information to output.
	return os;
}

ConnectionSocketTCP::ConnectionSocketTCP(int port, const char* address)
: ConnectionSocket(AF_INET, SOCK_STREAM, IPPROTO_TCP)
{
	if (!resolveAddress(&mRemote, address, port)) {
		mRemote.sin_family = AF_UNSPEC;
	}
}

int ConnectionSocketTCP::connect()
{
	return ::connect(mSocketFD, (sockaddr*)&mRemote, sizeof(mRemote));
}

int ConnectionSocketUnix::connect()
{
	return ::connect(mSocketFD, (sockaddr*)&mRemote, sizeof(mRemote));
}

ConnectionServerSocket::ConnectionServerSocket(int af, int type, int protocol)
: mSocketFD(-1)
{
	// Create the socket
	mSocketFD = ::socket(af, type, protocol);
	if (mSocketFD < 0) {
//		int errsv = errno;
//		LOG(ERROR) << "socket() failed with errno=" << errsv;
		mSocketFD = -1;
	}
	// Set "close on exec" flag to avoid open sockets inheritance by
	// child processes, like 'transceiver'.
	int flags = fcntl(mSocketFD, F_GETFD);
	if (flags >= 0) fcntl(mSocketFD, F_SETFD, flags | FD_CLOEXEC);
}

bool ConnectionServerSocket::bindInternal(const sockaddr *addr, int addrlen,
                                          int connectionQueueSize)
{
	int error;

	error = ::bind(mSocketFD, addr, addrlen);
	if (error < 0) {
//		int errsv = errno;
//		LOG(ERROR) << "bind() to port " << bindPort<< " failed with errnp=" << errsv;
		close();
		return false;
	}

	// Setup the queue for connection requests
	error = ::listen(mSocketFD,  connectionQueueSize);
	if (error < 0) {
//		int errsv = errno;
//		LOG(ERROR) << "listen() failed with errno=" << errsv;
		close();
		return false;
	}

	return true;
}

int ConnectionServerSocket::acceptInternal(sockaddr *addr, socklen_t &addrlen)
{
	// Wait for a client to connect.
	int newSocketFD = ::accept(mSocketFD, addr, &addrlen);
	if (newSocketFD < 0) {
//		int errsv = errno;
//		LOG(INFO) << "accept() failed with errno=" << errsv;
		mSocketFD = -1;
	}

	return newSocketFD;
}

void ConnectionServerSocket::close()
{
	// Under Linux we should call shutdown first to unblock blocking accept().
	::shutdown(mSocketFD, SHUT_RDWR);
	::close(mSocketFD);
}

ConnectionServerSocketTCP::ConnectionServerSocketTCP(int bindPort,
                                                     const char* bindAddress)
: ConnectionServerSocket(AF_INET, SOCK_STREAM, IPPROTO_TCP)
{
	const int one = 1;
	if (setsockopt(mSocketFD, SOL_SOCKET, SO_REUSEADDR, (char *)&one, sizeof(one))) {
//		int errsv = errno;
//		LOG(ERROR) << "socket() failed with errno=" << errsv;
	}

#ifdef __APPLE__
	// We use SO_NOSIGPIPE here because MSG_NOSIGNAL is not supported
	// for the write() call under MacOs X.
	if (::setsockopt(mSocketFD, SOL_SOCKET, SO_NOSIGPIPE, (char *)&one, sizeof(one))) {
//		int errsv = errno;
//		LOG(ERROR) << "setsockopt() failed with errno=" << errsv;
		close();
		return false;
	}
#endif

	if (!resolveAddress(&mLocal, bindAddress, bindPort)) {
		mLocal.sin_family = AF_UNSPEC;
	}
}

bool ConnectionServerSocketTCP::bind(int connectionQueueSize)
{
	return bindInternal((sockaddr*)&mLocal, sizeof(mLocal), connectionQueueSize);
}

ConnectionSocket* ConnectionServerSocketTCP::accept()
{
	sockaddr_in newConnectionAddr;
	socklen_t newConnectionAddrLen = sizeof(newConnectionAddr);
	
	int newSocketFD = acceptInternal((sockaddr*)&newConnectionAddr, newConnectionAddrLen);
	if (newSocketFD < 0) {
//		int errsv = errno;
//		LOG(INFO) << "accept() failed with errno=" << errsv;
		return NULL;
	}

	return new ConnectionSocketTCP(newSocketFD, newConnectionAddr);
}

ConnectionServerSocketUnix::ConnectionServerSocketUnix(const std::string &path)
: ConnectionServerSocket(AF_UNIX, SOCK_STREAM, 0)
, mBound(false)
{
	mLocal.sun_family = AF_UNIX;
	strncpy(mLocal.sun_path, path.data(), 108);
	mLocal.sun_path[108-1] = '\0';
}

bool ConnectionServerSocketUnix::bind(int connectionQueueSize)
{
	ConnectionSocketUnix testSocket(mLocal.sun_path);
	if (testSocket.connect() == 0) {
		// Socket is alive. Return error - we can't bind.
		return false;
	} else {
		// Socket is not alive. Remove its file entry to avoid EINVAL.
		::unlink(mLocal.sun_path);
	}
	
	mBound = bindInternal((sockaddr*)&mLocal, sizeof(mLocal), connectionQueueSize);
	return mBound;
}

ConnectionSocket* ConnectionServerSocketUnix::accept()
{
	sockaddr_un newConnectionAddr;
	socklen_t newConnectionAddrLen = sizeof(newConnectionAddr);
	
	int newSocketFD = acceptInternal((sockaddr*)&newConnectionAddr, newConnectionAddrLen);
	if (newSocketFD < 0) {
//		int errsv = errno;
//		LOG(INFO) << "accept() failed with errno=" << errsv;
		return NULL;
	}

	return new ConnectionSocketUnix(newSocketFD, newConnectionAddr);
}

// vim: ts=4 sw=4
