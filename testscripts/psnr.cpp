/*
 * Copyright (c) 2014-2015 Intel Corporation. All rights reserved.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include<unistd.h>

#define NORMAL_PSNR 35
#define MAX_WIDTH  8192
#define MAX_HEIGHT 4320
static unsigned char *bufferyuv1 = NULL;
static unsigned char *bufferyuv2 = NULL;

static void print_help(const char* app)
{
    printf("%s <options>\n", app);
    printf("   -i raw yuv file by software decoder\n");
    printf("   -o raw yuv file by hardward decoder\n");
    printf("   -W width  of video\n");
    printf("   -H height of video\n");
}

int
psnr_calculate(char *filename1, char *filename2, const char *eachpsnr, const char *psnrresult,
               int width, int height, int standardpsnr)
{
    int size1,size2;
    char videofile[512] = {0};
    FILE * fpraw1=NULL;
    FILE * fpraw2=NULL;
    FILE * fpeachpsnr=NULL;
    FILE * fppsnrresult = NULL;
    double sum=0;
    double mse=0;
    double psny=0;
    double psnu=0;
    double psnv=0;
    double psnrsumy=0;
    double psnrsumu=0;
    double psnrsumv=0;
    double avgy;
    double avgu;
    double avgv;

    char *path = NULL ;
    int framecount=0;
    int uvWidth = (width+1)/2;
    int uvHeight = (height+1)/2;
    int sizey = width*height;
    int sizeuv = uvWidth*uvHeight;

    fppsnrresult = fopen(psnrresult,"ab+");
    if (NULL==fppsnrresult)
    {
        printf("open result psnr fail\n");
        goto error;
    }

    fpraw1=fopen(filename1,"rb");
    if (NULL==fpraw1)
    {
        printf("open ref yuv fail\n");
        fprintf(fppsnrresult,"open %s fail\n",filename1);
        goto error;
    }
    fpraw2=fopen(filename2,"rb");
    if (NULL==fpraw2)
    {
        printf("open decode yuv fail\n");
        fprintf(fppsnrresult,"open %s fail\n",filename2);
        goto error;
    }
    fpeachpsnr=fopen(eachpsnr,"wb");

    if (NULL==fpeachpsnr)
    {
        printf("open record psnr fail\n");
        fprintf(fppsnrresult,"open %s fail\n",eachpsnr);
        goto error;
    }

    if (!bufferyuv1)
        bufferyuv1 = (unsigned char*)malloc(sizey);
    if (!bufferyuv2)
        bufferyuv2 = (unsigned char*)malloc(sizey);
    if (!bufferyuv1 || !bufferyuv2) {
        goto error;
    }

    while(1)
    {
        int i, j;
//calc psnr of y
        sum=0;
        size1=fread(bufferyuv1,sizeof(char),sizey,fpraw1);
        size2=fread(bufferyuv2,sizeof(char),sizey,fpraw2);
        if ((0==size1) || (0==size2) || (size1!=size2))
            break;
        for(i = 0;i <height ;i++)
        {
            for(j = 0;j <width ;j++)
            {
                sum += ((bufferyuv1[i*width +j]  -bufferyuv2[i*width + j])*(bufferyuv1[i * width +j ]-bufferyuv2[i *width +j]));
            }
        }
        mse = sum/(sizey);
        psny = 10*log10((pow(2,8)-1)*(pow(2,8)-1)/mse);

        if (size1<0)
        {
            break;
        }
//calc u
        memset(bufferyuv1,0,sizey);
        memset(bufferyuv2,0,sizey);
        size1=fread(bufferyuv1,sizeof(char),sizeuv,fpraw1);
        size2=fread(bufferyuv2,sizeof(char),sizeuv,fpraw2);

        sum=0;
        if ((0==size1) || (0==size2) || (size1!=size2))
            break;
        for(i = 0;i <uvHeight ;i++)
        {
            for(j = 0;j <uvWidth ;j++)
            {
                sum += ((bufferyuv1[i*uvWidth +j]  -bufferyuv2[i*uvWidth + j])*(bufferyuv1[i * uvWidth +j ]-bufferyuv2[i *uvWidth +j]));
            }
        }
        mse = sum/(sizeuv);
        psnu = 10*log10((pow(2,8)-1)*(pow(2,8)-1)/mse);
//calc v
        memset(bufferyuv1,0,sizey);
        memset(bufferyuv2,0,sizey);
        size1=fread(bufferyuv1,sizeof(char),sizeuv,fpraw1);
        size2=fread(bufferyuv2,sizeof(char),sizeuv,fpraw2);

        sum=0;
        if ((0==size1) || (0==size2) || (size1!=size2))
            break;
        for(i = 0;i <uvHeight ;i++)
        {
            for(j = 0;j <uvWidth ;j++)
            {
                sum += ((bufferyuv1[i*uvWidth +j]  -bufferyuv2[i*uvWidth + j])*(bufferyuv1[i * uvWidth +j ]-bufferyuv2[i *uvWidth +j]));
            }
        }
        mse = sum/(sizeuv);
        psnv = 10*log10((pow(2,8)-1)*(pow(2,8)-1)/mse);

        fprintf(fpeachpsnr,"frame %d, psnr\t%f\t%f\t%f\n", framecount, psny,psnu,psnv);
        psnrsumy += psny;
        psnrsumu += psnu;
        psnrsumv += psnv;

        ++framecount;

        if (size1<0)
            break;
    }
    avgy = psnrsumy/framecount;
    avgu = psnrsumu/framecount;
    avgv = psnrsumv/framecount;
    printf(" %s: %f  %f  %f\n\n ",filename2,avgy,avgu,avgv);
    fprintf(fpeachpsnr,"[%dx%d] frame = %d\n",width,height,framecount);
    fprintf(fpeachpsnr,"Average of psnr  %f  %f  %f\n",avgy,avgu,avgv);
    if ((path = strrchr (filename2, '/')))
        path++;
    else
        path = filename2;
    strcpy(videofile,path);
    if(avgy<standardpsnr || avgu<standardpsnr || avgv<standardpsnr)
        fprintf(fppsnrresult,"%s: Y:%f  U:%f  V:%f    fail\n",videofile,avgy,avgu,avgv);
    else
        fprintf(fppsnrresult,"%s: Y:%f  U:%f  V:%f    pass\n",videofile,avgy,avgu,avgv);
    fclose(fpraw1);
    fclose(fpraw2);
    fclose(fpeachpsnr);
    fclose(fppsnrresult);
    if(bufferyuv1)
        free(bufferyuv1);
    if(bufferyuv2)
        free(bufferyuv2);
    return 0;
error:
    if (fppsnrresult)
        fclose(fppsnrresult);
    if (fpraw1)
        fclose(fpraw1);
    if (fpraw2)
        fclose(fpraw2);
    if (fpeachpsnr)
        fclose(fpeachpsnr);
    return -1;
}

int main(int argc, char *argv[])
{
    char* filename1 = NULL;
    char* filename2 = NULL;
    const char* eachpsnr = "every_frame_psnr.txt";
    const char* psnrresult = "average_psnr.txt";
    int width=0,height=0;
    int standardpsnr = NORMAL_PSNR;
    char opt;
    while ((opt = getopt(argc, argv, "h:W:H:i:o:s:?")) != -1)
    {
        switch (opt) {
            case 'h':
            case '?':
                print_help(argv[0]);
                return false;
            case 'i':
                filename1 = optarg;
                break;
            case 'o':
                filename2 = optarg;
                break;
            case 'W':
                width = atoi(optarg);
                break;
            case 'H':
                height = atoi(optarg);;
                break;
            case 's':
                standardpsnr = atoi(optarg);;
                break;
            default:
                print_help(argv[0]);
                break;
        }
    }
    if(argc==1) {
        print_help(argv[0]);
        return -1;
    }

    if ( !filename1 || !filename2 ) {
        fprintf(stderr, "no comparison media file specified\n");
        return -1;
    }
    if ((width <= 0) ||(height <= 0) ||(width > MAX_WIDTH) || (height > MAX_HEIGHT)) {
        printf("input width and height is invalid\n");
        return -1;
    }
    printf(" filename1 %s\n filename2 %s\n result    %s \n",filename1,filename2,psnrresult);
    return psnr_calculate(filename1,filename2,eachpsnr,psnrresult,width,height,standardpsnr);
}
