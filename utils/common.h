#ifndef COMMON
#define COMMON

#include <iostream>
#include <vector>
#include <time.h>
#include <unistd.h>

typedef std::vector< char > bytearray;
typedef std::vector< u_char > ubytearray;

typedef std::vector< bytearray > vectorstream;

/**
 * @brief _msleep
 * @param ms
 */
void _msleep(long ms);

/**
 * @brief get_curtime_usec
 * @return
 */
int64_t get_curtime_usec();

/**
 * @brief get_curtime_msec
 * @return
 */
int64_t get_curtime_msec();

/**
 * @brief write_file
 * @param fn
 * @param data
 */
void write_file(const std::string fn, const bytearray& data);

/**
 * @brief CLEAR
 * @param val
 */
template<typename T>
inline void CLEAR(T& val)
{
	std::fill((char*)&val, (char*)&val + sizeof(T), '\0');
}

#endif // COMMON

