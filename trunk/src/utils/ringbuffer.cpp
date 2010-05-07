#include "ringbuffer.h"

RingBuffer::RingBuffer(int growth, int maxSize) : basicBlockSize(growth), maxBufferSize(maxSize)
{
	buffers << QByteArray();
	clear();
}

bool RingBuffer::isEmpty() const
{
	return tailBuffer == 0 && tail == 0;
}

int RingBuffer::size() const
{
	return bufferSize;
}

int RingBuffer::maximumSize() const
{
	return maxBufferSize;
}

void RingBuffer::clear()
{
	if (!buffers.isEmpty()) {
		QByteArray tmp = buffers[0];
		buffers.clear();
		buffers << tmp;
		if (buffers.at(0).size() != basicBlockSize)
			buffers[0].resize(basicBlockSize);
	}
	head = tail = 0;
	tailBuffer = 0;
	bufferSize = 0;
}

void RingBuffer::truncate(int pos)
{
	if (pos < size())
		chop(size() - pos);
}

void RingBuffer::chop(int bytes)
{
	bufferSize -= bytes;
	if (bufferSize < 0)
		bufferSize = 0;

	for (;;) {
		// special case: head and tail are in the same buffer
		if (tailBuffer == 0) {
			tail -= bytes;
			if (tail <= head)
				tail = head = 0;
			return;
		}

		if (bytes <= tail) {
			tail -= bytes;
			return;
		}

		bytes -= tail;
		buffers.removeAt(tailBuffer);

		--tailBuffer;
		tail = buffers.at(tailBuffer).size();
	}
}

int RingBuffer::skip(int length)
{
	return read(0, length);
}

int RingBuffer::write(QByteArray data)
{
	return write(data.constData(),data.size());
}

int RingBuffer::write(const char *data, int maxLength)
{
	int writeBytes = maxBufferSize>0 ? qMin(maxLength, maxBufferSize - bufferSize) : maxLength;
	char *ptr = writeBytes>0 ? reserve(writeBytes) : NULL;
	if (ptr)
	{
		memcpy(ptr,data,writeBytes);
		return writeBytes;
	}
	return 0;
}

QByteArray RingBuffer::read(int maxLength)
{
	QByteArray tmp;
	tmp.resize(qMin(maxLength, size()));
	read(tmp.data(), tmp.size());
	return tmp;
}

int RingBuffer::read(char *data, int maxLength)
{
	int bytesToRead = qMin(size(), maxLength);
	int readSoFar = 0;
	while (readSoFar < bytesToRead) {
		const char *ptr = readPointer();
		int bytesToReadFromThisBlock = qMin(bytesToRead - readSoFar, nextDataBlockSize());
		if (data)
			memcpy(data + readSoFar, ptr, bytesToReadFromThisBlock);
		readSoFar += bytesToReadFromThisBlock;
		free(bytesToReadFromThisBlock);
	}
	return readSoFar;
}

bool RingBuffer::canReadLine() const
{
	return indexOf('\n') != -1;
}

int RingBuffer::readLine(char *data, int maxLength)
{
	int index = indexOf('\n');
	if (index == -1)
		return read(data, maxLength);
	if (maxLength <= 0)
		return -1;

	int readSoFar = 0;
	while (readSoFar < index + 1 && readSoFar < maxLength - 1) {
		int bytesToRead = qMin((index + 1) - readSoFar, nextDataBlockSize());
		bytesToRead = qMin(bytesToRead, (maxLength - 1) - readSoFar);
		memcpy(data + readSoFar, readPointer(), bytesToRead);
		readSoFar += bytesToRead;
		free(bytesToRead);
	}

	// Terminate it.
	data[readSoFar] = '\0';
	return readSoFar;
}

void RingBuffer::free(int bytes)
{
	bufferSize -= bytes;
	if (bufferSize < 0)
		bufferSize = 0;

	for (;;) {
		int nextBlockSize = nextDataBlockSize();
		if (bytes < nextBlockSize) {
			head += bytes;
			if (head == tail && tailBuffer == 0)
				head = tail = 0;
			break;
		}

		bytes -= nextBlockSize;
		if (buffers.count() == 1) {
			if (buffers.at(0).size() != basicBlockSize)
				buffers[0].resize(basicBlockSize);
			head = tail = 0;
			tailBuffer = 0;
			break;
		}

		buffers.removeAt(0);
		--tailBuffer;
		head = 0;
	}
}

char *RingBuffer::reserve(int bytes)
{
	bufferSize += bytes;

	// if there is already enough space, simply return.
	if (tail + bytes <= buffers.at(tailBuffer).size()) {
		char *writePtr = buffers[tailBuffer].data() + tail;
		tail += bytes;
		return writePtr;
	}

	// if our buffer isn't half full yet, simply resize it.
	if (tail < buffers.at(tailBuffer).size() / 2) {
		buffers[tailBuffer].resize(tail + bytes);
		char *writePtr = buffers[tailBuffer].data() + tail;
		tail += bytes;
		return writePtr;
	}

	// shrink this buffer to its current size
	buffers[tailBuffer].resize(tail);

	// create a new QByteArray with the right size
	buffers << QByteArray();
	++tailBuffer;
	buffers[tailBuffer].resize(qMax(basicBlockSize, bytes));
	tail = bytes;
	return buffers[tailBuffer].data();
}

int RingBuffer::indexOf(char c) const
{
	int index = 0;
	for (int i = 0; i < buffers.size(); ++i) {
		int start = 0;
		int end = buffers.at(i).size();

		if (i == 0)
			start = head;
		if (i == tailBuffer)
			end = tail;
		const char *ptr = buffers.at(i).data() + start;
		for (int j = start; j < end; ++j) {
			if (*ptr++ == c)
				return index;
			++index;
		}
	}
	return -1;
}

int RingBuffer::nextDataBlockSize() const
{
	return (tailBuffer == 0 ? tail : buffers.first().size()) - head;
}

const char *RingBuffer::readPointer() const
{
	return buffers.isEmpty() ? 0 : (buffers.first().constData() + head);
}

QByteArray RingBuffer::peek(int maxLength) const
{
	int bytesToRead = qMin(size(), maxLength);
	if (maxLength <= 0)
		return QByteArray();
	QByteArray ret;
	ret.resize(bytesToRead);
	int readSoFar = 0;
	for (int i = 0; readSoFar < bytesToRead && i < buffers.size(); ++i) {
		int start = 0;
		int end = buffers.at(i).size();
		if (i == 0)
			start = head;
		if (i == tailBuffer)
			end = tail;
		const int len = qMin(ret.size()-readSoFar, end-start);
		memcpy(ret.data()+readSoFar, buffers.at(i).constData()+start, len);
		readSoFar += len;
	}
	Q_ASSERT(readSoFar == ret.size());
	return ret;
}
