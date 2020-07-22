#include<stdio.h>
#include<libavutil/log.h>
#include<libavformat/avformat.h>
//clang -g -o pvlib  ParseVideo.c  -lavformat -lavutil
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
int printMateInfo(char* file){
  int ret;
  AVFormatContext *fmt_ctx=NULL;
  av_register_all();
  ret= avformat_open_input(&fmt_ctx,file,NULL,NULL);
  if(ret<0){
     av_log(NULL,AV_LOG_ERROR,"Can't opent file: %s\n",av_err2str(ret));
   goto _fail;
  }

  av_dump_format(fmt_ctx,0,file,0);
  _fail:
  avformat_close_input(&fmt_ctx);
  return 0;
}

int main(int argc,char* args[]){
  showFile();
  char* src=args[1];
  char* des=args[2];
  if(argc<3){
    av_log(NULL,AV_LOG_ERROR,"input agumet is error");
  }else{
    av_log(NULL,AV_LOG_DEBUG,"main()------>input arg:%s",args[1]);
    printMateInfoprintMateInfo(src);
  }
  return 0;
}




