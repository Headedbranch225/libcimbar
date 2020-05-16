#pragma once

#include "FountainEncoder.h"
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>

class fountain_encoder_stream
{
public:
	static const unsigned _packetSize = 826;
	static const unsigned _headerSize = 3;

protected:
	fountain_encoder_stream(std::string&& data)
		: _data(data)
		, _encoder((uint8_t*)_data.data(), _data.size(), payload_size())
	{
	}

	unsigned payload_size() const
	{
		return _packetSize - _headerSize;
	}

public:
	template <typename STREAM>
	static fountain_encoder_stream create(STREAM& stream)
	{
		std::stringstream buffs;
		if (stream)
			 buffs << stream.rdbuf();
		return fountain_encoder_stream(buffs.str());
	}

	bool good() const
	{
		return _encoder.good();
	}

	unsigned block_count() const
	{
		return _block;
	}

	unsigned blocks_required() const
	{
		return (_data.size() / payload_size()) + 1;
	}

	void encode_new_block()
	{
		unsigned char* payload = _buffer.data() + _headerSize;
		size_t res = _encoder.encode(_block++, payload, payload_size());
		if (res != payload_size())
			_encoder.encode(_block++, payload, payload_size()); // try twice -- the last initial "packet" will be the wrong size

		unsigned block = _block - 1;
		_buffer.data()[0] = (block >> 16) & 0xFF;
		_buffer.data()[1] = (block >> 8) & 0xFF;
		_buffer.data()[2] = block & 0xFF;
		_buffIndex = 0;
	}

	// sometimes we want a new encoded batch, sometimes we just want our buffer
	std::streamsize readsome(char* data, unsigned length)
	{
		std::streamsize totalRead = 0;
		while (length > 0 and good())
		{
			if (_buffIndex >= _buffer.size())
				encode_new_block();

			unsigned readLen = std::min(length, (unsigned)(_buffer.size() - _buffIndex));
			if (!readLen)
				return totalRead;
			totalRead += readLen;

			uint8_t* first = _buffer.data() + _buffIndex;
			std::copy(first, first + readLen, data);

			length -= readLen;
			_buffIndex += readLen;
		}
		return totalRead;
	}

protected:
	std::string _data;
	FountainEncoder _encoder;
	std::array<uint8_t,_packetSize> _buffer;
	unsigned _buffIndex = ~0U;
	unsigned _block = 0;
};
