#include <jni.h>
#include <string>
#include <android/native_window_jni.h>
#include <unistd.h>
#include <android/log.h>

#ifdef __cplusplus
extern "C" {
#endif

#include "include/libavutil/dict.h"
#include "include/libavformat/avformat.h"
#include <libswscale/swscale.h>
#include <libavutil/imgutils.h>
#include <libavutil/frame.h>

void Java_com_example_ffmpegva_WonderfulPlayer_playVideo(JNIEnv *env,jobject thiz,jstring videoPath,jstring yuvPath,jstring yuvVideoPath,jstring yuvPgmPath,jobject surface){

    __android_log_print(ANDROID_LOG_ERROR, "FfmpegVA", "native method run to start");

    //底层的渲染都是通过ANativeWindow来实现的，这里传入surface
    ANativeWindow *nativeWindow = ANativeWindow_fromSurface(env,surface);
    //拿到播放路径，由于这是cpp代码，所以需要通过jni提供的方法将java的String转换成c的字符数组即字符型指针
    const char *path = env->GetStringUTFChars(videoPath,0);
    const char *pathYuv = env->GetStringUTFChars(yuvPath,0);
    const char *pathYuvVideo = env->GetStringUTFChars(yuvVideoPath,0);
    const char *pathYuvPgm= env->GetStringUTFChars(yuvPgmPath,0);
    //初始化网络模块，因为ffmpeg支持本地和网络数据播放
    avformat_network_init();
    //创建总上下文
    AVFormatContext *formatContext = avformat_alloc_context();
    //创建字典，相当于一个hashMap
    AVDictionary *dictionary = NULL;
    //设置超时时长,相当于put
    av_dict_set(&dictionary,"timeout","3000000",0);
    //打开视频文件,获取流信息(h264)
    int result = avformat_open_input(&formatContext,path,0,&dictionary);
    //非零表示失败
    if (result){
        __android_log_print(ANDROID_LOG_ERROR, "FfmpegVA", "打开视频文件失败！");
        __android_log_print(ANDROID_LOG_ERROR, "FfmpegVA", "调用的函数: avformat_open_input");
        __android_log_print(ANDROID_LOG_ERROR, "FfmpegVA", "错误码(十进制): %d",result);
        __android_log_print(ANDROID_LOG_ERROR, "FfmpegVA", "错误码(十六进制): %08x",result);
        __android_log_print(ANDROID_LOG_ERROR, "FfmpegVA", "文件路径: %s",path);
        return;
    }
    __android_log_print(ANDROID_LOG_ERROR, "FfmpegVA", "打开视频文件成功！");
    //解析流，这里的流包括视频流、音频流等
    result = avformat_find_stream_info(formatContext,NULL);
    if (result == 0){
        __android_log_print(ANDROID_LOG_ERROR, "FfmpegVA", "流解析成功！");
    } else{
        __android_log_print(ANDROID_LOG_ERROR, "FfmpegVA", "流解析失败！");
        __android_log_print(ANDROID_LOG_ERROR, "FfmpegVA", "错误码(十进制): %d",result);
        __android_log_print(ANDROID_LOG_ERROR, "FfmpegVA", "错误码(十六进制): %08x",result);
    }
    //视频流索引
    int video_stream_index = -1;
    //遍历流，取出流的解密器参数->取出参数类型->如果是视频流则记录位置
    for (int i = 0; i < formatContext->nb_streams; ++i) {
        if (formatContext->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO){
            video_stream_index = i;
            break;
        }
    }
    __android_log_print(ANDROID_LOG_ERROR, "FfmpegVA", "获取到视频流位置：%d",video_stream_index);
    //取出视频流解密参数，这个参数封装了视频的大小、时长等信息
    AVCodecParameters *parameters = formatContext->streams[video_stream_index]->codecpar;
    __android_log_print(ANDROID_LOG_ERROR, "FfmpegVA", "得到解码器参数，宽：%d",parameters->width);
    __android_log_print(ANDROID_LOG_ERROR, "FfmpegVA", "得到解码器参数，id：%d",parameters->codec_id);
    //创建出解码器
    AVCodec *codec = avcodec_find_decoder(parameters->codec_id);
    //创建出解码器上下文
    AVCodecContext *avCodecContext = avcodec_alloc_context3(codec);
    //将解码器参数复制到解码器上下文
    result = avcodec_parameters_to_context(avCodecContext,parameters);
    if(result == 0){
        __android_log_print(ANDROID_LOG_ERROR, "FfmpegVA", "将解码器参数复制到解码器上下文成功！");
    } else{
        __android_log_print(ANDROID_LOG_ERROR, "FfmpegVA", "将解码器参数复制到解码器上下文失败！");
        __android_log_print(ANDROID_LOG_ERROR, "FfmpegVA", "错误码(十进制): %d",result);
        __android_log_print(ANDROID_LOG_ERROR, "FfmpegVA", "错误码(十六进制): %08x",result);
    }
    //开启解码器
    result = avcodec_open2(avCodecContext,codec,NULL);
    if(result == 0){
        __android_log_print(ANDROID_LOG_ERROR, "FfmpegVA", "打开解码器成功！");
    } else{
        __android_log_print(ANDROID_LOG_ERROR, "FfmpegVA", "打开解码器失败！");
        __android_log_print(ANDROID_LOG_ERROR, "FfmpegVA", "错误码(十进制): %d",result);
        __android_log_print(ANDROID_LOG_ERROR, "FfmpegVA", "错误码(十六进制): %08x",result);
    }
    /**将编码数据(h264)转换成原始数据(yuv)**/
    //创建数据包，这里的数据包仍然是压缩的,可以理解成压缩的一帧数据
    AVPacket *packet = av_packet_alloc();
    //获取转码上下文，将yuv数据转码成rgba数据格式才能绘制到surface中
    //p1:原视频宽，p2原视频高，p3原视频格式，p4输出输出视频宽，p5输出视频高，p6输出视频格式，p7转换方式这里是最快忽视质量，
    SwsContext *swsContext = sws_getContext(avCodecContext->width,avCodecContext->height,avCodecContext->pix_fmt,
                                           avCodecContext->width,avCodecContext->height,AV_PIX_FMT_RGBA,
                                           SWS_BILINEAR,0,0,0);
    //设置window的大小
    ANativeWindow_setBuffersGeometry(nativeWindow,avCodecContext->width,avCodecContext->height,WINDOW_FORMAT_RGBA_8888);
    //创建一个绘制的缓冲区
    ANativeWindow_Buffer windowBuffer;
    //从视频流中读取数据包,并解码成原始数据，再绘制出了

    //打开输出文件,用于输出一帧yuv data
    FILE *f_pcm = fopen(pathYuv,"a");
    uint8_t *p;
    long addr = 0;
    int status = 0;

    //打开输出文件,用于输出yuv数据
    FILE *f_yuv = fopen(pathYuvVideo,"wb");
    //打开输出文件,用于输出pgm数据
    FILE *f_pgm = fopen(pathYuvPgm,"wb");

    uint8_t *buf = NULL;

    //内存拷贝计算变量
    int count = 0;
    while(av_read_frame(formatContext,packet)>=0){    //读取数据，这里实际上是放到一个队列中，>=0说明读到正确的数据
        __android_log_print(ANDROID_LOG_ERROR, "FfmpegVA", "读取到一个数据包");
        //将读到的数据取出来
        result = avcodec_send_packet(avCodecContext,packet);
        if (result == 0){
            __android_log_print(ANDROID_LOG_ERROR, "FfmpegVA", "取数据包成功！");
        }else if(result == AVERROR(EAGAIN)){
            __android_log_print(ANDROID_LOG_ERROR, "FfmpegVA", "取数据包失败！");
            __android_log_print(ANDROID_LOG_ERROR, "FfmpegVA", "错误码：EAGAIN");
        } else if(result == AVERROR_EOF){
            __android_log_print(ANDROID_LOG_ERROR, "FfmpegVA", "取数据包失败！");
            __android_log_print(ANDROID_LOG_ERROR, "FfmpegVA", "错误码：AVERROR_EOF");
        } else if(result == AVERROR(EINVAL)){
            __android_log_print(ANDROID_LOG_ERROR, "FfmpegVA", "取数据包失败！");
            __android_log_print(ANDROID_LOG_ERROR, "FfmpegVA", "错误码：EINVAL");
        }else if(result == AVERROR(ENOMEM)){
            __android_log_print(ANDROID_LOG_ERROR, "FfmpegVA", "取数据包失败！");
            __android_log_print(ANDROID_LOG_ERROR, "FfmpegVA", "错误码：ENOMEM");
        } else{
            __android_log_print(ANDROID_LOG_ERROR, "FfmpegVA", "取数据包失败！");
            __android_log_print(ANDROID_LOG_ERROR, "FfmpegVA", "非正常错误码(十进制): %d",result);
            __android_log_print(ANDROID_LOG_ERROR, "FfmpegVA", "非正常错误码(十六进制): %08x",result);
        }
        //创建原始数据容器,这就是yuv数据
        AVFrame *frame = av_frame_alloc();
        //将一个数据包的数据转换成yuv原始数据
        result = avcodec_receive_frame(avCodecContext,frame);
        if (result == AVERROR(EAGAIN)){
            __android_log_print(ANDROID_LOG_ERROR, "FfmpegVA", "数据包转换yuv失败！");
            __android_log_print(ANDROID_LOG_ERROR, "FfmpegVA", "错误码(十进制): %d",result);
            __android_log_print(ANDROID_LOG_ERROR, "FfmpegVA", "错误码(十六进制): %08x",result);
            continue;
        } else if(result<0){    //读到末尾了
            __android_log_print(ANDROID_LOG_ERROR, "FfmpegVA", "读到末尾了");
            __android_log_print(ANDROID_LOG_ERROR, "FfmpegVA", "错误码(十进制): %d",result);
            __android_log_print(ANDROID_LOG_ERROR, "FfmpegVA", "错误码(十六进制): %08x",result);
            break;
        }
        //实测data的长度为8，但是只有前三项有值，与linesize吻合，大小分别是576，288，288，猜测是yuv的三个分量
        //而linesize到底代表什么含义呢，根据官方注释：图片每行的大小（以字节为单位），我的理解是由于yuv的每个分量都只有一个数组有值，
        //也就是说一个数组包含了整帧的数据，那么如何区分一行的开始和结束就需要知道每一行的大小
        __android_log_print(ANDROID_LOG_ERROR, "FfmpegVA", "数据包转换yuv成功！");
        __android_log_print(ANDROID_LOG_ERROR, "FfmpegVA", "数据包转换yuv成功，帧宽: %d",frame->width);
        __android_log_print(ANDROID_LOG_ERROR, "FfmpegVA", "数据包转换yuv成功，帧高: %d",frame->height);
        __android_log_print(ANDROID_LOG_ERROR, "FfmpegVA", "数据包转换yuv成功，行数: %d", sizeof(frame->linesize)/sizeof(frame->linesize[0]));
        __android_log_print(ANDROID_LOG_ERROR, "FfmpegVA", "数据包转换yuv成功，0行大小: %d", frame->linesize[0]);
        __android_log_print(ANDROID_LOG_ERROR, "FfmpegVA", "数据包转换yuv成功，1行大小: %d", frame->linesize[1]);
        __android_log_print(ANDROID_LOG_ERROR, "FfmpegVA", "数据包转换yuv成功，2行大小: %d", frame->linesize[2]);
        __android_log_print(ANDROID_LOG_ERROR, "FfmpegVA", "数据包转换yuv成功，3行大小: %d", frame->linesize[3]);
        __android_log_print(ANDROID_LOG_ERROR, "FfmpegVA", "数据包转换yuv成功，4行大小: %d", frame->linesize[4]);
        __android_log_print(ANDROID_LOG_ERROR, "FfmpegVA", "数据包转换yuv成功，5行大小: %d", frame->linesize[5]);
        __android_log_print(ANDROID_LOG_ERROR, "FfmpegVA", "数据包转换yuv成功，6行大小: %d", frame->linesize[6]);
        __android_log_print(ANDROID_LOG_ERROR, "FfmpegVA", "数据包转换yuv成功，7行大小: %d", frame->linesize[7]);
        __android_log_print(ANDROID_LOG_ERROR, "FfmpegVA", "数据包转换yuv成功，data长度: %d", sizeof(frame->data)/ sizeof(frame->data[0]));

        //将一帧数据输出,用记事本打开有些问题，用其他方式打开
        //这里更linesize行大小没有什么关系了，具体分析看后面的色彩偏移
        status++;
        if (status >=20){
            status = 20;
        }
        if(status == 15){
            int count = 0;
            int length = 0;
            try{
                __android_log_print(ANDROID_LOG_ERROR, "FfmpegVA-count", "=================指针地址0============: %ld",(long)frame->data[0]);
                __android_log_print(ANDROID_LOG_ERROR, "FfmpegVA-count", "=================指针地址1============: %ld",(long)frame->data[1]);
                __android_log_print(ANDROID_LOG_ERROR, "FfmpegVA-count", "=================指针地址2============: %ld",(long)frame->data[2]);
                for (int i = 0; i < sizeof(frame->data)/sizeof(frame->data[0]); i++) {
                    if(i==0){
                        length = frame->width*frame->height;
                    } else if(i==1){
                        length = frame->width*frame->height/4;
                    } else if(i==2){
                        length = frame->width*frame->height/4;
                    } else{
                        length = 0;
                    }
                    p = frame->data[i];
                    addr = (long)p;
                    for (int j = 0; j < length; j++) {
                        fprintf(f_pcm,"%d ",*p);
                        p++;
                    }
                    count = (int)((long)p-addr);
                    fprintf(f_pcm,"\n\r=================%d=================\n\r",count);
                    __android_log_print(ANDROID_LOG_ERROR, "FfmpegVA-count", "==============总行数===============: %d",sizeof(frame->data)/sizeof(frame->data[0]));
                    __android_log_print(ANDROID_LOG_ERROR, "FfmpegVA-count", "==============行大小===============: %d",frame->linesize[i]);
                    __android_log_print(ANDROID_LOG_ERROR, "FfmpegVA-count", "=================输出个数============: %d",count);
                }
            }catch (...){
                fprintf(f_pcm,"\n\r========error=======error==========error=========");
            }

            //转成pgm保存到文件中
            fprintf(f_pgm,"P5\n%d %d\n%d\n",frame->width,frame->height,255);
            for (int i = 0; i < frame->height; ++i) {
                fwrite(frame->data[0]+i*frame->linesize[0],1,frame->width,f_pgm);
            }

            fclose(f_pcm);
            fclose(f_pgm);
        }

        //将yuv数据保存到文件中,这种方式保存打开无法观看
//        fwrite(frame->data[0],1,frame->width*frame->height,f_yuv);
//        fwrite(frame->data[1],1,frame->width*frame->height/4,f_yuv);
//        fwrite(frame->data[2],1,frame->width*frame->height/4,f_yuv);
        //下面做了偏移之后得到能够播放的数据，但是图像一直在闪并且不清晰，原因未知，找到原因了，宽高都要/2，视频ok
        for (int i = 0; i < frame->height; ++i) {
            fwrite(frame->data[0] + i*frame->linesize[0],1,frame->width,f_yuv);
        }
        for (int i = 0; i < frame->height/2; ++i) {
            fwrite(frame->data[1] + i*frame->linesize[1],1,frame->width/2,f_yuv);
        }
        for (int i = 0; i < frame->height/2; ++i) {
            fwrite(frame->data[2] + i*frame->linesize[2],1,frame->width/2,f_yuv);
        }

//        //这种方式ok
//        if(buf == NULL){
//            buf = new uint8_t[frame->width*frame->height*3/2];
//        }
//        memset(buf,0,frame->width*frame->height*3/2);
//        int offset = 0;
//        for (int i = 0; i < frame->height; ++i) {
//            memcpy(buf+offset,frame->data[0]+i*frame->linesize[0],frame->width);
//            offset += frame->width;
//        }
//        for (int i = 0; i < frame->height/2; ++i) {
//            memcpy(buf+offset,frame->data[1]+i*frame->linesize[1],frame->width/2);
//            offset += frame->width/2;
//        }
//        for (int i = 0; i < frame->height/2; ++i) {
//            memcpy(buf+offset,frame->data[2]+i*frame->linesize[2],frame->width/2);
//            offset += frame->width/2;
//        }
//        fwrite(buf,1,frame->width*frame->height*3/2,f_yuv);


//        //修改yuv数据，如果后两个指针指向的数组代表uv则图像为灰度图,实测这种方法不行
//        uint8_t a[0] = {};
//        frame->data[1] = a;
//        frame->data[2] = a;
//        frame->linesize[1] = 0;
//        frame->linesize[2] = 0;

//向蓝色偏移，实测有点奇怪,按我的理解应该是需要乘以232，因为有232行，但实际上232已经偏移到红色区域了
//正确的理解：yuv420(4:2:0)编码中，每4个y对应一组uv进行采样,如果每个分量都用一个字节表示，那么
//y=(frame->width*frame->height) 字节
//u=v=(frame->width*frame->height/4) 字节
//frame=(frame->width*frame->height/4*6) 字节
//而不同的存储方式又形成了不同的格式，如y u v三个分量分别存储的统称为yuv420p（包括yv12、yu12）
//y和uv分别存储的统称为yuv420sp（包括nv12、nv21），但是请注意他们都属于yuv格式范畴
//todo link https://mp.weixin.qq.com/s?__biz=MzAwODM5OTM2Ng==&idx=1&mid=2454862932&sn=245212b661fe8565ad002c93642b1127
//linesize代表的是每一行的字节数，在测试中他比width多了四个字节，这是由于cpu内存对齐导致的，也就是说解码出来的data数据中每一个linesize长度的数据
//代表了这一帧图像的一行，因为内存对齐的原因，linesize可能比width大，多余的部分应是没有用的，这样data的总大小就等于linesize[0]*232+linesize[1]*232/2+linesize[2]*232/2
//在这个列子中图像为568*232，每行y的大小为linesize[0]=576,每行u的大小为linesize[1]=288,每行v的大小为linesize[2]=288,
//根据yuv420采样图（具体看网上的采样图，很重要），u/v的宽为width/2,uv的高为height/2,这样是符合y:u:v=4:1:1的
//经过修改的代码得到了正确的蓝色偏移，除以2是因为高只有图像高的二分之一，上面分析过了
        for (int i = 0; i < sizeof(frame->data)/sizeof(frame->data[0]); i++) {
            p = frame->data[i];
            for (int j = 0; j < frame->linesize[i]*232/2; j++) {
                if(i == 1){
                    *p += 20;
                }
                p++;
            }
        }

        //rgba数据接收容器,可以理解为rgba四行n列的数据
        //实测发现1，2，3全为空，并且将大小设置为1也能播放
        uint8_t *dis_data[4];
        //每行首地址（注意：这里应该有错误，我的理解是每一行的size大小，以字节为单位）
        //实测发现1，2，3大小全为0，并且将大小设置为1也能播放
        int first_p[4];
        //根据视频宽高及编码格式(rgba)确定接收容器的大小及首地址,1:左对齐
        av_image_alloc(dis_data,first_p,avCodecContext->width,avCodecContext->height,AV_PIX_FMT_RGBA,1);
        //开始将yuv转换成rgba
        //p1转码上下文，p2输入数据源，p3输入数据行首地址，p4首行地址偏移量，p5这一帧行数，p6输出数据，输出数据每行首地址
        result = sws_scale(swsContext,frame->data,frame->linesize,0,frame->height,dis_data,first_p);
        if (result >= 0){
            __android_log_print(ANDROID_LOG_ERROR, "FfmpegVA", "yuv转rgba成功！");
            __android_log_print(ANDROID_LOG_ERROR, "FfmpegVA", "yuv转rgba成功,行大小-r: %d",first_p[0]);
            __android_log_print(ANDROID_LOG_ERROR, "FfmpegVA", "yuv转rgba成功,行大小-g: %d",first_p[1]);
            __android_log_print(ANDROID_LOG_ERROR, "FfmpegVA", "yuv转rgba成功,行大小-b: %d",first_p[2]);
            __android_log_print(ANDROID_LOG_ERROR, "FfmpegVA", "yuv转rgba成功,行大小-a: %d",first_p[3]);

//            try{
//                //这里调用导致崩溃，说明确实长度为0
//                uint8_t x = (uint8_t)dis_data[1][0];
//            }catch (...){
//                //c好像无法捕获数组越界
//                __android_log_print(ANDROID_LOG_ERROR, "FfmpegVA", "数组越界！");
//            }


        } else{
            __android_log_print(ANDROID_LOG_ERROR, "FfmpegVA", "yuv转rgba失败！");
            __android_log_print(ANDROID_LOG_ERROR, "FfmpegVA", "错误码(十进制): %d",result);
            __android_log_print(ANDROID_LOG_ERROR, "FfmpegVA", "错误码(十六进制): %08x",result);
        }
        //渲染前锁住window
        ANativeWindow_lock(nativeWindow,&windowBuffer,0);
        //调用内存拷贝，将rgba数据一行一行拷贝到缓冲区完成渲染
        uint8_t *firstWindow = static_cast<uint8_t *>(windowBuffer.bits);
        uint8_t *data = dis_data[0];
        int firstP = first_p[0];
        //一行的字节数
        int desStride = windowBuffer.stride*4;
        count = 0;
        //实测内存拷贝进行了232次，和yuv一帧的高度吻合
        //帧高为568，而firstP大小为2272，2272/4=568
        for (int i = 0; i < windowBuffer.height; i++) {
            memcpy(firstWindow+i*desStride,data+i*firstP,desStride);
//            __android_log_print(ANDROID_LOG_ERROR, "FfmpegVA", "内存拷贝: %d",count);
            count++;
        }
        //渲染后解锁window
        ANativeWindow_unlockAndPost(nativeWindow);
        usleep(1000*16);
        av_frame_free(&frame);
    }

    fclose(f_yuv);
    //根据方法名，这里应该是释放资源的意思
    env->ReleaseStringUTFChars(videoPath,path);
    env->ReleaseStringUTFChars(yuvPath,pathYuv);
    env->ReleaseStringUTFChars(yuvVideoPath,pathYuvVideo);
    env->ReleaseStringUTFChars(yuvPgmPath,pathYuvPgm);
    __android_log_print(ANDROID_LOG_ERROR, "FfmpegVA", "native method run to end");
}

#ifdef __cplusplus
}
#endif



//extern "C"{
//#include <libavcodec/avcodec.h>
//#include <libavformat/avformat.h>
//}
//extern "C" JNIEXPORT void JNICALL
//Java_com_example_ffmpegva_WonderfulPlayer_playVideo(JNIEnv *env,jobject thiz,jstring videoPath,jobject surface){
//    //拿到播放路径，由于这是cpp代码，所以需要通过jni提供的方法将java的String转换成c的字符数组即字符型指针
//    const char *path = env->GetStringUTFChars(videoPath,0);
//    //初始化网络模块，因为ffmpeg支持本地和网络数据播放
//    avformat_network_init();
//    //创建总上下文
//    AVFormatContext *formatContext = avformat_alloc_context();
//    //创建字典
//    AVDictionary *dictionary = NULL;
//    av_dict_set(&dictionary,"timeout","3000000",0);
////    avformat_open_input()
//    //根据方法名，这里应该是释放资源的意思
//    env->ReleaseStringUTFChars(videoPath,path);
//}
