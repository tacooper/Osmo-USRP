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


#ifndef SOCKETS_H
#define SOCKETS_H

#include <arpa/inet.h>
#include <netdb.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/un.h>
#include <errno.h>
#include <list>
#include <ostream>
#include <assert.h>
#include <stdint.h>




#define MAX_UDP_LENGTH 1500

/** A function to resolve IP host names. */
bool resolveAddress(struct sockaddr_in *address, const char *host, unsigned short port);

/** An exception to throw when a critical socket operation fails. */
class SocketError {};
#define SOCKET_ERROR {throw SocketError(); }

/** Abstract class for connectionless sockets. */
class DatagramSocket {

protected:

	int mSocketFD;				///< underlying file descriptor
	char mDestination[256];		///< address to which packets are sent
	char mSource[256];		///< return address of most recent received packet

public:

	/** An almost-does-nothing constructor. */
	DatagramSocket();

	virtual ~DatagramSocket();

	/** Return the address structure size for this socket type. */
	virtual size_t addressSize() const = 0;

	/**
		Send a binary packet.
		@param buffer The data bytes to send to mDestination.
		@param length Number of bytes to send, or strlen(buffer) if defaulted to -1.
		@return number of bytes written, or -1 on error.
	*/
	int write( const char * buffer, size_t length);

	/**
		Send a C-style string packet.
		@param buffer The data bytes to send to mDestination.
		@return number of bytes written, or -1 on error.
	*/
	int write( const char * buffer);

	/**
		Send a binary packet.
		@param buffer The data bytes to send to mSource.
		@param length Number of bytes to send, or strlen(buffer) if defaulted to -1.
		@return number of bytes written, or -1 on error.
	*/
	int writeBack(const char * buffer, size_t length);

	/**
		Send a C-style string packet.
		@param buffer The data bytes to send to mSource.
		@return number of bytes written, or -1 on error.
	*/
	int writeBack(const char * buffer);


	/**
		Receive a packet.
		@param buffer A char[MAX_UDP_LENGTH] procured by the caller.
		@return The number of bytes received or -1 on non-blocking pass.
	*/
	int read(char* buffer);

	/**
		Receive a packet with a timeout.
		@param buffer A char[MAX_UDP_LENGTH] procured by the caller.
		@param maximum wait time in milliseconds
		@return The number of bytes received or -1 on timeout.
	*/
	int read(char* buffer, unsigned timeout);


	/** Send a packet to a given destination, other than the default. */
	int send(const struct sockaddr *dest, const char * buffer, size_t length);

	/** Send a C-style string to a given destination, other than the default. */
	int send(const struct sockaddr *dest, const char * buffer);

	/** Make the socket non-blocking. */
	void nonblocking();

	/** Make the socket blocking (the default). */
	void blocking();

	/** Close the socket. */
	void close();

};



/** UDP/IP User Datagram Socket */
class UDPSocket : public DatagramSocket {

public:

	/** Open a USP socket with an OS-assigned port and no default destination. */
	UDPSocket( unsigned short localPort=0);

	/** Given a full specification, open the socket and set the dest address. */
	UDPSocket( 	unsigned short localPort, 
			const char * remoteIP, unsigned short remotePort);

	/** Set the destination port. */
	void destination( unsigned short wDestPort, const char * wDestIP );

	/** Return the actual port number in use. */
	unsigned short port() const;

	/** Open and bind the UDP socket to a local port. */
	void open(unsigned short localPort=0);

	/** Give the return address of the most recently received packet. */
	const struct sockaddr_in* source() const { return (const struct sockaddr_in*)mSource; }

	size_t addressSize() const { return sizeof(struct sockaddr_in); }

};


/** Unix Domain Datagram Socket */
class UDDSocket : public DatagramSocket {

public:

	UDDSocket(const char* localPath=NULL, const char* remotePath=NULL);

	void destination(const char* remotePath);

	void open(const char* localPath);

	/** Give the return address of the most recently received packet. */
	const struct sockaddr_un* source() const { return (const struct sockaddr_un*)mSource; }

	size_t addressSize() const { return sizeof(struct sockaddr_un); }

};

/** A base class for connection-oriented sockets. */
class ConnectionSocket {

public:

	/** An almost-does-nothing constructor. */
	ConnectionSocket(int af, int type, int protocol);

protected:
	/** Constructor for the use with ConnectionServerSocket. */
	ConnectionSocket(int socketFD) : mSocketFD(socketFD) {}
public:

	virtual ~ConnectionSocket();

	/**
		Connect to the specified destination.
		@return Same as system connect() function - 0 on success, -1 on error.
	*/
	virtual int connect() =0;

	/**
		Send data.
		@param buffer The data bytes to send.
		@param length Number of bytes to send.
		@return number of bytes written, or -1 on error.
	*/
	int write( const char * buffer, int length);

	/**
		Receive data.
		@param buffer A buffer to read data to.
		@param length Length of the buffer.
		@return The number of bytes received or -1 on non-blocking pass.
	*/
	int read(char* buffer, size_t length);

	/** Make the socket non-blocking. */
	void nonblocking();

	/** Make the socket blocking (the default). */
	void blocking();

	/** Close the socket. */
	void close();

friend std::ostream& operator<<(std::ostream& os, const ConnectionSocket& conn);
friend class ConnectionServerSocket;

protected:

	int mSocketFD;				///< underlying file descriptor

};

class ConnectionSocketTCP : public ConnectionSocket {
	friend class ConnectionServerSocketTCP;
public:
	
	/** Constructor */
	ConnectionSocketTCP(const struct sockaddr_in &remote)
	: ConnectionSocket(AF_INET, SOCK_STREAM, IPPROTO_TCP)
	, mRemote(remote)
	{}

	/** Constructor */
	ConnectionSocketTCP(int port, const char* address);

protected:
	/** Constructor for the use with ConnectionServerSocket. */
	ConnectionSocketTCP(int socketFD, const sockaddr_in &peerAddr)
	: ConnectionSocket(socketFD), mRemote(peerAddr) {}
public:

	/**
		Connect to the specified destination.
		@return Same as system connect() function - 0 on success, -1 on error.
	*/
	virtual int connect();

protected:

	sockaddr_in mRemote;

};

class ConnectionSocketUnix : public ConnectionSocket {
friend class ConnectionServerSocketUnix;
public:
	
	/** Constructor */
	ConnectionSocketUnix(const struct sockaddr_un &remote)
	: ConnectionSocket(AF_UNIX, SOCK_STREAM, 0)
	, mRemote(remote)
	{}

	/** Constructor */
	ConnectionSocketUnix(const std::string &path)
	: ConnectionSocket(AF_UNIX, SOCK_STREAM, 0)
	{
		mRemote.sun_family = AF_UNIX;
		strncpy(mRemote.sun_path, path.data(), 108);
		mRemote.sun_path[108-1] = '\0';
	}

protected:
	/** Constructor for the use with ConnectionServerSocket. */
	ConnectionSocketUnix(int socketFD, const sockaddr_un &peerAddr)
	: ConnectionSocket(socketFD), mRemote(peerAddr) {}
public:

	/**
		Connect to the specified destination.
		@return Same as system connect() function - 0 on success, -1 on error.
	*/
	virtual int connect();

protected:

	sockaddr_un mRemote;

};

/** Abstract class for a server side of connection-oriented sockets. */
class ConnectionServerSocket {

public:

	/** Constructor - creates a socket. */
	ConnectionServerSocket(int af, int type, int protocol);

	virtual ~ConnectionServerSocket() {};

	/**
		Perform bind.
		@return True on success, false otherwise.
	*/
	virtual bool bind(int connectionQueueSize = 16) =0;

	/**
		Wait for the next incoming connection request.
		@return Incoming connection or NULL on error.
	*/
	virtual ConnectionSocket* accept() =0;

	/** Close the socket. */
	void close();

protected:

	int mSocketFD;       ///< underlying file descriptor

	/**
		Bind to given address.
		@return True on success, false otherwise.
	*/
	bool bindInternal(const sockaddr *addr, int addrlen, int connectionQueueSize = 16);

	/**
		Wait for the next incoming connection request.
		@return Same as system accept() - descriptor on success, -1 on error.
	*/
	int acceptInternal(sockaddr *addr, socklen_t &addrlen);

};

class ConnectionServerSocketTCP : public ConnectionServerSocket {
public:

	enum {
		DEFAULT_PORT = 0  ///< Use this value for bindPort to let OS select port for you.
	};

	/** Constructor */
	ConnectionServerSocketTCP(int bindPort, const char* bindAddress);

	/** Destructor */
	virtual ~ConnectionServerSocketTCP() {close();}

	/**
		Perform bind.
		@return True on success, false otherwise.
	*/
	virtual bool bind(int connectionQueueSize = 16);

	/**
		Wait for the next incoming connection request.
		@return Incoming connection or NULL on error.
	*/
	virtual ConnectionSocket* accept();

protected:

	sockaddr_in mLocal;

};

class ConnectionServerSocketUnix : public ConnectionServerSocket {
public:

	/** Constructor */
	ConnectionServerSocketUnix(const std::string &path);

	/** Destructor */
	virtual ~ConnectionServerSocketUnix() {
		close();
		if (mBound) ::unlink(mLocal.sun_path);
	}

	/**
		Perform bind.
		@return True on success, false otherwise.
	*/
	virtual bool bind(int connectionQueueSize = 16);

	/**
		Wait for the next incoming connection request.
		@return Incoming connection or NULL on error.
	*/
	virtual ConnectionSocket* accept();

protected:

	sockaddr_un mLocal;
	bool mBound; ///< Set to true if we successfully bound the socket.

};

#endif



// vim: ts=4 sw=4
