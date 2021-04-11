#ifndef VIDEOV4L2_H
#define VIDEOV4L2_H

#include <string>
#include <vector>
#include <mutex>
#include <opencv2/opencv.hpp>

struct buffer{
    void *start = nullptr;
    size_t length = 0;
};

class videov4l2
{
public:
    videov4l2(const std::string &dev = "/dev/video0");
    ~videov4l2();

    bool is_open() const;
    bool open();
    void close();
    cv::Mat get();
    void set_exposure(int val);

    void set_resolution_id(int val);

private:
    std::string mDev;
    int mFd = 0;
    bool mIsOpen = false;
    int mWidth = 0;
    int mHeight = 0;
    std::vector<buffer> mBuffrs;
    int mBytesPerLines = 0;
    std::mutex mMutex;

    int mResolutionId = 0;

    bool xioctl(int fd, int request, void *args);
};

#endif // VIDEOV4L2_H
