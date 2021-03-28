#ifndef OPJ_DECODE
#define OPJ_DECODE

#ifndef _CV
#include "utils.h"
#else
#include <opencv2/core/core.hpp>
#endif

#include "common.h"

void encode_frame(const cv::Mat& mat, bytearray& data, int compression);

#endif // OPJ_DECODE

