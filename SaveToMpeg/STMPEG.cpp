#include "StdAfx.h"
#include "stmpeg.h"

#define LineWidthBytes(biWidth, biBitCount) ((biWidth * biBitCount + 31) / 32 * 4)

CSTMPEG::CSTMPEG(): m_nOutBufSize(200000)
{
    m_MPEGType  = VCD_PAL;
    m_nWidth  = 352;
    m_nHeight  = 288;
    m_fFrameRate = 25;
    m_fBitRate  = 800000;
    m_nFrame  = -1; 

    m_pRGBFrame     = NULL; 
    m_pYUVFrame     = NULL;

    m_pFormatContext= NULL;
    m_pVideoStream = NULL;
    m_pAudioStream = NULL;
}

CSTMPEG::~CSTMPEG(void)
{
}

void CSTMPEG::InitMPEGData(MPEG_TYPE type)
{
    switch(type) {
    case VCD_PAL:
    {
        m_nWidth = 352;
        m_nHeight = 288;
        m_fFrameRate = 25;
    }
    break;
    case VCD_NTSC:
    {
        m_nWidth = 352;
        m_nHeight = 240;
        m_fFrameRate = 29.97;
    }
    break;
    case SVCD_PAL:
    {
        m_nWidth = 480;
        m_nHeight = 576;
        m_fFrameRate = 25;
    }
    break;
    case SVCD_NTSC:
    {
        m_nWidth = 480;
        m_nHeight = 480;
        m_fFrameRate = 23.976;
    }
    break;
    case DVD_PAL:
    {
        m_nWidth = 720;
        m_nHeight = 576;
        m_fFrameRate = 25;
    }
    break;
    case DVD_NTSC:
    {
        m_nWidth = 720;
        m_nHeight = 480;
        m_fFrameRate = 29.97;
    }
    break;
    default:
    break;
  }
}

void CSTMPEG::SetMPEGFormat(MPEG_TYPE type)
{
    if (type != m_MPEGType) {
        m_MPEGType = type;
        InitMPEGData(type);
    }
}

void CSTMPEG::SetMPEGWidth(int width)
{
    if (m_MPEGType == CUSTOM_MPEG1 || m_MPEGType == CUSTOM_MPEG2 ) 
        m_nWidth = width;
}

void CSTMPEG::SetMPEGHeight(int height)
{
    if (m_MPEGType == CUSTOM_MPEG1 || m_MPEGType == CUSTOM_MPEG2 ) 
        m_nHeight = height;
}

void CSTMPEG::SetFrameRate(float rate)
{
    if (m_MPEGType == CUSTOM_MPEG1 || m_MPEGType == CUSTOM_MPEG2 ) 
        m_fFrameRate = rate;
}

void CSTMPEG::SetMPEGVideoBitRate(float rate)
{
    m_fBitRate = rate * 1000;
}

void CSTMPEG::RGBBuffer2RGBFrame(AVFrame *pRGBFrame, int width, int height, int bpp, uint8_t* pRGBBuffer)
{
    ASSERT( pRGBFrame && pRGBBuffer);
    int linebytes = LineWidthBytes(width, bpp);

    int nPixels = bpp / 8;

    int x, y;
    for(y=0;y<height;y++) {
        for(x=0;x<width;x++) {
            pRGBFrame->data[0][y * pRGBFrame->linesize[0] + x * 3 ] = 
                                                      pRGBBuffer[ (height - y - 1)  * linebytes + x * nPixels  ];     //B
            pRGBFrame->data[0][y * pRGBFrame->linesize[0] + x * 3 + 1 ] = 
                                                      pRGBBuffer[ (height - y - 1)  * linebytes + x * nPixels + 1]; //G
            pRGBFrame->data[0][y * pRGBFrame->linesize[0] + x * 3 + 2 ] = 
                                                      pRGBBuffer[ (height - y - 1)  * linebytes + x * nPixels + 2]; //R
        }
    }
}

AVFrame *CSTMPEG::alloc_picture(int pix_fmt, int width, int height)
{
    AVFrame* picture = avcodec_alloc_frame();
    if (!picture)
        return NULL;

    int size = avpicture_get_size(pix_fmt, width, height);
    uint8_t *picture_buf = (uint8_t *)malloc(size);
    if (!picture_buf) {
        av_free(picture);
        return NULL;
    }

    avpicture_fill((AVPicture *)picture, picture_buf,
    pix_fmt, width, height);
 
    return picture;
}

bool CSTMPEG::OpenMPEG(const char* strfile)
{   
    av_register_all();

    switch(m_MPEGType) {
    case VCD_PAL:
    case VCD_NTSC:
        m_pOutputFormat = guess_format("vcd", NULL, NULL);
    break;
    case SVCD_PAL:
    case SVCD_NTSC:
        m_pOutputFormat = guess_format("svcd", NULL, NULL);
    break;
    case DVD_PAL:
    case DVD_NTSC:
        m_pOutputFormat = guess_format("dvd", NULL, NULL);
    break;
    default:
        m_pOutputFormat = guess_format(NULL, strfile, NULL);
    break;
    }

    //初始化输出文件格式
    if ( NULL == m_pOutputFormat)
        m_pOutputFormat = guess_format("mpeg", NULL, NULL);

    if ( NULL == m_pOutputFormat)
        return false;

    //初始化文件格式上下文
    m_pFormatContext = av_alloc_format_context();
    if ( NULL == m_pFormatContext ) 
        return false;

    m_pFormatContext->oformat = m_pOutputFormat;
    _sntprintf(m_pFormatContext->filename, sizeof(m_pFormatContext->filename), "%s", strfile);

    //创建视频流，初始化编解码器上下文
    if (m_pOutputFormat->video_codec != CODEC_ID_NONE)
    m_pVideoStream = add_video_stream(m_pFormatContext, m_pOutputFormat->video_codec);

    //创建音频流，初始化编解码器上下文
    // if (m_pOutputFormat->audio_codec != CODEC_ID_NONE)
    // m_pAudioStream = add_audio_stream(m_pFormatContext, m_pOutputFormat->audio_codec);

    if (av_set_parameters(m_pFormatContext, NULL) < 0)
        return false; 

    //打开编解码器，给每一帧分配空间
    if (m_pVideoStream){
        if( !open_video(m_pFormatContext, m_pVideoStream))
            return false;
    }

    // if (m_pAudioStream)
    // open_audio(m_pFormatContext, m_pAudioStream);

    if ( !(m_pOutputFormat->flags & AVFMT_NOFILE) ) {
        if (url_fopen(&m_pFormatContext->pb, strfile, URL_WRONLY) < 0) {
            return false;
        }
    }

    //写文件头 
    av_write_header(m_pFormatContext);

    return true;
}

AVStream* CSTMPEG::add_video_stream(AVFormatContext *pFormatContext, int codec_id)
{
    ASSERT(pFormatContext);

    AVStream *pStream = av_new_stream(pFormatContext, 0);
    if ( NULL == pStream )
        return NULL;

    AVCodecContext *pCodecContext = pStream->codec;

    pCodecContext->codec_id   = (CodecID)codec_id;
    pCodecContext->codec_type  = CODEC_TYPE_VIDEO;
    pCodecContext->bit_rate   = 1150000;//800000;//m_fBitRate;
    pCodecContext->width   = m_nWidth;
    pCodecContext->height   = m_nHeight;
    pCodecContext->time_base.den = 25;//m_fFrameRate;
    pCodecContext->time_base.num = 1;
    pCodecContext->gop_size   = 18; 
    pCodecContext->pix_fmt   = PIX_FMT_YUV420P;
 
    if (pCodecContext->codec_id == CODEC_ID_MPEG2VIDEO)
        pCodecContext->max_b_frames = 2;
 
    if (pCodecContext->codec_id == CODEC_ID_MPEG1VIDEO)
        pCodecContext->mb_decision=2;

    if(!strcmp(pFormatContext->oformat->name,  "mp4") || !strcmp(pFormatContext->oformat->name, "mov") ||
                      !strcmp(pFormatContext->oformat->name, "3gp"))
        pCodecContext->flags |= CODEC_FLAG_GLOBAL_HEADER;

    return pStream;
}

bool CSTMPEG::open_video(AVFormatContext *pFormatContext, AVStream *pVideoStream)
{
    AVCodec   *pCodec     = NULL;
    AVCodecContext *pCodecContext = NULL;

    pCodecContext = pVideoStream->codec;

    // find the video encoder 
    pCodec = avcodec_find_encoder(pCodecContext->codec_id);
    if (NULL == pCodec)
        return false;

    // open the codec 
    if (avcodec_open(pCodecContext, pCodec) < 0)  return false;

    m_pYUVFrame = alloc_picture(pCodecContext->pix_fmt, pCodecContext->width, pCodecContext->height);
    if (!m_pYUVFrame) return false; 

    m_pRGBFrame = alloc_picture(PIX_FMT_BGR24, pCodecContext->width, pCodecContext->height);
    if (!m_pRGBFrame) return false;

    m_pOutBuf = (uint8_t*)malloc(m_nOutBufSize);
    if (!m_pOutBuf) return false;

    return true;
}

 

int CSTMPEG::AddFrame(int width, int height, int bpp, uint8_t* pRGBBuffer)
{
    ASSERT(pRGBBuffer);

    RGBBuffer2RGBFrame(m_pRGBFrame, width, height, bpp, pRGBBuffer);
    if ( false == write_video_frame(m_pFormatContext, m_pVideoStream) ) {
        return -1;
    }

    m_nFrame++;

    return 0;
}

bool CSTMPEG::write_video_frame(AVFormatContext *pFormatcontext, AVStream *pVideoStream)
{
    ASSERT(pFormatcontext && pVideoStream);

    int ret;

    AVCodecContext *pCodecContext = pVideoStream->codec;

    img_convert( (AVPicture *)m_pYUVFrame, pCodecContext->pix_fmt, (AVPicture *)m_pRGBFrame, 
                                                                   PIX_FMT_BGR24, pCodecContext->width, pCodecContext->height);

    int out_size = avcodec_encode_video(pCodecContext, m_pOutBuf, m_nOutBufSize, m_pYUVFrame);
    if (out_size > 0){
        AVPacket pkt;
        av_init_packet(&pkt);

        pkt.pts = av_rescale_q(pCodecContext->coded_frame->pts, pCodecContext->time_base, 
                                                   pVideoStream->time_base);
        if( pCodecContext->coded_frame->key_frame )
            pkt.flags |= PKT_FLAG_KEY;
  
        pkt.stream_index = pVideoStream->index;
        pkt.data= m_pOutBuf;
        pkt.size= out_size;

        ret = av_write_frame(pFormatcontext, &pkt);
    }else{
        ret = 0;
    }

    if (ret != 0)    return false;

    return true;
}

void CSTMPEG::close_video(AVFormatContext *pFormatcontext, AVStream *pVideoStream)
{
    avcodec_close(pVideoStream->codec);

    if (m_pYUVFrame) {
        free(m_pYUVFrame->data[0]);
        av_free(m_pYUVFrame);
    }

    if (m_pRGBFrame) {
        free(m_pRGBFrame->data[0]);
        av_free(m_pRGBFrame);
    }

    free(m_pOutBuf);
}

void CSTMPEG::CloseMPEG()
{
    //关闭编码器，释放帧占用的内存；
    close_video(m_pFormatContext, m_pVideoStream);
 
    av_write_trailer(m_pFormatContext);

    //释放编码器和流缓冲区。
    for(int i = 0; i < m_pFormatContext->nb_streams; i++) {
        av_freep(&m_pFormatContext->streams[i]->codec);
        av_freep(&m_pFormatContext->streams[i]);
    }

    if ( !(m_pOutputFormat->flags & AVFMT_NOFILE) )
        url_fclose( &m_pFormatContext->pb );

    //释放文件。
    av_free(m_pFormatContext); 

}