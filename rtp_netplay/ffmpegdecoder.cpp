#include "ffmpegdecoder.h"
#include "log.h"
extern "C"
{
#include "libavcodec/avcodec.h"
#include "libavformat/avformat.h"
#include "libswscale/swscale.h"
};

#include <cassert>

namespace {
	AVCodec* pCodec = NULL;
	AVFrame* pFrame=NULL;
	AVFrame* pFrameRGB = NULL;
	AVCodecContext* pCodecCtx=NULL;
	AVCodecParserContext* pParser=NULL;
	AVPacket* pPacket = NULL;
	SwsContext* pImgConvertCtx=NULL;
}

FFmpegDecoder::~FFmpegDecoder()
{
	stop();
}

FFmpegDecoder* FFmpegDecoder::getInstance()
{
	static FFmpegDecoder ins;
	return &ins;
}

void FFmpegDecoder::start()
{
	is_stop = false;
	sem_nalu.init(0);
	thread_decode = new std::thread(decode_thread,this);
}

void FFmpegDecoder::stop()
{
	is_stop = true;
	sem_nalu.post();
	if (thread_decode)
	{
		thread_decode->join();
		delete thread_decode;
		thread_decode = nullptr;
	}
	while (!que_nalu.empty())
	{
		delete[] que_nalu.front().naluData;
		que_nalu.pop();
	}
}

bool FFmpegDecoder::init(CODEC_TYPE type,std::function<void(const uint8_t*, int,int,uint32_t,void*)> _rgb_callback,void* arg)
{
	pf_rgb_callback = _rgb_callback;
	pCall_this = arg;
	av_register_all();
	if (CODEC_TYPE::H264 == type)
	{
		pCodec = avcodec_find_decoder(AV_CODEC_ID_H264);
	}
	else if(CODEC_TYPE::H265 == type)
	{
		pCodec = avcodec_find_decoder(AV_CODEC_ID_H265);
	}
	if (NULL == pCodec)
	{
		LOG_ERR("find decodec fail!");
		return false;
	}

	pCodecCtx = avcodec_alloc_context3(pCodec);
	pCodecCtx->time_base = { 1,25 };
	pCodecCtx->pix_fmt = AV_PIX_FMT_YUV420P;
	pCodecCtx->flags |= AV_CODEC_FLAG_LOW_DELAY;
	if (avcodec_open2(pCodecCtx, pCodec, NULL) < 0)
	{
		LOG_ERR("Could not open codec");
		return false;
	}
	pFrame = av_frame_alloc();
	pFrameRGB = av_frame_alloc();
	if (!pFrame||!pFrameRGB) {
		LOG_ERR("Could not alloc frame");
		return false;
	}
	pParser = av_parser_init(pCodec->id);
	pPacket = av_packet_alloc();
	return true;
}

void FFmpegDecoder::push(const char* data, int len, uint32_t timestamp)
{
	{
		std::unique_lock<std::mutex> locker(mutex_nalu);
		que_nalu.push(NALU(data, len, timestamp));
	}

	sem_nalu.post();
}

void FFmpegDecoder::decode_thread(void* arg)
{
	LOG_INFO("ffmpeg decode thread start! id:%lu", std::this_thread::get_id());

	FFmpegDecoder* pThis = static_cast<FFmpegDecoder*>(arg);
	uint8_t* rgb_buffer = nullptr;

	for (;;)
	{
		pThis->sem_nalu.wait();

		if (pThis->is_stop) break;

		pThis->mutex_nalu.lock();
		NALU nalu = pThis->que_nalu.front();
		pThis->que_nalu.pop();
		pThis->mutex_nalu.unlock();
		
		uint8_t* pData = (uint8_t*)nalu.naluData;
		int size = nalu.naluLen;
		pPacket->pts = nalu.timestamp;
		while (size > 0)
		{	
			int ret = av_parser_parse2(pParser,pCodecCtx,&pPacket->data,&pPacket->size, pData, size, pPacket->pts, AV_NOPTS_VALUE, 0);
			if (ret < 0)
			{
				LOG_ERR("decode parser err!");
				continue;
			}
			pData += ret;
			size -= ret;
		
			if (pPacket->size)
			{
				int num= avcodec_send_packet(pCodecCtx, pPacket);
				if (num < 0)
				{
					LOG_ERR("Error sending a packet for decoding");
					assert(false);
				}

				while (num >= 0)
				{
					num = avcodec_receive_frame(pCodecCtx, pFrame);

					if (num == AVERROR(EAGAIN) || num == AVERROR_EOF) {
						break;
					}
					else if (num < 0)
					{
						LOG_ERR("Error during decoding");
						assert(false);
					}

#if 1
					if (!pImgConvertCtx)
						pImgConvertCtx = sws_getContext(pCodecCtx->width, pCodecCtx->height, AV_PIX_FMT_YUV420P,
							pCodecCtx->width, pCodecCtx->height, AV_PIX_FMT_RGB24, SWS_FAST_BILINEAR, NULL, NULL, NULL);
					if (pImgConvertCtx)
					{
						static int rgb_size = 0;
						if (!rgb_buffer)
						{
							rgb_size = avpicture_get_size(AV_PIX_FMT_BGR24, pCodecCtx->width, pCodecCtx->height);
							rgb_buffer = new uint8_t[rgb_size];
							avpicture_fill((AVPicture*)pFrameRGB, rgb_buffer, AV_PIX_FMT_RGB24, pCodecCtx->width, pCodecCtx->height);
						}
						sws_scale(pImgConvertCtx, pFrame->data, pFrame->linesize, 0, pCodecCtx->height, pFrameRGB->data, pFrameRGB->linesize);
						pThis->pf_rgb_callback(rgb_buffer, pCodecCtx->height, pCodecCtx->width, pFrame->pts,pThis->pCall_this);
					}
					else {
						LOG_ERR("img convert err!");
						assert(false);
					}
#endif
				}
			}
		}
		delete[] nalu.naluData;
	}

	{
		//empty dec
		int temp = avcodec_send_packet(pCodecCtx, NULL);
		while (temp >= 0)
		{
			temp = avcodec_receive_frame(pCodecCtx, pFrame);
		}
	}

	if (rgb_buffer) delete[] rgb_buffer;
	av_parser_close(pParser);
	//avcodec_close(pCodecCtx);
	avcodec_free_context(&pCodecCtx);
	av_packet_free(&pPacket);
	if(pFrame) av_frame_free(&pFrame);
	if(pFrameRGB) av_frame_free(&pFrameRGB);
	if (pImgConvertCtx)
	{
		sws_freeContext(pImgConvertCtx);
		pImgConvertCtx = NULL;
	}

	LOG_INFO("ffmpeg decode thread exit");
}