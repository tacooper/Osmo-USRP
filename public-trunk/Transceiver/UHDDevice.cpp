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


#include "Device.h"
#include "Threads.h"
#include "Logger.h"
#include <uhd/usrp/single_usrp.hpp>
#include <uhd/utils/thread_priority.hpp>


/*
    enableExternalRef - Enable external 10MHz clock reference

    masterClockRate   - Master clock frequency

    rxTimingOffset    - Timing correction in seconds between receive and
                        transmit timestamps. This value corrects for delays on
                        on the RF side of the timestamping point of the device.

    txDelayThreshold  - During transmit alignment, the send packet must arrive
                        on the device before the current device time. This is
                        the minimum difference between current device time read
                        from the device and the time on the transmit packet.

    sampleBufSize     - The receive sample buffer size in bytes. 
*/
const bool enableExternalRef = false;
const double masterClockRate = 100.0e6;
const double rxTimingOffset = .00005;
const double txDelayThreshold = 0.0040;
const size_t sampleBufSize = (1 << 20);


/*
    Sample Buffer - Allows reading and writing of timed samples using OpenBTS
                    or UHD style timestamps. Time conversions are handled
                    internally or accessable through the static convert calls.
*/
class SampleBuffer {
public:
	/** Sample buffer constructor
	    @param len number of 32-bit samples the buffer should hold
	    @param rate sample clockrate 
	    @param timestamp 
	*/
	SampleBuffer(size_t len, double rate);
	~SampleBuffer();

	/** Query number of samples available for reading
	    @param timestamp time of first sample
	    @return number of available samples or error
	*/
	ssize_t availableSamples(TIMESTAMP timestamp) const;
	ssize_t availableSamples(uhd::time_spec_t timestamp) const;

	/** Read and write
	    @param buf pointer to buffer
	    @param len number of samples desired to read or write
	    @param timestamp time of first stample
	    @return number of actual samples read or written or error
	*/
	ssize_t read(void *buf, size_t len, TIMESTAMP timestamp);
	ssize_t read(void *buf, size_t len, uhd::time_spec_t timestamp);
	ssize_t write(void *buf, size_t len, TIMESTAMP timestamp);
	ssize_t write(void *buf, size_t len, uhd::time_spec_t timestamp);

	/** Buffer status string
	    @return a formatted string describing internal buffer state
	*/
	std::string stringStatus() const;

	/** Timestamp conversion
	    @param timestamp a UHD or OpenBTS timestamp
	    @param rate sample rate
	    @return the converted timestamp
	*/
	static uhd::time_spec_t convertTime(TIMESTAMP timestamp, double rate);
	static TIMESTAMP convertTime(uhd::time_spec_t timestamp, double rate);

	/** Formatted error string 
	    @param code an error code
	    @return a formatted error string
	*/
	static std::string stringCode(ssize_t code);

	enum errorCode {
		ERROR_TIMESTAMP = -1,
		ERROR_READ = -2,
		ERROR_WRITE = -3,
		ERROR_OVERFLOW = -4
	};

private:
	uint32_t *data;
	size_t bufferLen;

	double clockRate;

	TIMESTAMP timeStart;
	TIMESTAMP timeEnd;

	size_t dataStart;
	size_t dataEnd;
};


/*
    UHDDevice - UHD implementation of the Device interface. Timestamped samples
                are sent to and received from the device. An intermediate buffer
                on the receive side collects and aligns packets of samples.
                Events and errors such as underruns are reported asynchronously
                by the device and received in a separate thread.
*/
class UHDDevice : public Device {
public:
	UHDDevice(double desiredSampleRate, bool skipRx);
	~UHDDevice();

	bool open();
	bool start();
	bool stop();

	int readSamples(short *buf, int len, bool *overrun, 
			TIMESTAMP timestamp, bool *underrun, unsigned *RSSI);

	int writeSamples(short *buf, int len, bool *underrun, 
			TIMESTAMP timestamp, bool isControl);

	bool updateAlignment(TIMESTAMP timestamp);

	bool setTxFreq(double wFreq);
	bool setRxFreq(double wFreq);

	inline double getSampleRate() { return actualSampleRate; }
	inline double numberRead() { return 0; }
	inline double numberWritten() { return 0; }

	/** Receive and process asynchronous message
	    @return true if message received or false on timeout or error
	*/
	bool recvAsyncMesg();

private:
	uhd::usrp::single_usrp::sptr usrpDevice;

	double desiredSampleRate;
	double actualSampleRate;

	size_t sendSamplesPerPacket;
	size_t recvSamplesPerPacket;

	bool started;
	bool aligned;
	bool skipRx; 

	TIMESTAMP timestampOffset;
	SampleBuffer *recvBuffer;

	std::string stringCode(uhd::rx_metadata_t metadata);
	std::string stringCode(uhd::async_metadata_t metadata);

	Thread asyncEventServiceLoopThread;
	friend void *AsyncMesgServiceLoopAdapter();
};


void *AsyncEventServiceLoopAdapter(UHDDevice *dev)
{
	while (1) {
		dev->recvAsyncMesg();
		pthread_testcancel();
	}
}


UHDDevice::UHDDevice(double desiredSampleRate, bool skipRx)
	: actualSampleRate(0), sendSamplesPerPacket(0), recvSamplesPerPacket(0),
	  started(false), aligned(false), timestampOffset(0), recvBuffer(NULL)
{
	this->desiredSampleRate = desiredSampleRate;
	this->skipRx = skipRx;
}


UHDDevice::~UHDDevice()
{
	stop();

	if (recvBuffer)
		delete recvBuffer;
}


bool UHDDevice::open()
{
	LOG(INFO) << "creating USRP device...";

	// Use the first USRP2
	uhd::device_addr_t dev_addr("type=usrp2");
	try {
		usrpDevice = uhd::usrp::single_usrp::make(dev_addr);
	}
	
	catch(...) {
		LOG(ERROR) << "USRP make failed";
		return false;
	}

	// Number of samples per over-the-wire packet
	sendSamplesPerPacket =
		usrpDevice->get_device()->get_max_send_samps_per_packet();
	recvSamplesPerPacket =
		usrpDevice->get_device()->get_max_recv_samps_per_packet();

	// Set device sample rates
	usrpDevice->set_tx_rate(desiredSampleRate);
	usrpDevice->set_rx_rate(desiredSampleRate);
	actualSampleRate = usrpDevice->get_tx_rate();

	if (actualSampleRate != desiredSampleRate) {
		LOG(ERROR) << "Actual sample rate differs from desired rate";
		return false;
	}
	if (usrpDevice->get_rx_rate() != actualSampleRate) {
		LOG(ERROR) << "Transmit and receive sample rates do not match";
		return false;
	}

	// Create receive buffer
	size_t bufferLen = sampleBufSize / sizeof(uint32_t);
	recvBuffer = new SampleBuffer(bufferLen, actualSampleRate);

	// Set receive chain sample offset 
	timestampOffset = (TIMESTAMP)(rxTimingOffset * actualSampleRate);

	// Set gains to midpoint
	uhd::gain_range_t txGainRange = usrpDevice->get_tx_gain_range();
	usrpDevice->set_tx_gain((txGainRange.max + txGainRange.min) / 2);
	uhd::gain_range_t rxGainRange = usrpDevice->get_rx_gain_range();
	usrpDevice->set_rx_gain((rxGainRange.max + rxGainRange.min) / 2);

	// Set reference clock
	uhd::clock_config_t clock_config;
	clock_config.pps_source = uhd::clock_config_t::PPS_SMA;
	clock_config.pps_polarity = uhd::clock_config_t::PPS_NEG;

	if (enableExternalRef)
		clock_config.ref_source = uhd::clock_config_t::REF_SMA;
	else
		clock_config.ref_source = uhd::clock_config_t::REF_INT;

	usrpDevice->set_clock_config(clock_config);

	// Print configuration
	LOG(INFO) << usrpDevice->get_pp_string();
}


bool UHDDevice::start()
{
	LOG(INFO) << "Starting USRP...";

	if (started) {
		LOG(ERROR) << "Device already started";
		return false;
	}

	// Enable priority scheduling
	uhd::set_thread_priority_safe();

	// Start asynchronous event (underrun check) loop
	asyncEventServiceLoopThread.start(
		(void * (*)(void*))AsyncEventServiceLoopAdapter, (void*)this);

	// Start streaming
	uhd::stream_cmd_t cmd = uhd::stream_cmd_t::STREAM_MODE_START_CONTINUOUS;
	cmd.stream_now = true;
	usrpDevice->set_time_now(uhd::time_spec_t(0.0));

	if (!skipRx)
		usrpDevice->issue_stream_cmd(cmd);

	// Display usrp time
	double timeNow = usrpDevice->get_time_now().get_real_secs();
	LOG(INFO) << "The current time is " << timeNow << " seconds";

	started = true;
	return true;
}


bool UHDDevice::stop()
{
	// Stop streaming
	uhd::stream_cmd_t stream_cmd =
		uhd::stream_cmd_t::STREAM_MODE_STOP_CONTINUOUS;
	usrpDevice->issue_stream_cmd(stream_cmd);

	started = false;
	return true;
}


int UHDDevice::readSamples(short *buf, int len, bool *overrun,
			TIMESTAMP timestamp, bool *underrun, unsigned *RSSI)
{
	if (skipRx)
		return 0;

	// Shift read time with respect to transmit clock
	timestamp += timestampOffset; 

	// Check that timestamp is valid
	ssize_t ret = recvBuffer->availableSamples(timestamp);
	if (ret < 0) {
		LOG(ERROR) << recvBuffer->stringCode(ret);
		LOG(ERROR) << recvBuffer->stringStatus();
		return 0;
	}

	// Receive samples from the usrp until we have enough
	while (recvBuffer->availableSamples(timestamp) < len) {
		uhd::rx_metadata_t metadata;
		uint32_t recvBuf[recvSamplesPerPacket];

		size_t numSamples = usrpDevice->get_device()->recv(
					(void*)recvBuf,
					recvSamplesPerPacket,
					metadata,
					uhd::io_type_t::COMPLEX_INT16,
					uhd::device::RECV_MODE_ONE_PACKET);

		// Recv error in UHD
		if (!numSamples) {
			LOG(ERROR) << stringCode(metadata);
			return 0;
		}

		// Missing timestamp
		if (!metadata.has_time_spec) {
			LOG(ERROR) << "UHD: Received packet missing timestamp";
			return 0;
		}

		ssize_t ret = recvBuffer->write(recvBuf,
						numSamples,
						metadata.time_spec);

		// Continue on local overrun, exit on other errors
		if ((ret < 0)) {
			LOG(ERROR) << recvBuffer->stringCode(ret);
			LOG(ERROR) << recvBuffer->stringStatus();
			if (ret != SampleBuffer::ERROR_OVERFLOW)
				return 0;
		}
	}

	// We have enough samples
	ret = recvBuffer->read(buf, len, timestamp);
	if ((ret < 0) || (ret != len)) {
		LOG(ERROR) << recvBuffer->stringCode(ret);
		LOG(ERROR) << recvBuffer->stringStatus();
		return 0;
	}

	return len;
}


int UHDDevice::writeSamples(short *buf, int len, bool *underrun,
			unsigned long long timestamp,bool isControl)
{
	// No control packets
	if (isControl) {
		LOG(ERROR) << "Control packets not supported";
		return 0;
	}

	uhd::tx_metadata_t metadata;
	metadata.has_time_spec = true;
	metadata.start_of_burst = true;
	metadata.end_of_burst = false;
	metadata.time_spec =
		SampleBuffer::convertTime(timestamp, actualSampleRate);

	if (!aligned) {
		uhd::time_spec_t now = usrpDevice->get_time_now();
		uhd::time_spec_t diff = metadata.time_spec - now;

		if (diff.get_real_secs() < txDelayThreshold) {
			LOG(DEBUG) << "Realigning transmitter";
			*underrun = true;
			return len;
		}

		aligned = true;
	}

	size_t samplesSent = usrpDevice->get_device()->send(buf,
					len,
					metadata,
					uhd::io_type_t::COMPLEX_INT16,
					uhd::device::SEND_MODE_FULL_BUFF);

	if (samplesSent != (unsigned)len)
		LOG(ERROR) << "Sent fewer samples than requested";

	return samplesSent;
}


bool UHDDevice::updateAlignment(TIMESTAMP timestamp)
{
	/* NOP */
	return true;
}


bool UHDDevice::setTxFreq(double wFreq) {
	uhd::tune_result_t tr = usrpDevice->set_tx_freq(wFreq);
	LOG(INFO) << tr.to_pp_string();
	return true;
}


bool UHDDevice::setRxFreq(double wFreq) {
	uhd::tune_result_t tr = usrpDevice->set_rx_freq(wFreq);
	LOG(INFO) << tr.to_pp_string();
	return true;
}


bool UHDDevice::recvAsyncMesg()
{
	uhd::async_metadata_t metadata;
	if (!usrpDevice->get_device()->recv_async_msg(metadata))
		return false;

	// Assume that any error requires resynchronization
	if (metadata.event_code != uhd::async_metadata_t::EVENT_CODE_SUCCESS) {
		aligned = false;
		LOG(DEBUG) << stringCode(metadata);
	}

	return true;
}


std::string UHDDevice::stringCode(uhd::rx_metadata_t metadata)
{
	std::ostringstream ost("UHD: ");

	switch (metadata.error_code) {
	case uhd::rx_metadata_t::ERROR_CODE_NONE:
		ost << "No error";
		break;
	case uhd::rx_metadata_t::ERROR_CODE_TIMEOUT:
		ost << "No packet received, implementation timed-out";
		break;
	case uhd::rx_metadata_t::ERROR_CODE_LATE_COMMAND:
		ost << "A stream command was issued in the past";
		break;
	case uhd::rx_metadata_t::ERROR_CODE_BROKEN_CHAIN:
		ost << "Expected another stream command";
		break;
	case uhd::rx_metadata_t::ERROR_CODE_OVERFLOW:
		ost << "An internal receive buffer has filled";
		break;
	case uhd::rx_metadata_t::ERROR_CODE_BAD_PACKET:
		ost << "The packet could not be parsed";
		break;
	default:
		ost << "Unknown error " << metadata.error_code;
	}

	return ost.str();
}


std::string UHDDevice::stringCode(uhd::async_metadata_t metadata)
{
	std::ostringstream ost("UHD: ");

	switch (metadata.event_code) {
	case uhd::async_metadata_t::EVENT_CODE_SUCCESS:
		ost << "A packet was successfully transmitted";
		break;
	case uhd::async_metadata_t::EVENT_CODE_UNDERFLOW:
		ost << "An internal send buffer has emptied";
		break;
	case uhd::async_metadata_t::EVENT_CODE_SEQ_ERROR:
		ost << "Packet loss between host and device";
		break;
	case uhd::async_metadata_t::EVENT_CODE_TIME_ERROR:
		ost << "Packet time was too late or too early";
		break;
	case uhd::async_metadata_t::EVENT_CODE_UNDERFLOW_IN_PACKET:
		ost << "Underflow occurred inside a packet";
		break;
	case uhd::async_metadata_t::EVENT_CODE_SEQ_ERROR_IN_BURST:
		ost << "Packet loss within a burst";
		break;
	default:
		ost << "Unknown error " << metadata.event_code;
	}

	if (metadata.has_time_spec)
		ost << " at " << metadata.time_spec.get_real_secs() << " sec.";

	return ost.str();
}


SampleBuffer::SampleBuffer(size_t len, double rate)
	: bufferLen(len), clockRate(rate),
	  timeStart(0), timeEnd(0), dataStart(0), dataEnd(0)
{
	data = new uint32_t[len];
}


SampleBuffer::~SampleBuffer()
{
	delete[] data;
}


ssize_t SampleBuffer::availableSamples(TIMESTAMP timestamp) const
{
	if (timestamp < timeStart)
		return ERROR_TIMESTAMP;
	else if (timestamp >= timeEnd)
		return 0;
	else
		return timeEnd - timestamp;
}


ssize_t SampleBuffer::availableSamples(uhd::time_spec_t timespec) const
{
	return availableSamples(convertTime(timespec, clockRate));
}


ssize_t SampleBuffer::read(void *buf, size_t len, TIMESTAMP timestamp)
{
	// Check for valid read
	if (timestamp < timeStart)
		return ERROR_TIMESTAMP;
	if (timestamp >= timeEnd)
		return 0;
	if (len >= bufferLen)
		return ERROR_READ;

	// How many samples should be copied
	size_t numSamples = timeEnd - timestamp;
	if (numSamples > len);
		numSamples = len;

	// Starting index
	size_t readStart = dataStart + (timestamp - timeStart);

	// Read it
	if (readStart + numSamples < bufferLen) {
		size_t numBytes = len * 2 * sizeof(short);
		memcpy(buf, data + readStart, numBytes);
	}
	else {
		size_t firstCopy = (bufferLen - readStart) * 2 * sizeof(short);
		size_t secondCopy = len * 2 * sizeof(short) - firstCopy;

		memcpy(buf, data + readStart, firstCopy);
		memcpy((char*) buf + firstCopy, data, secondCopy);
	}

	dataStart = (readStart + len) % bufferLen;
	timeStart = timestamp + len;

	if (timeStart > timeEnd)
		return ERROR_READ;
	else
		return numSamples;
}


ssize_t SampleBuffer::read(void *buf, size_t len, uhd::time_spec_t timeSpec)
{
	return read(buf, len, convertTime(timeSpec, clockRate));
}


ssize_t SampleBuffer::write(void *buf, size_t len, TIMESTAMP timestamp)
{
	// Check for valid write
	if ((len == 0) || (len >= bufferLen))
		return ERROR_WRITE; 
	if ((timestamp + len) <= timeEnd)
		return ERROR_TIMESTAMP;

	// Starting index
	size_t writeStart = (dataStart + (timestamp - timeStart)) % bufferLen;

	// Write it
	if ((writeStart + len) <= bufferLen) {
		size_t numBytes = len * 2 * sizeof(short);
		memcpy(data + writeStart, buf, numBytes);
	}
	else {
		size_t firstCopy = (bufferLen - writeStart) * 2 * sizeof(short);
		size_t secondCopy = len * 2 * sizeof(short) - firstCopy;

		memcpy(data + writeStart, buf, firstCopy);
		memcpy(data, (char*) buf + firstCopy, secondCopy);
	}

	dataEnd = (writeStart + len) % bufferLen;
	timeEnd = timestamp + len;

	if (((writeStart + len) > bufferLen) && (dataEnd > dataStart))
		return ERROR_OVERFLOW;
	else if (timeEnd <= timeStart)
		return ERROR_WRITE; 
	else
		return len;
}


ssize_t SampleBuffer::write(void *buf, size_t len, uhd::time_spec_t timeSpec)
{
	return write(buf, len, convertTime(timeSpec, clockRate));
}


uhd::time_spec_t SampleBuffer::convertTime(TIMESTAMP ticks, double rate)
{
	double secs = (double) ticks / rate;
	return uhd::time_spec_t(secs);
}


TIMESTAMP SampleBuffer::convertTime(uhd::time_spec_t timeSpec, double rate)
{
	size_t secTicks = timeSpec.get_full_secs() * rate;
	return timeSpec.get_tick_count(rate) + secTicks;
}


std::string SampleBuffer::stringStatus() const
{
	std::ostringstream ost("Sample buffer: ");

	ost << "length = " << bufferLen;
	ost << ", timeStart = " << timeStart;
	ost << ", timeEnd = " << timeEnd;
	ost << ", dataStart = " << dataStart;
	ost << ", dataEnd = " << dataEnd;

	return ost.str(); 
}


std::string SampleBuffer::stringCode(ssize_t code)
{
	switch (code) {
	case ERROR_TIMESTAMP:
		return "Sample buffer: Requested timestamp is not valid";
	case ERROR_READ:
		return "Sample buffer: Read error";
	case ERROR_WRITE:
		return "Sample buffer: Write error";
	case ERROR_OVERFLOW:
		return "Sample buffer: Overrun";
	default:
		return "Sample buffer: Unknown error";
	}
}


Device *Device::make(double sampleRate, bool skipRx)
{
	return new UHDDevice(sampleRate, skipRx);
}


short Device::convertHostDeviceShort(short value)
{
	// Type conversion handled internally by UHD
	return value;
}


short Device::convertDeviceHostShort(short value)
{
	// Type conversion handled internally by UHD
	return value;
}
