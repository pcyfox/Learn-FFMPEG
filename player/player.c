
#include<stdio.h>
#include<libavutil/log.h>
#include<libavformat/avformat.h>
static int bStop = 0;
//clang -g -o pvlib  ParseVideo.c  -lavformat -lavutil
//clang -g -o pvlib  ParseVideo.c `pkg-config --libs libavutil libavformat`
//打印当前目录下的文件，类似ls命令
int showFile(){
  //设置输出日志级别
  av_log_set_level(AV_LOG_DEBUG);
  int ret;
  //调用FFMPEG函数删除文件
  int r= avpriv_io_delete("test.db");
  if(r<0){
      av_log(NULL,AV_LOG_ERROR,"delete file flail\n");
  }else{
      av_log(NULL,AV_LOG_DEBUG,"delete file success\n");
  }
  //实现简单的ls命令功能（输出当前文件所在目录下的文件名称）
  AVIODirContext *ioCtx=NULL;
  AVIODirEntry * ioEntry=NULL;
  ret=avio_open_dir(&ioCtx,"./",NULL);
  if(ret<0){
     av_log(NULL,AV_LOG_ERROR,"open dir fail %s \n",av_err2str(ret));
     return -1;
  }
 
  av_log(NULL,AV_LOG_INFO,"----------current dir files info--------------\n");
  while(1){
   ret= avio_read_dir(ioCtx,&ioEntry);
   if(ret<0){
    av_log(NULL,AV_LOG_ERROR,"read dir fail %s /n",av_err2str(ret));
    goto _fail; 
   }
   if(!ioEntry){
    return -1;
   } 
   av_log(NULL,AV_LOG_INFO,"size:%12"PRId64" name:%s\n",ioEntry->size,ioEntry->name);
   avio_free_directory_entry(&ioEntry);
  }
  _fail:
    avio_close_dir(&ioCtx);
  return 0;
}

AVFormatContext* registerAndDump(char* url){
  if(!url){
     av_log(NULL,AV_LOG_ERROR,"fuck, url is NULL!");
     return NULL; 
  }

  AVFormatContext *fmt_ctx=NULL;
  av_register_all();
  //打开多媒体文件，根据文件后缀名解析,第三个参数是显式制定文件类型，当文件后缀与文件格式不符
  //或者根本没有后缀时需要填写，
  int ret= avformat_open_input(&fmt_ctx,url,NULL,NULL);
  if(ret<0){
     av_log(NULL,AV_LOG_ERROR,"can't open source: %s msg:%s \n",url,av_err2str(ret));
  }
  //输出多媒体文件信息,第二个参数是流的索引值（默认0），第三个参数，0:输入流，1:输出流
  av_dump_format(fmt_ctx,0,url,0);
  return fmt_ctx;
}


//抽取音频数据
int getAudioData(AVFormatContext *ctx,char* dest){
  //创建一个可写的输出文件,用于存放输出数据	
  FILE* dest_fd=fopen(dest,"a*");
  if(!dest_fd){
    av_log(NULL,AV_LOG_ERROR,"open out file %s fail! \n",dest); 
    return -1;
  }

  //找到音频流,第二参数流类型（FFMPEG宏）第三个流的索引号，未知，填-1，第四个，相关流的索引号，
  //如音频流对应的视频流索引号，也未知，第五个制定解解码器，最后一个是标志符解
  int index=av_find_best_stream(ctx,AVMEDIA_TYPE_AUDIO,-1,-1,NULL,0);

  if(index<0){
     av_log(NULL,AV_LOG_ERROR,"find audio stream fail!");
     return -1;
  }
  //读取数据包（因历史原因，函数未改名字）
  AVPacket audio_pkt;
  av_init_packet(&audio_pkt);
  while(av_read_frame(ctx,&audio_pkt)>=0){
      if(audio_pkt.stream_index==index){
      //TODO 每个音频包前加入头信息
      int len=fwrite(audio_pkt.data,1,audio_pkt.size,dest_fd);	

      if(len!=audio_pkt.size){//判断写入文件是否成功
	  av_log(NULL,AV_LOG_WARNING,"the write data length not equals audio_pkt size \n");
      }
         av_packet_unref(&audio_pkt);
      }
  }
}


static unsigned rtsp2file(AVFormatContext *i_fmt_ctx,char *url,char* file)
{

    AVStream *i_video_stream=NULL;

    AVFormatContext *o_fmt_ctx=NULL;
    AVStream *o_video_stream=NULL;

    /* find stream info  */
    if (avformat_find_stream_info(i_fmt_ctx, NULL)<0)
    {
        fprintf(stderr, "could not find stream info\n");
        return -1;
    }

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

        printf("w=%d h=%d pix_fmt=%d", c->width, c->height, c->pix_fmt);
        fprintf(stderr, "--------->time_base.num = %d time_base.den = %d\n", c->time_base.num, c->time_base.den);
/*
        c->bit_rate = i_video_stream->codec->bit_rate;
        c->codec_id = i_video_stream->codec->codec_id;
        c->codec_type = i_video_stream->codec->codec_type;
        c->time_base.num = i_video_stream->time_base.num;
        c->time_base.den = i_video_stream->time_base.den;

        c->width = i_video_stream->codec->width;
        c->height = i_video_stream->codec->height;
        c->pix_fmt = i_video_stream->codec->pix_fmt;


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



int saveH264Data(AVFormatContext *ctx,char* storePath){
  //创建一个可写的输出文件,用于存放输出数据	
  FILE* store_file=fopen(storePath,"a+");
  if(!store_file){
    av_log(NULL,AV_LOG_ERROR,"open out file %s fail! \n",storePath); 
    return -1;
  }

  //找到音频流,第二参数流类型（FFMPEG宏）第三个流的索引号，未知，填-1，第四个，相关流的索引号，
  //如音频流对应的视频流索引号，也未知，第五个制定解解码器，最后一个是标志符解
  int index=av_find_best_stream(ctx,AVMEDIA_TYPE_VIDEO,-1,-1,NULL,0);

  if(index<0){
     av_log(NULL,AV_LOG_ERROR,"find video stream fail!");
     return -1;
  }
  //读取数据包（因历史原因，函数未改名字）
  AVPacket pkt;
  av_init_packet(&pkt);
  while(av_read_frame(ctx,&pkt)>=0){
      if(pkt.stream_index==index){
      int len=fwrite(pkt.data,1,pkt.size,store_file);	
      if(len!=pkt.size){//判断写入文件是否成功
	  av_log(NULL,AV_LOG_WARNING,"the write data length not equals audio_pkt size \n");
      }
         av_packet_unref(&pkt);
      }
  }

}


int main(int argc,char* args[]){
  char* url=args[1];
  char* dest=args[2];
  for(int i=1;i<argc;i++){
    av_log(NULL,AV_LOG_INFO,"input arg i=%d,arg=%s \n",i,args[i]);
  }
  AVFormatContext *ctx=registerAndDump(url);
  if(ctx){
      rtsp2file(ctx,url,dest);
  }else{
    av_log(NULL,AV_LOG_ERROR,"get input AVFormatContext ERROR!");
  }

  return 0;
}



	 
	 

