

#ifdef __cplusplus 
extern "C" { 
#endif

#include <libavcodec/avcodec.h> 
#include <libavformat/avformat.h> 
#include <libavutil/avutil.h> 
#include <stdlib.h> 
#include <stdio.h> 
#include <string.h> 
#include <math.h>

#ifdef __cplusplus 
} 
#endif



static int bStop = 0;

static unsigned rtsp2file(char *url,char* file)
{
    av_register_all();

    AVFormatContext *i_fmt_ctx=NULL;
    AVStream *i_video_stream=NULL;

    AVFormatContext *o_fmt_ctx=NULL;
    AVStream *o_video_stream=NULL;
    /* should set to NULL so that avformat_open_input() allocate a new one */
    if (avformat_open_input(&i_fmt_ctx, url, NULL, NULL)!=0)
    {
        fprintf(stderr, "could not open input file\n");
        return -1;
    }
    /* find stream info  */
    if (avformat_find_stream_info(i_fmt_ctx, NULL)<0)
    {
    
        fprintf(stderr, "could not find stream info\n");
        return -1;
    }

    av_dump_format(i_fmt_ctx, 0, url, 0);

    /* find first video stream */
    int index=-1;
    for (unsigned i=0; i<i_fmt_ctx->nb_streams; i++)
    {
        if (i_fmt_ctx->streams[i]->codec->codec_type == AVMEDIA_TYPE_VIDEO)
        {
            i_video_stream = i_fmt_ctx->streams[i];
	    index=i;
	    printf("select input stream index=%d\n",i);
            break;
        }
    }

    if (i_video_stream == NULL)
    {
        fprintf(stderr, "didn't find any video stream\n");
        return -1;
    }
    /* create output context by file name*/
    avformat_alloc_output_context2(&o_fmt_ctx, NULL, NULL, file);

    /*
    * since all input files are supposed to be identical (framerate, dimension, color format, ...)
    * we can safely set output codec values from first input file
    */
    o_video_stream = avformat_new_stream(o_fmt_ctx, NULL);
    {
        AVCodecContext *c= o_video_stream->codec;
        /* copy input stream codec info parm to out AVCodecContext*/
	avcodec_parameters_from_context(c,i_video_stream->codecpar );
	av_codec_set_pkt_timebase(c, i_video_stream->time_base);
        /*must set dimensions by self*/
        c->width = i_video_stream->codec->width;
        c->height = i_video_stream->codec->height;

/*
        c->bit_rate = i_video_stream->codec->bit_rate;
        c->codec_id = i_video_stream->codec->codec_id;
        c->codec_type = i_video_stream->codec->codec_type;
        c->time_base.num = i_video_stream->time_base.num;
        c->time_base.den = i_video_stream->time_base.den;

        fprintf(stderr, "--------->time_base.num = %d time_base.den = %d\n", c->time_base.num, c->time_base.den);
        c->width = i_video_stream->codec->width;
        c->height = i_video_stream->codec->height;
        c->pix_fmt = i_video_stream->codec->pix_fmt;

        printf("w=%d h=%d pix_fmt=%d", c->width, c->height, c->pix_fmt);

        c->flags = i_video_stream->codec->flags;
        c->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;
        c->me_range = i_video_stream->codec->me_range;
        c->max_qdiff = i_video_stream->codec->max_qdiff;
        c->qmin = i_video_stream->codec->qmin;
        c->qmax = i_video_stream->codec->qmax;
        c->qcompress = i_video_stream->codec->qcompress;
	*/
    }
    /*open file  pb:AVIOContext*/
    if(avio_open(&o_fmt_ctx->pb, file, AVIO_FLAG_WRITE)<0)
    {
	 av_log(NULL,AV_LOG_ERROR,"open file error");
	 goto __FAIL;
    
    }

    /*write file header*/
    if(avformat_write_header(o_fmt_ctx, NULL)<0)
    {
	 av_log(NULL,AV_LOG_ERROR,"write file header error");
	 goto __FAIL;
    
    }

    int last_pts = 0;
    int last_dts = 0;
    int64_t pts, dts;

    while (!bStop)
    {
        AVPacket i_pkt;
        av_init_packet(&i_pkt);
        i_pkt.size = 0;
        i_pkt.data = NULL;
        /*read packet*/
        if (av_read_frame(i_fmt_ctx, &i_pkt) <0 )
            break;
        /*
        * pts and dts should increase monotonically
        * pts should be >= dts
	* AV_PKT_FLAG_KEY     0x0001 ///< The packet contains a keyframe
        */
        i_pkt.flags |= AV_PKT_FLAG_KEY;//set frame type
        pts = i_pkt.pts;
        i_pkt.pts += last_pts;
        dts = i_pkt.dts;
        i_pkt.dts += last_dts;
        i_pkt.stream_index = index;

        static int num = 1;
        printf("---->frame count=%d  type=%d\n", num++,i_pkt.flags);

        /*write packet and auto free packet*/
        av_interleaved_write_frame(o_fmt_ctx, &i_pkt);
        //sleep(10);
    }
    last_dts += dts;
    last_pts += pts;

    avformat_close_input(&i_fmt_ctx);

    /*write file trailer*/
    av_write_trailer(o_fmt_ctx);

__FAIL:
    avcodec_close(o_fmt_ctx->streams[index]->codec);
    av_freep(&o_fmt_ctx->streams[index]->codec);
    av_freep(&o_fmt_ctx->streams[index]);
    avio_close(o_fmt_ctx->pb);
    av_free(o_fmt_ctx);

    return 0;
}

int main(int argc, char **argv)
{
    printf("url=%s,file=%s\n",argv[1],argv[2]);
    rtsp2(argv[1],argv[2]);
    printf("按任意键停止录像\n");
    getchar();
    bStop = 0;
    printf("按任意键退出\n");
    getchar();
    return 0;
}
