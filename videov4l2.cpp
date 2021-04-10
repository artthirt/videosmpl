#include "videov4l2.h"

#include <iostream>
#include <libv4l2.h>
#include <linux/videodev2.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/mman.h>

#include <thread>

union BS{
    uint32_t ui;
    char c[4];
};

#define CLEAR(fmt) memset(&(fmt), 0, sizeof(fmt))

videov4l2::videov4l2(const std::string &dev)
{
    mDev = dev;
}

videov4l2::~videov4l2()
{
    close();
}

bool videov4l2::is_open() const
{
    return mIsOpen;
}

struct Res{
    int w = 0;
    int h = 0;
    Res(){

    }
    Res(int w, int h){
        this->w = w;
        this->h = h;
    }
};

const Res Resolutions[] = {
    Res(1920, 1080),
    Res(1280, 720),
    Res(640, 480),
};

bool videov4l2::open()
{
    if(mIsOpen)
        return true;

    mFd = v4l2_open(mDev.c_str(), O_RDWR | O_NONBLOCK);

    if(mFd < 0)
        return false;

    Res r = Resolutions[mResolutionId];

    struct v4l2_format      fmt;
    CLEAR(fmt);
    fmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    fmt.fmt.pix.width = r.w;
    fmt.fmt.pix.height = r.h;
    fmt.fmt.pix.pixelformat = 0x30314742;//01GB;
    fmt.fmt.pix.field = V4L2_FIELD_ANY;
    xioctl(mFd, VIDIOC_S_FMT, &fmt);
    mWidth = fmt.fmt.pix.width;
    mHeight = fmt.fmt.pix.height;
    mBytesPerLines =fmt.fmt.pix.bytesperline;

    BS bs;
    bs.ui = fmt.fmt.pix.pixelformat;

    struct v4l2_requestbuffers req;
    CLEAR(req);
    req.count = 2;
    req.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    req.memory = V4L2_MEMORY_MMAP;
    xioctl(mFd, VIDIOC_REQBUFS, &req);

    mBuffrs.resize(req.count);
    for(size_t i = 0; i < mBuffrs.size(); ++i){
        struct v4l2_buffer buf;
        CLEAR(buf);
        buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        buf.memory = V4L2_MEMORY_MMAP;
        buf.index = i;
        xioctl(mFd, VIDIOC_QUERYBUF, &buf);

        mBuffrs[i].length = buf.length;
        mBuffrs[i].start = v4l2_mmap(nullptr, buf.length, PROT_READ | PROT_WRITE, MAP_SHARED, mFd, buf.m.offset);
        if (MAP_FAILED == mBuffrs[i].start){
            return false;
        }
    }

    for(size_t i = 0; i < mBuffrs.size(); ++i){
        struct v4l2_buffer buf;
        CLEAR(buf);
        buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        buf.memory = V4L2_MEMORY_MMAP;
        buf.index = i;
        xioctl(mFd, VIDIOC_QBUF, &buf);
    }

    v4l2_buf_type type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    xioctl(mFd, VIDIOC_STREAMON, &type);

    mIsOpen = mFd >= 0;

    return mIsOpen;
}

void videov4l2::close()
{
    mIsOpen = false;

    if(mFd >= 0){

        int res = 0;
        for(size_t i = 0; i < mBuffrs.size(); ++i){
             res = v4l2_munmap(mBuffrs[i].start, mBuffrs[i].length);
        }
        mBuffrs.clear();

        v4l2_buf_type type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        xioctl(mFd, VIDIOC_STREAMOFF, &type);
        v4l2_close(mFd);
        mFd = 0;
    }
}

cv::Mat videov4l2::get()
{
    cv::Mat res(mHeight, mWidth, CV_16UC1);

    if(!is_open())
        return cv::Mat::zeros(mHeight, mWidth, CV_16UC1);

    if(mBuffrs.empty())
        return cv::Mat::zeros(mHeight, mWidth, CV_16UC1);

    v4l2_buffer buf;
    CLEAR(buf);
    buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    buf.memory = V4L2_MEMORY_MMAP;
    xioctl(mFd, VIDIOC_DQBUF, &buf);

    if(mBuffrs.empty() || buf.index > mBuffrs.size())
        return cv::Mat::zeros(mHeight, mWidth, CV_16UC1);

    for(int i = 0; i < mHeight; ++i){
        char *ptr = (char*)mBuffrs[buf.index].start + i * mBytesPerLines;
        memcpy(res.ptr(i), ptr, res.step[0]);
    }
    //memcpy(res.data, mBuffrs[buf.index].start, buf.bytesused);

    xioctl(mFd, VIDIOC_QBUF, &buf);

    return res;
}

void videov4l2::set_exposure(int val)
{
    if(!mIsOpen)
        return;

    struct v4l2_ext_control ctrl;
    CLEAR(ctrl);
    ctrl.id = 0x009a200a;
    ctrl.value64 = val;

    v4l2_ext_controls ctrls;
    CLEAR(ctrls);
    ctrls.ctrl_class = V4L2_CTRL_ID2CLASS(ctrl.id);
    ctrls.count = 1;
    ctrls.controls = &ctrl;

    xioctl(mFd, VIDIOC_S_EXT_CTRLS, &ctrls);
}

void videov4l2::set_resolution_id(int val)
{
    if(val == mResolutionId){
        return;
    }
    mResolutionId = val;
    close();
    //std::this_thread::sleep_for(std::chrono::milliseconds(2000));
    open();
}

bool videov4l2::xioctl(int fd, int request, void *args)
{
    int r;

    do{
        r = v4l2_ioctl(fd, request, args);
    }while(r == -1 && ((errno == EINTR) || (errno == EAGAIN)));

    if(r == -1){
        std::cout << r << std::endl;
        return false;
    }
    return true;
}
