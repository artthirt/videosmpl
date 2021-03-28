#ifndef ASF_STREAM_H
#define ASF_STREAM_H

#include <vector>
#include <common.h>

class asf_protocol;

class asf_stream
{
public:
	asf_stream();
	virtual ~asf_stream();

	void set_maxsize_packet(size_t value);
	size_t maxsize_packet() const;
	vectorstream generate_stream(const bytearray& data);

	bytearray add_packet(const bytearray& data) const;
private:
	size_t m_maxpacket_size;
	int m_serial;
	asf_protocol* m_protocol;
};

#endif // ASF_STREAM_H
