#ifndef POOL_SEND
#define POOL_SEND

#include <vector>
#include <string>
#include <atomic>
#include <mutex>
#include <queue>
#include <thread>
#include <functional>

#include <boost/asio/ip/udp.hpp>
#include <boost/asio/error.hpp>

#include "common.h"

#ifndef _CV
#include "utils.h"
#else
#include <opencv2/core/core.hpp>
#endif

#include "asf_stream.h"

struct Cntrl{
    int exposure = 0;
    int quality = 50;
    int resolution_id = 0;
};

typedef std::function<void(Cntrl)> funsetexposure;

struct Frame{
	Frame();
    Frame(const cv::Mat& mat, int quality);
	Frame(const Frame& frame);
	void run();

    std::thread thread;
	int compression;
	cv::Mat image;
	bytearray data;
	bool done;
};

class pool_send{
public:
	pool_send();
	~pool_send();

	void set_address(const std::string& address, ushort port);
	std::string address() const;
	u_short port() const;

    void start();

	void push_frame(const cv::Mat& mat);
	void run();
	void run2();
	void close();

    void set_ctrl(funsetexposure exp);

    bool check_frames();
	void send_data(const bytearray& data);
	void write_handler(boost::system::error_code error, size_t size);

private:
	bool m_done;
    int mQuality = 50;
	std::queue< Frame > m_pool;
	std::mutex m_mutex;
	int m_serial;
	asf_stream m_asf;
	boost::asio::ip::udp::socket *m_socket;
	boost::asio::ip::udp::endpoint m_destination;
	boost::asio::io_service m_io;
    boost::asio::ip::udp::endpoint mReceivePoint;
    timepoint mTimeStart;

    funsetexposure mFunExposure;

    std::thread mThread;

    void handleReceive(boost::system::error_code error, size_t sz);
    void doReceive();
    void parseData(const bytearray& data);
};

#endif // POOL_SEND

