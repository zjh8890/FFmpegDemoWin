#include "ffmpegDemo.h"
#include <stdio.h>
#include <stdlib.h>   
#include <string.h>  
#ifdef _MSC_VER
#include <Windows.h>
#include <time.h>
#else
#include <time.h>
#include <sys/time.h> 
#endif

#define threadnum        2
#define OutputFileEnable 0
#define Ittiam_Enable    1
#define Stess_Test       0


#if Ittiam_Enable
#define CodecID AV_CODEC_ID_ITMHEVC
#else
#define CodecID AV_CODEC_ID_HEVC
#endif

static void yv12_save(AVFrame *frame, FILE *f)
{
	int width = frame->width;
	int height = frame->height;
	int lineszie;
	uint8_t *data = NULL;	

	for(int i = 0; i < 3; i++)
	{
		data = frame->data[i];
		lineszie = frame->linesize[i];
		height = i ? (frame->height >> 1) : frame->height;
		width = i ? (frame->width >> 1) : frame->width;

		for(int h = 0; h < height; h++)
		{
			fwrite(data,1,width,f);
			data += lineszie;
		}
	}
}

bool findSpsPos(uint8_t* data, int* length, int filesize)
{
	bool isSpsFound = false;
	uint8_t naltype = 0xff;

	for (int i = 3; i < filesize - 2; i++)
	{
		if (data[i] == 1)
			if ((data[i - 3] | data[i - 2] | data[i - 1]) == 0){
				naltype = (data[i + 1] >> 1) & 0x3f;
				if (isSpsFound && (naltype > 34 || naltype < 32))
				{
					*length = i;
					return true;
				}

				if (naltype == 33)
				{
					isSpsFound = true;
				}
			}
	}
	return false;
}

bool extractFrameData(uint8_t* data, int* nalstart, int *nallen, int *naltype, int uselen, int filesize)
{
	bool FrameStartFound = false;
	*naltype = 0xff;
	*nalstart = -1;

	for (int i = uselen + 3; i < filesize - 2; i++)
	{
		if (data[i] == 1)
			if ((data[i - 3] | data[i - 2] | data[i - 1]) == 0){
				*naltype = (data[i + 1] >> 1) & 0x3f;
				if (*naltype <= 63)
				{
					if (FrameStartFound)
					{
						*nallen = i - (*nalstart) - 3;
						return FrameStartFound;
					}
					else
					{
						FrameStartFound = true;
						*nalstart = i - 3;
						*nallen = filesize - (*nalstart);
					}
				}
			}
	}
	return FrameStartFound;
}

int64_t GetTime_ms(void)
{
#ifdef _MSC_VER
	LARGE_INTEGER m_nFreq;
	LARGE_INTEGER m_nTime;

	QueryPerformanceFrequency(&m_nFreq);
	QueryPerformanceCounter(&m_nTime);

	return m_nTime.QuadPart * 1000 / m_nFreq.QuadPart;

#elif defined(__GNUC__)
	struct timeval tv_date;
	gettimeofday(&tv_date, NULL);
	return (INT64)tv_date.tv_sec * 1000 + (INT64)tv_date.tv_usec;
#endif
}

int main(int argc, char *argv[])
 {
	AVFormatContext *pFormatCtx = NULL;  
    AVCodecContext *pCodecCtx;  
    AVCodec *pCodec;  
    AVFrame *pFrame;  
    AVPacket *packet;  
    int frameFinished;
	int Decodedlen;
#if OutputFileEnable
	FILE *fpOutFile = NULL;	
#endif
	FILE *fpInFile = NULL;
	int framenum = 0;

    if(argc < 2)  
    {  
        fprintf(stderr, "Usage: ffmpegDemoWin.exe -i x.265 -o x.yuv/n");  
        exit(1);  
    }    

	for(int iInputParam = 1; iInputParam<argc; iInputParam++)
	{
		if (strcmp("-i", argv[iInputParam]) == 0 && (iInputParam + 1) < argc)
		{
			fpInFile = fopen(argv[++iInputParam], "rb");
			if (NULL == fpInFile)
			{
				fprintf(stderr, "Unable to open a h265 stream file %s \n", argv[iInputParam]);
				goto exitmain;
			}
			printf("decoding file: %s...\n", argv[iInputParam]);
		}
#if OutputFileEnable
		else if (strcmp("-o", argv[iInputParam]) == 0 && (iInputParam + 1) < argc)
		{
			/* open yuv file */
			fpOutFile = fopen(argv[++iInputParam], "wb");
			if (NULL == fpOutFile)
			{
				fprintf(stderr, "Unable to open the file to save yuv %s.\n", argv[iInputParam]);
				goto exitmain;
			}
			printf("save yuv file: %s...\n", argv[iInputParam]);
		}
#endif
	}

	fseek(fpInFile, 0, SEEK_END);
	int iFileLen = ftell(fpInFile);
	fseek(fpInFile, 0, SEEK_SET);

	uint8_t *pInputStream = (unsigned char *)malloc(iFileLen);
	if (NULL == pInputStream)
	{
		fprintf(stderr, "Unable to malloc stream buffer (Size %d).\n", iFileLen);
		goto exitmain;
	}

	fread(pInputStream, 1, iFileLen, fpInFile);
	int StreamLen = iFileLen;
	int pStreamStart;
	int consumed = 0;
	int pStreamlen = 0;
	int naltype;
	int fps = 0;
	int length;
	int64_t tStart, tPose, tCost = 0;
	
	avcodec_register_all();
	pCodec = avcodec_find_decoder(CodecID);
	pCodecCtx = avcodec_alloc_context3(pCodec);
	packet = (AVPacket *)malloc(sizeof(AVPacket));
	av_init_packet(packet);
	pFrame = av_frame_alloc();
#if Ittiam_Enable
	pCodecCtx->codec_type = AVMEDIA_TYPE_VIDEO;
#endif
	pCodecCtx->thread_count = threadnum;
	bool isSpsFound = findSpsPos( pInputStream, &length, iFileLen);
	pCodecCtx->extradata_size = isSpsFound ? length : 200;
	pCodecCtx->extradata = pInputStream;

	if (avcodec_open2(pCodecCtx, pCodec, NULL) < 0)
	{
		av_frame_free((AVFrame **)&pFrame);
		avcodec_close(pCodecCtx);
		avcodec_free_context(&pCodecCtx);
	}

	while (StreamLen > 0)
	{
		extractFrameData(pInputStream, &pStreamStart, &pStreamlen, &naltype, consumed, iFileLen);
		packet->data = pInputStream + pStreamStart;
		packet->size = pStreamlen;
		tStart = GetTime_ms();
		Decodedlen = avcodec_decode_video2(pCodecCtx, pFrame, &frameFinished, packet);
		if (!Decodedlen)
			Decodedlen = pStreamlen;
		tPose = GetTime_ms();
		tCost += tPose - tStart;
		StreamLen -= Decodedlen;
		consumed += Decodedlen;
#if Stess_Test
		if (StreamLen < 1)
		{
			StreamLen = iFileLen;
			framenum = 0;
			consumed = 0;
		}
#endif
		if (frameFinished)
		{
#if OutputFileEnable
			if(fpOutFile != NULL)
			yv12_save(pFrame, fpOutFile);
#endif
			printf("Decode video with resolution: %d x %d  frame: %d nallen: %d\n",  \
				pFrame->width, pFrame->height, framenum++, Decodedlen);
		}
	}
	if (tCost != 0)
	   fps = 1000 * framenum / (int)tCost;

	printf("Finished decoded with fps: %d, framenum: %d\n", fps, framenum);
	av_frame_free((AVFrame **)&pFrame);
	avcodec_close(pCodecCtx);
	while (1);

exitmain:

	if (fpInFile != 0)		fclose(fpInFile);
#if OutputFileEnable
	if (fpOutFile != 0)     fclose(fpOutFile);
#endif
    return 0;  
};