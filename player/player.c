
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

AVFormatContext* registerAndDump(char* source){
  if(!source){
     return NULL; 
  }
  AVFormatContext *fmt_ctx=NULL;
  av_register_all();
  //打开多媒体文件，根据文件后缀名解析,第三个参数是显式制定文件类型，当文件后缀与文件格式不符
  //或者根本没有后缀时需要填写，
  int ret= avformat_open_input(&fmt_ctx,source,NULL,NULL);
  if(ret<0){
     av_log(NULL,AV_LOG_ERROR,"can't open source: %s msg:%s \n",source,av_err2str(ret));
   goto _fail;
  }
  //输出多媒体文件信息,第二个参数是流的索引值（默认0），第三个参数，0:输入流，1:输出流
  av_dump_format(fmt_ctx,0,source,0);
  _fail:
  avformat_close_input(&fmt_ctx);
  return fmt_ctx;
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
      //TODO 每个音频包前加入头信息
      int len=fwrite(audio_pkt.data,1,audio_pkt.size,dest_fd);	

      if(len!=audio_pkt.size){//判断写入文件是否成功
	  av_log(NULL,AV_LOG_WARNING,"the write data length not equals audio_pkt size \n");
      }
         av_packet_unref(&audio_pkt);
      }
  }
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
  showFile();
  char* src=args[1];
  char* dest=args[2];
  for(int i=1;i<argc;i++){
    av_log(NULL,AV_LOG_INFO,"input arg i=%d,arg=%s \n",i,args[i]);
  }
  AVFormatContext *ctx=registerAndDump(src);
  return 0;
}



	 
	 

