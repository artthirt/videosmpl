#include <iostream>

#include <fstream>
#include <stdio.h>
#include <stdlib.h>
#include <thread>
#include <vector>
#include <queue>
#include <libv4l2.h>

#include <opencv2/opencv.hpp>

#include "videov4l2.h"
#include "pool_send.h"

using namespace std;
using namespace cv;

void initLUT(std::vector<uint16_t> &V)
{
    V.resize(65536);
    for(int i = 0; i < 65536; ++i){
        float t = 1. * i/65535;
        float v = pow(t, 1/1.5);
        V[i] = 65535. * v;
    }
}

void setLut(cv::Mat& mat, const std::vector<uint16_t>& LUT)
{
    mat.forEach<uint16_t>([&](uint16_t& p, const int pos[]){
        p = LUT[p];
    });
}

struct Worker{

    int mMaxBufferCount = 25;
    std::queue< Mat > mBuffer;
    std::mutex mMutex;
    videov4l2 mVideo;
    pool_send mPool;


    std::thread mThread;

    Worker(){
    }

    ~Worker(){
        mThread.join();
    }

    void start(){
        mPool.start();
    }

    template<typename T>
    void setCntrl(T fun){
        mPool.set_ctrl(fun);
    }
    template<typename T>
    void push_frame(T m){
        mPool.push_frame(m);
    }

    bool open(){
        if(!mVideo.open())
            return false;

        run();

        return true;
    }

    void close(){
        mVideo.close();
    }

    void set_exposure(int val){
        mVideo.set_exposure(val);
    }

    void setResolutionId(int val){
        mVideo.set_resolution_id(val);
    }

    bool empty() const{
        return mBuffer.empty();
    }

    Mat take(){
        mMutex.lock();
        Mat m = mBuffer.front();
        mBuffer.pop();
        mMutex.unlock();
        return m;
    }

    void do_work()
    {
        while(1){
            if(mBuffer.size() > mMaxBufferCount || !mPool.isSending()){
                std::this_thread::sleep_for(std::chrono::milliseconds(32));
                continue;
            }
            auto now = getNow();
            Mat m = mVideo.get();
            if(!m.empty()){
                mMutex.lock();
                mBuffer.push(m);
                mMutex.unlock();
            }
            double duration = getDuration(now);

            if(duration < 16){
                std::this_thread::sleep_for(std::chrono::milliseconds(16 - (int)duration));
            }
        }
    }

    void run(){
        mThread = std::thread(std::bind(&Worker::do_work, this));
    }

};

int main(int argc, char *argv[])
{
#if 0
    std::ifstream f("d:\\develop\\dir16\\videosmpl\\data\\tmp.bin", std::ios_base::binary);
#else
    //std::ifstream f("/dev/video0", std::ios_base::binary);
#endif

//    f.seekg(0, std::ios_base::end);
//    size_t fsize = f.tellg();
//    f.seekg(0, std::ios_base::beg);
	//FILE *fw = fopen("tmp.bin", "w");

//    if(!f.is_open()){
//        return 1;
//    }

    std::vector<uint16_t> LUT;
    initLUT(LUT);

	int id = 0;

    Worker worker;

    if(!worker.open())
        return 1;

    FileStorage xml("config.xml", FileStorage::READ);
    if(xml.isOpened()){
        string address = xml["address"];
        int port = xml["port"];
        worker.mPool.set_address(address, port);
        std::cout << "address: " << worker.mPool.address() << "\nport: " << worker.mPool.port() <<"\n";
    }else{
        FileStorage xml("config.xml", FileStorage::WRITE);
        xml << "address" << worker.mPool.address() << "port" << worker.mPool.port();
        std::cout << "address: " << worker.mPool.address() << "\nport: " << worker.mPool.port() <<"\n";
    }

    worker.start();

    worker.set_exposure(200);

    auto fne = [&worker](Cntrl val){
        worker.setResolutionId(val.resolution_id);
        worker.set_exposure(val.exposure);
    };

    worker.setCntrl(fne);

    double duration = 0;

    Mat mat, mat2;

    while(1){
        {
            if(worker.empty()){
                std::this_thread::sleep_for(std::chrono::milliseconds(32));
                continue;
            }
            auto now = getNow();

            mat = worker.take();

            //memcpy(mat.ptr(), buf.data(), size);
            mat2 = 16 * mat;
            setLut(mat2, LUT);
            cv::cvtColor(mat2, mat2, cv::COLOR_BayerRG2BGR);
            worker.push_frame(mat2);

            duration = getDuration(now);

            if(duration < 16){
                std::this_thread::sleep_for(std::chrono::milliseconds(16 - (int)duration));
            }
			id++;
        }
    }

    worker.close();
    return 0;
}
