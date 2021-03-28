#include "opj_encode.h"

#include <iostream>
#include <openjpeg-2.3/openjpeg.h>

using namespace cv;
using namespace std;

opj_image_t *mat2image(const opj_cparameters_t& param, const cv::Mat& mat)
{
	opj_image_t *image;
	opj_image_cmptparm_t cmpparm[4];
	OPJ_COLOR_SPACE color_space;
	int numcomps;

	CLEAR(cmpparm);
	//std::fill(cmpparm[0], cmpparm[3], '\0');

	color_space = OPJ_CLRSPC_SRGB;

	switch (mat.type()) {
		case CV_8UC1:
			numcomps = 1;
			color_space = OPJ_CLRSPC_GRAY;
			break;
		case CV_8UC3:
			numcomps = 3;
			break;
		default:
			numcomps = 4;
			break;
	}
	for(int i = 0; i < numcomps; i++){
		cmpparm[i].prec = 8;
		cmpparm[i].bpp = 8;
		cmpparm[i].sgnd = 0;
		cmpparm[i].dx = param.subsampling_dx;
		cmpparm[i].dy = param.subsampling_dy;
		cmpparm[i].w = mat.cols;
		cmpparm[i].h = mat.rows;
	}

	image = opj_image_create(numcomps, &cmpparm[0], color_space);

	if(!image)
		return 0;

	image->x0 = param.image_offset_x0;
	image->y0 = param.image_offset_y0;
	if(!image->x0){
		image->x1 = (mat.cols - 1) * param.subsampling_dx + 1;
	}else{
		image->x1 = image->x0 + (mat.cols - 1) * param.subsampling_dx + 1;
	}
	if(!image->y0){
		image->y1 = (mat.rows - 1) * param.subsampling_dy + 1;
	}else{
		image->y1 = image->y0 + (mat.rows - 1) * param.subsampling_dy + 1;
	}

	for(int y = 0; y < mat.rows; y++){
		int index = y * mat.cols;
		if(numcomps == 3){
			for(int x = 0; x < mat.cols; x++){
				Vec3b sc = mat.at<Vec3b>(y, x);
				image->comps[0].data[index] = sc[0];
				image->comps[1].data[index] = sc[1];
				image->comps[2].data[index] = sc[2];
				index++;
			}
		}else if(numcomps == 4){
			for(int x = 0; x < mat.cols; x++){
				Vec4b sc = mat.at<Vec4b>(y, x);
				image->comps[0].data[index] = sc[0];
				image->comps[1].data[index] = sc[1];
				image->comps[2].data[index] = sc[2];
				image->comps[3].data[index] = sc[3];
				index++;
			}
		}
	}
	return image;
}

void error_handler(const char *msg, void *client_data)
{
	cout << msg << endl;
}

void info_handler(const char *msg, void *client_data)
{
	//cout << msg << endl;
}

void warning_handler(const char *msg, void *client_data)
{
	cout << msg << endl;
}

struct opjstream{
	opjstream(){
		offset = 0;
	}

	void write(char* buffer, int64_t len){
		if(offset + len > data.size()){
			data.resize(offset + len);
		}
		std::copy(buffer, buffer + len, &data[offset]);
		offset += len;
	}
	void seek(int64_t offs){
		offset = offs;
	}
	void skip(int64_t skp){
		offset += skp;
	}

	bytearray data;
	int64_t offset;
};

OPJ_SIZE_T fn_write(void * p_buffer, OPJ_SIZE_T p_nb_bytes, void * p_user_data)
{
	opjstream *writer = static_cast<opjstream*>(p_user_data);
	if(writer){
		writer->write((char*)p_buffer, p_nb_bytes);
		return writer->data.size();
	}
	return 0;
}

OPJ_SIZE_T fn_read(void * p_buffer, OPJ_SIZE_T p_nb_bytes, void * p_user_data)
{
	return 0;
}

OPJ_BOOL fn_seek(OPJ_OFF_T p_nb_bytes, void * p_user_data)
{
	opjstream *writer = static_cast<opjstream*>(p_user_data);
	if(writer){
		writer->seek(p_nb_bytes);
		return writer->offset;
	}
	return 0;
}

OPJ_OFF_T fn_skip(OPJ_OFF_T p_nb_bytes, void * p_user_data)
{
	opjstream *writer = static_cast<opjstream*>(p_user_data);
	if(writer){
		writer->skip(p_nb_bytes);
	}
	return true;
}

void fn_free(void * p_user_data)
{

}

void encode_frame(const Mat& mat, bytearray& data, int compression)
{
	if(mat.empty())
		return;

	opj_cparameters_t parameters;
	opj_image_t *image = 0;
	opj_codec_t *cinfo = 0;
	opj_stream_t *stream = 0;

	opjstream writer;

	bool res;

	cinfo = opj_create_compress(OPJ_CODEC_J2K);
	if(!cinfo)
		return;

	opj_set_default_encoder_parameters(&parameters);

	parameters.tcp_numlayers = 1;
	parameters.cp_disto_alloc = 1;
	parameters.tcp_rates[0] = compression;

	image = mat2image(parameters, mat);

	if(!image){
		opj_destroy_codec(cinfo);
		return;
	}

	opj_set_warning_handler(cinfo, warning_handler, 0);
	opj_set_error_handler(cinfo, error_handler, 0);

	opj_setup_encoder(cinfo, &parameters, image);

	stream = opj_stream_default_create(OPJ_FALSE);
	opj_stream_set_user_data(stream, &writer, fn_free);
	opj_stream_set_write_function(stream, fn_write);
	opj_stream_set_read_function(stream, fn_read);
	opj_stream_set_seek_function(stream, fn_seek);
	opj_stream_set_skip_function(stream, fn_skip);
	opj_stream_set_user_data_length(stream, 0);

	res = opj_start_compress(cinfo, image, stream);
	if(res){
		res = opj_encode(cinfo, stream);
		if(res)
			res = opj_end_compress(cinfo, stream);
	}

	if(res){
		data = writer.data;
	}

	if(stream)
		opj_stream_destroy(stream);
	if(image)
		opj_image_destroy(image);
	if(cinfo)
		opj_destroy_codec(cinfo);
}
