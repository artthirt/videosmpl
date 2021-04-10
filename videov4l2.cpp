#include "videov4l2.h"

#include <iostream>
#include <libv4l2.h>
#include <linux/videodev2.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/mman.h>

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

bool videov4l2::open()
{
    if(mIsOpen)
        return true;

    mFd = v4l2_open(mDev.c_str(), O_RDWR | O_NONBLOCK);

    if(mFd < 0)
        return false;

    struct v4l2_format      fmt;
    CLEAR(fmt);
    fmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    fmt.fmt.pix.width = 1280;
    fmt.fmt.pix.height = 720;
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
        v4l2_buf_type type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        xioctl(mFd, VIDIOC_STREAMOFF, &type);
        v4l2_close(mFd);
        mFd = 0;
    }
}

cv::Mat videov4l2::get()
{
    cv::Mat res(mHeight, mWidth, CV_16UC1);

    v4l2_buffer buf;
    CLEAR(buf);
    buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    buf.memory = V4L2_MEMORY_MMAP;
    xioctl(mFd, VIDIOC_DQBUF, &buf);

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
