/*
* Copyright 2008, 2009, 2010 Free Software Foundation, Inc.
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

#ifndef _DEVICE_H_
#define _DEVICE_H_

/** a 64-bit virtual timestamp for device data */
typedef unsigned long long TIMESTAMP;

class Device {
public:
	/** Factory method */
	static Device *make(double desiredSampleRate, bool skipRx = false);

	/** Open, start, and stop the device
	    @return success or fail
	*/
	virtual bool open() = 0;
	virtual bool start() = 0;
	virtual bool stop() = 0;
	virtual void setPriority() = 0;

	/** Read samples from the USRP.
	    @param buf preallocated buf to contain read result
	    @param len number of samples desired
	    @param overrun set if read buffer has been overrun
	    @param timestamp time of the first samples to be read
	    @param underrun set if USRP does not have data to transmit
	    @param RSSI received signal strength of the read result
	    @return number of samples actually read
	*/
	virtual int readSamples(short *buf, int len, bool *overrun,
				TIMESTAMP timestamp = 0xffffffff,
				bool *underrun = 0,
				unsigned *RSSI = 0) = 0;

	/** Write samples to the USRP.
	    @param buf contains the data to be written
	    @param len number of samples to write
	    @param underrun set if USRP does not have data to transmit
	    @param timestamp time of the first sample of the data buffer
	    @param isControl set if data is a control packet
	    @return number of samples actually written
	    */
	virtual int writeSamples(short *buf, int len, bool *underrun,
				TIMESTAMP timestamp = 0xffffffff,
				bool isControl = false) = 0;

	/** Update the alignment between the read and write timestamps
	    @return success or fail
	*/
	virtual bool updateAlignment(TIMESTAMP timestamp) = 0;

	/** Set transmit and receive frequencies
	    @param freq desired frequency
	    @return success or fail
	*/
	virtual bool setTxFreq(double freq) = 0;
	virtual bool setRxFreq(double freq) = 0;

	/** Return internal status values */
	virtual double getSampleRate() = 0;
	virtual double numberRead() = 0;
	virtual double numberWritten() = 0;

	/** Virtual destructor */
	virtual ~Device() { }
};

#endif // _DEVICE_H_
