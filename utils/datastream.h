#ifndef DATASTREAM_H
#define DATASTREAM_H

#include <algorithm>
#include <vector>
#include <string>
#include <assert.h>

class basicstream{
public:
	enum byteorder{bigendian, littleendian};
	basicstream(): m_byteorder(bigendian){

	}
	virtual ~basicstream(){

	}

	/**
	 * @brief readRawData
	 * @param data
	 * @param len
	 * @return
	 */
	virtual int readRawData(char* data, int len) { return 0; }
	/**
	 * @brief writeRawData
	 * @param data
	 * @param len
	 * @return
	 */
	virtual int writeRawData(char* data, int len) { return 0; }
	virtual size_t size() const { return 0; }
	/**
	 * @brief pos
	 * @return
	 */
	inline int pos() const {return m_pos;}

	void set_byteorder(byteorder order){
		m_byteorder = order;
	}
	byteorder order() const{
		return m_byteorder;
	}

protected:
	/**
	 * @brief inc
	 * @param len
	 */
	inline void inc(int len){ m_pos += len; }
	/**
	 * @brief set_pos
	 * @param pos
	 */
	inline void set_pos(int pos){ m_pos = pos; }

	byteorder m_byteorder;

private:
	int m_pos;

};

class inputstream: public basicstream{
public:
	inputstream(std::vector< char >* source);
	/**
	 * @brief write
	 * @param v
	 */
	template< typename T >
	void write(const T& v){
		int sizetype = sizeof(v);

		allocate(sizetype);

		switch (m_byteorder) {
			case bigendian:
				std::reverse_copy((char*)&v, (char*)&v + sizetype, &(*m_buffer)[pos()]);
				break;
			case littleendian:
			default:
				std::copy((char*)&v, (char*)&v + sizetype, &(*m_buffer)[pos()]);
				break;
		}
		inc(sizetype);
	}
	/**
	 * @brief writeRawData
	 * @param data
	 * @param len
	 * @return
	 */
	virtual int writeRawData(char* data, int len);
	virtual size_t size() const { return m_buffer? m_buffer->size() : 0; }
protected:
	void allocate(size_t len);
private:
	std::vector< char > *m_buffer;
};

class outputstream: public basicstream{
public:
	outputstream(const std::vector< char > &source);
	/**
	 * @brief read
	 * @return
	 */
	template< typename T >
	T read(){
		T v(0);
		size_t sizetype = sizeof(v);
		if(m_buffer.size() < pos() + sizetype)
			return v;

		switch (m_byteorder) {
			case bigendian:
				std::reverse_copy((char*)&m_buffer[pos()], (char*)&m_buffer[pos()] + sizetype, (char*)&v);
				break;
			case littleendian:
			default:
				std::copy((char*)&m_buffer[pos()], (char*)&m_buffer[pos()] + sizetype, (char*)&v);
				break;
		}

		inc(sizetype);
		return v;
	}
	/**
	 * @brief readRawData
	 * @param data
	 * @param len
	 * @return
	 */
	virtual int readRawData(char* data, int len);
	virtual size_t size() const { return m_buffer.size(); }
private:
	std::vector< char > m_buffer;
};

class datastream
{
public:
	datastream(const std::vector< char > &source);
	datastream(std::vector< char > *source);
	~datastream();

	void set_byteorder(basicstream::byteorder order){
		if(m_stream){
			m_stream->set_byteorder(order);
		}
	}
	basicstream::byteorder order() const{
		if(m_stream)
			return m_stream->order();
		return basicstream::bigendian;
	}

	/**
	 * @brief operator >>
	 * @param v
	 * @return
	 */
	template< typename T >
	inline datastream& operator>> (T& v){
		v = T();
		outputstream* os = dynamic_cast< outputstream* >(m_stream);
		assert(os != 0);
		if(os)
			v = os->read<T>();
		return *this;
	}

	/**
	 * @brief operator <<
	 * @param v
	 * @return
	 */
	template< typename T >
	inline datastream& operator<< (T v){
		inputstream *is = dynamic_cast< inputstream* >(m_stream);
		assert(is != 0);
		if(is)
			is->write(v);
		return *this;
	}
	/**
	 * @brief readRawData
	 * @param data
	 * @param len
	 * @return
	 */
	int readRawData(char* data, int len);
	/**
	 * @brief writeRawData
	 * @param data
	 * @param len
	 * @return
	 */
	int writeRawData(char* data, int len);
	/**
	 * @brief size
	 * @return
	 */
	size_t size() const;
	/**
	 * @brief pos
	 * @return
	 */
	size_t pos() const;
private:
	basicstream *m_stream;
};

#endif // DATASTREAM_H
