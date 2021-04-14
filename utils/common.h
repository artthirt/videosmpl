#ifndef COMMON
#define COMMON

#include <iostream>
#include <vector>
#include <time.h>
#include <unistd.h>
#include <chrono>
#include <thread>

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

#ifdef _MSC_VER
typedef std::chrono::steady_clock::time_point timepoint;
#else
typedef std::chrono::system_clock::time_point timepoint;
#endif

inline timepoint getNow(){
    return std::chrono::high_resolution_clock::now();
}

inline double getDuration(const timepoint& tm)
{
    auto d = std::chrono::high_resolution_clock::now() - tm;
    double d1 = std::chrono::duration_cast<std::chrono::microseconds>(d).count();
    return d1 / 1000;
}

#endif // COMMON

