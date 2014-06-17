void DSGrabberCallback::initFFMpeg(){
    const char* filename="G:/test1.mpg";
    avcodec_register_all();
    printf("Encode video file %s\n", filename);

    AVCodecID codec_id=AV_CODEC_ID_MPEG2VIDEO;
    codec = avcodec_find_encoder(codec_id);

    c = avcodec_alloc_context3(codec);

    if (!c) {
        fprintf(stderr, "Could not allocate video codec context\n");
    }

    c->bit_rate = 4000000;
    c->width = 320;
    c->height = 240;

    AVRational test;
    test.den=25;
    test.num=1;
    c->time_base= test;
    c->gop_size = 10;
    //c->max_b_frames=1;
    c->pix_fmt = AV_PIX_FMT_YUV420P;

    if(codec_id == AV_CODEC_ID_H264)
        av_opt_set(c->priv_data, "preset", "slow", 0);

    if (avcodec_open2(c, codec, NULL) < 0) {
        fprintf(stderr, "Could not open codec\n");
    }

    f = fopen(filename, "wb");
    if (!f) {
        fprintf(stderr, "Could not open %s\n", filename);
    }

    picture = alloc_picture(c->pix_fmt, c->width, c->height);


    /*picture->format = c->pix_fmt;
    picture->width  = c->width;
    picture->height = c->height;*/

    av_init_packet(&pkt);

}

void DSGrabberCallback::encodeFrame(unsigned char *frame,ULONG size){

    std::cout<<"called.....";

    pkt.data = NULL;   
    pkt.size = 0; 
    picture->data[0]=frame;
    fflush(stdout);

    picture->pts=counter;


    ret = avcodec_encode_video2(c, &pkt, picture, &got_output);
    if (ret < 0) {
        fprintf(stderr, "Error encoding frame\n");
    }

    if (got_output) {
        printf("Write frame %3d (size=%5d)\n", counter, pkt.size);
        fwrite(pkt.data, 1, pkt.size, f);
        av_free_packet(&pkt);
    }
}

STDMETHODIMP DSGrabberCallback::SampleCB(double time, IMediaSample* sample)
{
    BYTE* data = NULL;
    ULONG length = 0;
    m_bytes=NULL;

    counter=counter+1;

    if(FAILED(sample->GetPointer(&data)))
    {
        return E_FAIL;
    }

    length = sample->GetActualDataLength();
    if(length == 0)
    {
        return S_OK;
    }

    if(!m_bytes || m_bytesLength < length)
    {
        if(m_bytes)
        {
            delete[] m_bytes;
        }

        m_bytes = new unsigned char[length];
        m_bytesLength = length;
    }
    if(true)
    {
        for(size_t row = 0 ; row < 480 ; row++)
        {
            memcpy((m_bytes + row * 640 * 2), data + (480 - 1 - row) * 640 * 2,
                640 * 2);
        }
    }
    std::cout<<"hiiiiiiiiiiiiiiiiiiiiiiii";

    // memcpy(m_bytes, data, length);
    // std::cout<<"called............... "<<m_bytes<<"\n";
    if(counter<500){
        encodeFrame(m_bytes,length);
    }else{
        fwrite(endcode, 1, sizeof(endcode), f);
        fclose(f);
        avcodec_close(c);
        av_free(c);
        av_freep(&picture->data[0]);
        avcodec_free_frame(&picture);
        printf("\n");
        exit(1);
    }


    //rtp.sendRTP(data,length);

    //sample->Release();
    //printf("Sample received: %p %u\n", data, length);


    return S_OK;
}