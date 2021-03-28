#include "datastream.h"

using namespace std;

inputstream::inputstream(std::vector<char> *source)
	: m_buffer(source)
{
	set_pos(0);
}

int inputstream::writeRawData(char *data, int len)
{
	allocate(len);

	std::copy(data, data + len, &(*m_buffer)[pos()]);
	inc(len);
	return len;
}

void inputstream::allocate(size_t len)
{
	if(pos() + len > m_buffer->size()){
		m_buffer->resize(pos() + len);
	}
}

//////////////////////////////////////

outputstream::outputstream(const std::vector<char> &source)
	: m_buffer(source)
{
	set_pos(0);
}

int outputstream::readRawData(char *data, int len)
{
	int sizelen = std::min<int>(len, m_buffer.size() - pos());
	if(!sizelen)
		return 0;
	std::copy((char*)&m_buffer[pos()], (char*)&m_buffer[pos()] + sizelen, data);
	inc(sizelen);
	return sizelen;
}

//////////////////////////////////////

datastream::datastream(const std::vector<char> &source)
	: m_stream(new outputstream(source))
{

}

datastream::datastream(std::vector<char> *source)
	: m_stream(new inputstream(source))
{

}

datastream::~datastream()
{
	if(m_stream)
		delete m_stream;
}

int datastream::readRawData(char *data, int len)
{
	return m_stream->readRawData(data, len);
}

int datastream::writeRawData(char *data, int len)
{
	return m_stream->writeRawData(data, len);
}

size_t datastream::size() const
{
	return m_stream->size();
}

size_t datastream::pos() const
{
	return m_stream->pos();
}
