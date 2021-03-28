#include "common.h"

#include <fstream>

void _msleep(long ms)
{
	timespec tm;
	tm.tv_nsec = (ms * 1000000) % 1000000000;
	tm.tv_sec = ms / 1000;
	clock_nanosleep(CLOCK_MONOTONIC, 0, &tm, 0);
}

int64_t get_curtime_usec()
{
	timespec tm;
	clock_gettime(CLOCK_MONOTONIC, &tm);
	return (tm.tv_nsec + (tm.tv_sec * 1e+9))/1000;
}

int64_t get_curtime_msec()
{
	int64_t msec = get_curtime_usec();
	return msec / 1000;
}

void write_file(const std::string fn, const bytearray& data)
{
	using namespace std;

	fstream f;
	f.open(fn, ios_base::out | ios_base::binary);
	if(!f.is_open())
		return;

	f.write(&data[0], data.size());
	f.close();
}
