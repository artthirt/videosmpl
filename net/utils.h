#ifndef UTILS
#define UTILS

#include <vector>
#include <sstream>
#include <iostream>
#include <memory>
#include <sstream>
#include <ctime>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <linux/videodev2.h>

#include "common.h"

#include <unistd.h>

namespace _cv{

enum CV_TYPE{
	CV_8UC1 = 1,
	CV_8UC3 = 3,
	CV_8UC4 = 4
};

template<typename T, int count>
class Vec_;

class Mat{
public:
	int cols;
	int rows;
	int size;
	std::shared_ptr<ubytearray> data;

	Mat(){
		cols	= 0;
		rows	= 0;
		size	= 0;
		m_type	= 0;
	}
	Mat(int rows, int cols, int type){
		this->rows	= rows;
		this->cols	= cols;
		m_type		= type;
		size		= rows * cols * m_type;
		data = std::shared_ptr<ubytearray>(new ubytearray);
		data.get()->resize(size);
	}
	Mat(const Mat& m){
		rows	= m.rows;
		cols	= m.cols;
		m_type	= m.m_type;
		size	= m.size;
		data	= m.data;
	}

	Mat& operator= (const Mat& m){
		rows	= m.rows;
		cols	= m.cols;
		m_type	= m.m_type;
		size	= m.size;
		data	= m.data;
		return *this;
	}

	template<class T>
	T* row(int i0){
		return (T*)(&(*data.get())[i0 * cols * m_type]);
	}

	template<class T>
	T& at(int i0, int i1){
		int index = cols * i0 * m_type;
		u_char &d = data.get()[index];
		return ((T*)(&d))[i1];
	}
	template<class T>
	const T& at(int i0, int i1) const{
		int index = cols * i0 * m_type;
		u_char &d = (*data.get())[index];
		return ((const T*)(&d))[i1];
	}
	bool empty() const{
		return data.get()->size() == 0;
	}
	int type() const{
		return m_type;
	}

private:
	int m_type;
};

template<typename T, int count>
class Vec_{
public:
	T data[count];

	Vec_(){
		std::fill(data, &data[count], '\0');
	}
	Vec_(T a0){
		data[0] = a0;
	}
	Vec_(T a0, T a1){
		data[0] = a0;
		data[1] = a1;
	}
	Vec_(T a0, T a1, T a2){
		data[0] = a0;
		data[1] = a1;
		data[2] = a2;
	}
	Vec_(T a0, T a1, T a2, T a3){
		data[0] = a0;
		data[1] = a1;
		data[2] = a2;
		data[3] = a3;
	}
	Vec_(const T* val){
		std::copy(val, &val[count], data);
	}

	T& operator[] (int index){
		return data[index];
	}
	T& operator[] (int index) const{
		return data[index];
	}

private:
};

typedef Vec_<u_char, 3> Vec3b;
typedef Vec_<u_char, 4> Vec4b;



class VideoCapture{
public:
	VideoCapture();
	VideoCapture(int dev);
	~VideoCapture();
	void open(int dev);
	void close();
	bool isOpened() const;

	VideoCapture& operator >> (Mat& mat);

private:
	enum INPUT_METHOD{
		IM_READ,
		IM_USERPTR
	};

	struct v4l2_capability cap;
	struct v4l2_crop crop;
	struct v4l2_format fmt;
	int camera;
	bool m_is_open;
	int type;
	ubytearray data;
	INPUT_METHOD m_im;
	std::vector< ubytearray > buffers;

	int xioctl(int fd, int request, void* arg);
	bool init_userptr(int buf_size);
	bool start_stream();
	void get_support_devices();
};

}

#endif // UTILS

