#ifndef JPEG_ENCODE_H
#define JPEG_ENCODE_H

#include "common.h"

#include <opencv2/core/core.hpp>

class jpeg_encode
{
public:
	jpeg_encode();

	void encode(const cv::Mat& mat, bytearray& data, int quality);
};

#endif // JPEG_ENCODE_H
