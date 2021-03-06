#include "pool_send.h"

#include <functional>

#ifdef _JPEG
#include "jpeg_encode.h"
#else
#include "opj_encode.h"
#endif

const int default_compression	= 50;
const size_t max_list_size		= 3;

///////////////////////////////////////////

Frame::Frame()
{
	done = false;
	compression = default_compression;
}

Frame::Frame(const cv::Mat& mat, int quality)
{
	done = false;
	image = mat;
    compression = quality;
}

Frame::Frame(const Frame& frame)
{
	done = false;
	compression = frame.compression;
	image = frame.image;

    thread = std::thread(std::bind(&Frame::run, this));
}

void Frame::run()
{
#ifdef _JPEG
	jpeg_encode enc;
    enc.encode(image, data, compression);
#else
	encode_frame(image, data, compression);
#endif
	done = true;
}


///////////////////////////////////////////

using namespace std;
using namespace cv;

pool_send::pool_send()
	: m_done(false)
	, m_destination(boost::asio::ip::address::from_string("192.168.0.100"), 10000)
	, m_socket(0)
	, m_serial(1)
{
    mTimeStart = getNow();
}

pool_send::~pool_send()
{
    mThread.join();
	if(m_socket)
		delete m_socket;
}

void pool_send::set_address(const string& address, ushort port)
{
	m_destination.address(boost::asio::ip::address::from_string(address));
	m_destination.port(port);
}

string pool_send::address() const
{
	return m_destination.address().to_string();
}

u_short pool_send::port() const
{
    return m_destination.port();
}

void pool_send::start()
{
    mThread = std::thread(std::bind(&pool_send::run, this));
}

void pool_send::push_frame(const Mat& mat)
{
    if(!isSending())
        return;
	m_mutex.lock();
	if(m_pool.size() > max_list_size){
		m_mutex.unlock();
		return;
	}
    m_pool.push(Frame(mat, mQuality));
	m_mutex.unlock();
}

void pool_send::run()
{
    using namespace boost::asio;

    m_socket = new ip::udp::socket(m_io);
    m_socket->open(ip::udp::v4());

    m_socket->bind(ip::udp::endpoint(ip::udp::v4(), 10000));

    std::thread thread(std::bind(&pool_send::run2, this));

    std::thread thrrcv(std::bind(&pool_send::doReceive, this));

	m_io.run();

	thread.join();
    thrrcv.join();

}
void pool_send::run2()
{
	while(!m_done){
        if(!check_frames()){
            _msleep(10);
        }
	}
}
void pool_send::close()
{
	m_io.stop();
    m_done = true;
}

void pool_send::set_ctrl(funsetexposure exp)
{
    mFunExposure = exp;
}

bool pool_send::check_frames()
{
    if(m_pool.empty())
        return false;

    bool send = false;
	bytearray data;
	m_mutex.lock();
	if(m_pool.size()){
		if(m_pool.front().done){
			data = m_pool.front().data;
			m_pool.front().thread.join();
			m_pool.pop();
		}
	}
	m_mutex.unlock();

	if(data.size()){
		vectorstream vstream = m_asf.generate_stream(data);

		for(size_t i = 0; i < vstream.size(); i++){
			send_data(vstream[i]);
            send = true;
		}
	}
    return send;
}

void pool_send::send_data(const bytearray& data)
{
    if(!data.size() || !m_socket || !mReceivePoint.port())
		return;

    if(getDuration(mTimeStart) > 2 * 1000){
        mReceivePoint = boost::asio::ip::udp::endpoint();
    }

    boost::asio::socket_base::message_flags flags = 0;
    boost::system::error_code err;
    size_t res = m_socket->send_to(boost::asio::buffer(data), mReceivePoint, flags, err);
	//cout << res << endl;
}

void pool_send::write_handler(boost::system::error_code error, size_t size)
{
	if(error !=0 ){
		cout << error << endl;
    }
}

void pool_send::handleReceive(boost::system::error_code error, size_t sz)
{
    size_t size = m_socket->available();

    bytearray buffer;
    buffer.resize(size);

    m_socket->receive(boost::asio::buffer(buffer, size));

    if(!buffer.empty()){
        int *vals = reinterpret_cast<int*>(buffer.data());
        std::cout << vals[0] << "\n";
    }
}

void pool_send::doReceive()
{
    bytearray pkt, cp;
    size_t packetSize = 0;
    pkt.resize(65536);
    boost::asio::ip::udp::endpoint ep;
    for(;!m_done;){
        packetSize = m_socket->receive_from(boost::asio::buffer((pkt)), ep);
        if(packetSize){
            mReceivePoint = ep;
            cp.resize(packetSize);
            std::copy(pkt.begin(), pkt.begin() + packetSize, cp.begin());
            parseData(cp);
        }else{
            std::this_thread::sleep_for(std::chrono::milliseconds(5));
        }
    }
}

void pool_send::parseData(const bytearray &data)
{
    if(!data.empty()){
        const int *vals = reinterpret_cast<const int*>(data.data());
        if(vals[0] == 0xFFFFFFFF){
            mTimeStart = getNow();
            return;
        }
        std::cout << "exposure: " << vals[0] << "\n";
        std::cout << "quality:  " << vals[1] << "\n";
        mQuality = vals[1];
        if(mFunExposure){
            Cntrl cntrl;
            cntrl.exposure        = vals[0];
            cntrl.quality         = vals[1];
            cntrl.resolution_id   = vals[2];
            mFunExposure(cntrl);
        }
    }
}
