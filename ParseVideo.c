
#include<stdio.h>
#include<libavutil/log.h>
#include<libavformat/avformat.h>
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
//打印视频信息
int printMateInfo(char* file,char* out){
  int ret;
  AVFormatContext *fmt_ctx=NULL;
  av_register_all();
  //打开多媒体文件，根据文件后缀名解析,第三个参数是显式制定文件类型，当文件后缀与文件格式不符
  //或者根本没有后缀时需要填写，
  ret= avformat_open_input(&fmt_ctx,file,NULL,NULL);
  if(ret<0){
     av_log(NULL,AV_LOG_ERROR,"Can't opent file: %s \n",av_err2str(ret));
   goto _fail;
  }
  //输出多媒体文件信息,第二个参数是流的索引值（默认0），第三个参数，0:输入流，1:输出流
  av_dump_format(fmt_ctx,0,file,0);

  int r=getAudioData(fmt_ctx,out);
  if(r<0){
    goto _fail; 
  }
  _fail:
  avformat_close_input(&fmt_ctx);
  return 0;
}
//抽取音频数据
int getAudioData(AVFormatContext *ctx,char* dest){
  //创建一个可写的输出文件,用于存放输出数据	
  FILE* dest_fd=fopen(dest,"wb");
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
      int len=fwrite(audio_pkt.data,1,audio_pkt.size,dest_fd);	
      if(len!=audio_pkt.size){
	  av_log(NULL,AV_LOG_WARNING,"the write data length not equals audio_pkt size \n");
      }
      av_packet_unref(&audio_pkt);
      }
  }
}

int main(int argc,char* args[]){
  showFile();
  char* src=args[1];
  char* dest=args[2];
  av_log(NULL,AV_LOG_INFO,"input arg length=%d \n",argc);
  if(argc<3){
    av_log(NULL,AV_LOG_ERROR,"input agumet is error \n");
  }else{
    av_log(NULL,AV_LOG_DEBUG,"main()------>input src=%s dest=%s \n",src,dest);
    printMateInfo(src,dest);
  }
  return 0;
}



	 
	 

