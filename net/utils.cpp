#include "utils.h"

#include <fstream>

using namespace std;
using namespace _cv;

Vec3b YUV_to_Bitmap(int y,int u,int v)
{
   int r,g,b;
   Vec3b bm;

   // u and v are +-0.5
   u -= 128;
   v -= 128;

   // Conversion
   r = y + 1.370705 * v;
   g = y - 0.698001 * v - 0.337633 * u;
   b = y + 1.732446 * u;

/*
   r = y + 1.402 * v;
   g = y - 0.344 * u - 0.714 * v;
   b = y + 1.772 * u;
*/
/*
   y -= 16;
   r = 1.164 * y + 1.596 * v;
   g = 1.164 * y - 0.392 * u - 0.813 * v;
   b = 1.164 * y + 2.017 * u;
*/

   // Clamp to 0..1
   if (r < 0) r = 0;
   if (g < 0) g = 0;
   if (b < 0) b = 0;
   if (r > 255) r = 255;
   if (g > 255) g = 255;
   if (b > 255) b = 255;

   bm[0] = r;
   bm[1] = g;
   bm[2] = b;
   //bm.a = 0;

   return (bm);
}


void Convert422(const unsigned char *yuv, Vec3b &rgb1, Vec3b &rgb2)
{
   int y1,y2,u,v;

   // Extract yuv components
   u  = yuv[0];
   y1 = yuv[1];
   v  = yuv[2];
   y2 = yuv[3];

   // yuv to rgb
   rgb1 = YUV_to_Bitmap(y1,u,v);
   rgb2 = YUV_to_Bitmap(y2,u,v);
}

void get_rgb_from_yuv(const ubytearray& data, int width, int height, Mat& mat)
{
	mat = Mat(height, width,  CV_8UC3);

	for(int i = 0; i < height/2; i++){
		Vec3b *row0 = mat.row<Vec3b>(i);
		Vec3b *row1 = mat.row<Vec3b>(i + 1);
		int k = i * width;
		for(int j = 0; j < width; j++){
			Convert422(&data[k], row0[j], row1[j]);
			k += 4;
		}
	}
}

////////////////////////////////////////////

VideoCapture::VideoCapture()
{
	m_im = IM_READ;
	camera = 0;
	m_is_open = false;
}

VideoCapture::VideoCapture(int dev)
{
	m_im = IM_READ;
	camera = 0;
	m_is_open = false;
	open(dev);
}

VideoCapture::~VideoCapture()
{
	close();
}

void VideoCapture::open(int dev)
{

	using namespace std;

	close();

	int width = 640;
	int height = 480;

	const string linux_video = "/dev/video";
	stringstream sdev;
	sdev << linux_video << dev;

	camera = ::open(sdev.str().c_str(), O_RDONLY);

	if(xioctl(camera, VIDIOC_QUERYCAP, &cap) == -1){
		if(errno == EINVAL){
			cout << "'" << sdev.str() << "' is no video capture device" << endl;
		}else{
			cout << "Error in ioctl VIDIOC_QUERYCAP\n";
		}
		return;
	}

	if(!(cap.capabilities & V4L2_CAP_VIDEO_CAPTURE)){
		cout << "'" << sdev.str() << "' is no video capture device" << endl;
		return;
	}

	if(!(cap.capabilities & V4L2_CAP_READWRITE)){
		cout << "device does not support read i/o ___\n";
		if(!(cap.capabilities & V4L2_CAP_STREAMING)){
			cout << "device does not support streaming\n";
			close();
			return;
		}else{
			m_im = IM_USERPTR;
		}
	}

	get_support_devices();

	if(xioctl(camera, VIDIOC_G_FMT, &fmt) == -1){
		cout << "VIDIOC_G_FMT\n";
	}

	fmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	fmt.fmt.pix.width	= width;
	fmt.fmt.pix.height	= height;

	fmt.fmt.pix.pixelformat = V4L2_PIX_FMT_BGR24;
	fmt.fmt.pix.field		= V4L2_FIELD_ANY;
	if(xioctl(camera, VIDIOC_S_FMT, &fmt) == -1){
		cout << "VIDIOC_S_FMT\n";
		close();
		return;
	}

	if(m_im == IM_USERPTR){
		size_t min = fmt.fmt.pix.width * 3;
		if (fmt.fmt.pix.bytesperline < min)
			fmt.fmt.pix.bytesperline = min;
		min = fmt.fmt.pix.bytesperline * fmt.fmt.pix.height;
		if (fmt.fmt.pix.sizeimage < min)
			fmt.fmt.pix.sizeimage = min;

		if(!init_userptr(fmt.fmt.pix.sizeimage)){
			cout << "not init user ptr\n";
			close();
			return;
		}
		if(!start_stream()){
			close();
			return;
		}
	}

	m_is_open = true;
}

void VideoCapture::close()
{
	if(camera){
		::close(camera);
		camera = 0;
	}
	std::fill((char*)&cap, (char*)&cap + sizeof(cap), '\0');
	std::fill((char*)&crop, (char*)&crop + sizeof(crop), '\0');
	std::fill((char*)&fmt, (char*)&fmt + sizeof(fmt), '\0');

	m_is_open = false;
}

bool VideoCapture::isOpened() const
{
	return m_is_open;
}

VideoCapture& VideoCapture::operator >> (Mat& mat)
{
	using namespace std;

	if(isOpened()){
		if(m_im == IM_READ){
			mat = Mat(fmt.fmt.pix.height, fmt.fmt.pix.height, CV_8UC3);
			int res = read(camera, mat.row<u_char>(0), mat.size);

			if(res <= 0){
				cout << "Frame not captured\n";
			}
		}else if(m_im == IM_USERPTR){
			v4l2_buffer buf;
			CLEAR(buf);
			buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
			buf.memory = V4L2_MEMORY_USERPTR;

			if(xioctl(camera, VIDIOC_DQBUF, &buf) == -1){
				switch (errno) {
					case EAGAIN:
						return *this;
						break;
					default:
						cout << "VIDIOC_DQBUF\n";
						break;
				}
			}

			mat = Mat(fmt.fmt.pix.height, fmt.fmt.pix.width, CV_8UC3);
			std::copy((char*)buf.m.userptr, (char*)buf.m.userptr + buf.bytesused, &(*mat.data.get())[0]);

			fstream fs;
			fs.open("is.txt", ios_base::binary | ios_base::out);
			if(fs.is_open()){
				fs.write((char*)buf.m.userptr, mat.size);
				fs.close();
			}

			if(xioctl(camera, VIDIOC_QBUF, &buf) == -1){
				cout << "VIDIOC_QBUF\n";
			}
		}

	}
	return *this;
}

int VideoCapture::xioctl(int fd, int request, void* arg)
{
	int r;
	do{
		r = ioctl(fd, request, arg);
	}while(r == -1 && errno == EINTR);
	return r;
}

bool VideoCapture::init_userptr(int buf_size)
{
	if(!camera)
		return false;

	struct v4l2_requestbuffers req;
	CLEAR(req);
	req.count = 4;
	req.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	req.memory = V4L2_MEMORY_USERPTR;

	if(xioctl(camera, VIDIOC_REQBUFS, &req) == -1){
		using namespace std;

		if(errno == EINVAL){
			cout << "does not support\n";
		}else{
			cout << "VIDIOC_REQBUFS\n";
		}
		return false;
	}
	buffers.resize(req.count);
	for(size_t i = 0; i < buffers.size(); i++){
		buffers[i].resize(buf_size);
	}

	return true;
}

bool VideoCapture::start_stream()
{
	for(size_t i = 0; i < buffers.size(); i++){
		struct v4l2_buffer buf;
		CLEAR(buf);
		buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
		buf.memory = V4L2_MEMORY_USERPTR;
		buf.index = i;
		buf.m.userptr = (unsigned long)&buffers[i][0];
		buf.length = buffers[i].size();

		if(xioctl(camera, VIDIOC_QBUF, &buf) == -1){
			return false;
		}
	}
	type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	if(xioctl(camera, VIDIOC_STREAMON, &type) == -1){
		return false;
	}
	return true;
}

void VideoCapture::get_support_devices()
{
	using namespace std;

	v4l2_fmtdesc pf;
	CLEAR(pf);
	pf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	while(xioctl(camera, VIDIOC_ENUM_FMT, &pf) == 0){
		char a[5] = { 0 };
		a[0] = (pf.pixelformat) & 0xFF;
		a[1] = (pf.pixelformat >> 8) & 0xFF;
		a[2] = (pf.pixelformat >> 16) & 0xFF;
		a[3] = (pf.pixelformat >> 24) & 0xFF;
		cout << a << endl;
		pf.index++;
	}
}
