#include <iostream>

#include <fstream>
#include <stdio.h>
#include <stdlib.h>
#include <thread>
#include <vector>
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

    videov4l2 v;
    if(!v.open())
        return 1;

    int w = 2688;
    int h = 1944;
    cv::Mat mat(h, w, CV_16UC1), mat2, mat3;

    std::vector<uint8_t> buf;
    buf.resize(w * h * 2);

    //cv::namedWindow("ww", cv::WINDOW_NORMAL | cv::WINDOW_FREERATIO);

    auto fn = [&mat, &buf, &w, &h](int d){
        h += d;
        buf.resize(w * h * 2);
        mat = cv::Mat(h, w, CV_16UC1);
        printf("width %d height %d\n", w, h);
    };

	int id = 0;

    bool read = true;

    pool_send pool;

    FileStorage xml("config.xml", FileStorage::READ);
    if(xml.isOpened()){
        string address = xml["address"];
        int port = xml["port"];
        pool.set_address(address, port);
        std::cout << "address: " << pool.address() << "\nport: " << pool.port() <<"\n";
    }else{
        FileStorage xml("config.xml", FileStorage::WRITE);
        xml << "address" << pool.address() << "port" << pool.port();
        std::cout << "address: " << pool.address() << "\nport: " << pool.port() <<"\n";
    }

    pool.start();

    cv::Mat out;

    v.set_exposure(200);

    auto fne = [&v](int val){
        v.set_exposure(val);
    };

    pool.set_exposure(fne);

    int s = 0;
    while(1){
        if(read){
            //f.read((char*)buf.data(), buf.size());
            //s = f.gcount();
            out = v.get();
            s = !out.empty();
        }else{
            s = 1;
        }
        if(s > 0){
//            fwrite(buf.data(), 1, s, fw);
//            for(int i = 0; i < std::min(10, s); ++i){
//                std::cout << (uint16_t)buf[i] << " ";
//            }
//            std::cout << "\n";

            mat = out;

            //memcpy(mat.ptr(), buf.data(), size);
            mat2 = 16 * mat;
            setLut(mat2, LUT);
            cv::cvtColor(mat2, mat2, cv::COLOR_BayerRG2BGR);
            pool.push_frame(mat2);
            //cv::imshow("ww", mat2);
            int key = cv::waitKey(5);
            if(key == 'c'){
                break;
            }
            if(key == 'r'){
                read = !read;
            }
            if(key == '+'){
                fn(1);
            }
            if(key == '-'){
                fn(-1);
            }

			id++;
		}else{
			break;
		}
    }

	mat2.convertTo(mat3, CV_8U, 1./256.);
	cv::imwrite("1.bmp", mat3);

    v.close();

    cout << "Hello World!" << endl;
    return 0;
}
