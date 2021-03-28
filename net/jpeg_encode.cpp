#include "jpeg_encode.h"

#include <jpeglib.h>

struct bufstream{
	enum{ BLOCK_SIZE = 16384};
	bufstream(){
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
	size_t add_block(){
		size_t old_size = data.size();
		data.resize(old_size + BLOCK_SIZE);
		return old_size;
	}
	const u_char *ptr() const{
		return &data[0];
	}
	u_char *ptr(){
		return &data[0];
	}
	u_char *ptr(size_t offset){
		return &data[offset];
	}
	void resize(size_t size)
	{
		data.resize(size);
	}

	size_t size()const{
		return data.size();
	}

	ubytearray data;
	int64_t offset;
};

void init_dst(j_compress_ptr cinfo)
{
	bufstream* bs = (bufstream*)cinfo->client_data;
	bs->add_block();
	cinfo->dest->next_output_byte = bs->ptr();
	cinfo->dest->free_in_buffer = bs->size();
}

boolean empty_output_buffer(j_compress_ptr cinfo)
{
	bufstream* bs = (bufstream*)cinfo->client_data;
	size_t os = bs->add_block();
	cinfo->dest->next_output_byte = bs->ptr(os);
	cinfo->dest->free_in_buffer = bs->size() - os;
	return true;
}

void term_dst(j_compress_ptr cinfo)
{
	bufstream* bs = (bufstream*)cinfo->client_data;
	bs->resize(bs->size() - cinfo->dest->free_in_buffer);
}

jpeg_encode::jpeg_encode()
{

}

void jpeg_encode::encode(const cv::Mat &_mat, bytearray &data, int quality)
{
	struct jpeg_compress_struct cinfo;

	struct jpeg_error_mgr jerr;
	struct jpeg_destination_mgr dst;
	JSAMPROW row_pointer[1];
	int row_stride;
	bufstream bufstr;

    cv::Mat mat = _mat;
    if(mat.depth() == CV_16U){
        mat.convertTo(mat, CV_8U, 1./255);
    }

	CLEAR(cinfo);

	cinfo.err = jpeg_std_error(&jerr);

	jpeg_create_compress(&cinfo);

	cinfo.image_width = mat.cols;
	cinfo.image_height = mat.rows;

    if(mat.channels() == 3){
        cinfo.input_components = 3;
        cinfo.in_color_space = JCS_RGB;
    }else if(mat.channels() == 4){
        cinfo.input_components = 4;
        cinfo.in_color_space = JCS_EXT_RGBA;
    }else if(mat.channels() == 1){
        cinfo.input_components = 1;
        cinfo.in_color_space = JCS_GRAYSCALE;
    }

	dst.init_destination = init_dst;
	dst.empty_output_buffer = empty_output_buffer;
	dst.term_destination = term_dst;
	cinfo.dest = &dst;
	cinfo.client_data = &bufstr;

	jpeg_set_defaults(&cinfo);

	jpeg_set_quality(&cinfo, quality, TRUE);

	jpeg_start_compress(&cinfo, TRUE);

	row_stride = cinfo.image_width * cinfo.input_components;

	while(cinfo.next_scanline < cinfo.image_height){
		uchar* b = &mat.data[cinfo.next_scanline * row_stride];
		row_pointer[0] = b;//mat.at< JSAMPROW >(index++);
		jpeg_write_scanlines(&cinfo, row_pointer, 1);
	}

	jpeg_finish_compress(&cinfo);
	jpeg_destroy_compress(&cinfo);

	if(bufstr.size()){
		data.resize(bufstr.size());
		std::copy(bufstr.ptr(), bufstr.ptr() + bufstr.size(), &data[0]);
	}
}

