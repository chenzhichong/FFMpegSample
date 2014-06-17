#pragma once
#include "include/avformat.h"


class CSTMPEG
{
public:
	CSTMPEG();
	~CSTMPEG();

	//打开一个文件写mpeg
	bool OpenMPEG(const char* strfile);
	//写一帧图像数据到mpeg文件
	int AddFrame(int width, int height, int bpp, uint8_t* pRGBBuffer);
	//释放资源
	void CloseMPEG();

	enum MPEG_TYPE {
		VCD_PAL,
		VCD_NTSC,
		SVCD_PAL,
		SVCD_NTSC,
		DVD_PAL,
		DVD_NTSC,
		CUSTOM_MPEG1,
		CUSTOM_MPEG2
	};	

	//设置mpeg的类型
	void SetMPEGFormat(MPEG_TYPE type);
	//设置画面的宽度
	void SetMPEGWidth(int width);
	//设置画面的高度
	void SetMPEGHeight(int height);
	//设置帧率（fps）
	void SetFrameRate(float rate);
	//设置视频码率（kbits/sec rate:3000 4000 6000 8000
	void SetMPEGVideoBitRate(float rate);

private:
	void InitMPEGData(MPEG_TYPE type);

	//把图象颠倒过来
    void RGBBuffer2RGBFrame(AVFrame *pRGBFrame, int width, int height, int bpp, uint8_t* pRGBBuffer);
    AVFrame *alloc_picture(int pix_fmt, int width, int height);

    bool open_video(AVFormatContext *pFormatContext, AVStream *pVideoStream);
    AVStream *add_video_stream(AVFormatContext *pFormatContext, int codec_id);
    bool write_video_frame(AVFormatContext *pFormatcontext, AVStream *pAudioStream);
    void close_video(AVFormatContext *pFormatcontext, AVStream *pVideoStream);
private:
    MPEG_TYPE m_MPEGType;
    int m_nWidth, m_nHeight;
    float m_fFrameRate;
    float m_fBitRate;
    int   m_nFrame;

    AVFrame   *m_pRGBFrame;  //RGB帧数据 
    AVFrame   *m_pYUVFrame;  //YUV帧数据 

    uint8_t*  m_pOutBuf;   //将一帧数据编码到这个缓冲区，用于写入到文件
    const int  m_nOutBufSize;      //编码缓冲区的大小

    //使用流来读写文件
    AVOutputFormat *m_pOutputFormat;
    AVFormatContext *m_pFormatContext;
    AVStream  *m_pVideoStream;
    AVStream  *m_pAudioStream;
};
