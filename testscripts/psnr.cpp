#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include<unistd.h>

#define NORMAL_PSNR 35
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
psnr_calculate(char *filename1, char *filename2, char *eachpsnr, char *psnrresult, int width, int height)
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
    int framecount=0;
    int uvWidth = (width+1)/2;
    int uvHeight = (height+1)/2;
    int sizey = width*height;
    int sizeuv = uvWidth*uvHeight;

    fppsnrresult = fopen(psnrresult,"ab+");
    if (NULL==fppsnrresult)
    {
        printf("open result psnr fail\n");
        return -1;
    }

    fpraw1=fopen(filename1,"rb");
    if (NULL==fpraw1)
    {
        printf("open ref yuv fail\n");
        fprintf(fppsnrresult,"open %s fail\n",filename1);
        return -1;
    }
    fpraw2=fopen(filename2,"rb");
    if (NULL==fpraw2)
    {
        printf("open decode yuv fail\n");
        fprintf(fppsnrresult,"open %s fail\n",filename2);
        return -1;
    }
    fpeachpsnr=fopen(eachpsnr,"wb");

    if (NULL==fpeachpsnr)
    {
        printf("open record psnr fail\n");
        fprintf(fppsnrresult,"open %s fail\n",eachpsnr);
        return -1;
    }

    if(!bufferyuv1)
        bufferyuv1 = (unsigned char*)malloc(sizey);
    if(!bufferyuv2)
        bufferyuv2 = (unsigned char*)malloc(sizey);
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
    double avgy = psnrsumy/framecount;
    double avgu = psnrsumu/framecount;
    double avgv = psnrsumv/framecount;
    printf(" %s: %f  %f  %f\n\n ",filename2,avgy,avgu,avgv);
    fprintf(fpeachpsnr,"[%dx%d] frame = %d\n",width,height,framecount);
    fprintf(fpeachpsnr,"Average of psnr  %f  %f  %f\n",avgy,avgu,avgv);
    char *path = NULL ;
    if (path = strrchr (filename2, '/'))
        path++;
    strcpy(videofile,path);
    if(avgy<NORMAL_PSNR || avgu<NORMAL_PSNR || avgv<NORMAL_PSNR)
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
}

int main(int argc, char *argv[])
{
    char filename1[256] = {0};
    char filename2[256] = {0};
    char eachpsnr[256] = {0};
    char psnrresult[256] = {0};
    int width=0,height=0;
    char opt;
    char *path = NULL;
    while ((opt = getopt(argc, argv, "h:W:H:i:o:?")) != -1)
    {
        switch (opt) {
            case 'h':
            case '?':
                print_help (argv[0]);
                return false;
            case 'i':
                strcpy(filename1,optarg);
                break;
            case 'o':
                strcpy(filename2,optarg);
                strcat(eachpsnr,filename2);
                strcat(eachpsnr,".txt");
                if (path = strrchr (filename2, '/'))
                    path++;
                memcpy(psnrresult,filename2,path-filename2);
                strcat(psnrresult,"jpg_psnr.txt");
                break;
            case 'W':
                width = atoi(optarg);
                break;
            case 'H':
                height = atoi(optarg);;
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

    if (!filename1) {
        fprintf(stderr, "no input media file specified\n");
        return -1;
    }
    printf(" filename1 %s\n filename2 %s\n result    %s \n",filename1,filename2,psnrresult);
    return psnr_calculate(filename1,filename2,eachpsnr,psnrresult,width,height);
}