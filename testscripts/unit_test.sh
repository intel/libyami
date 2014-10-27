#!/bin/sh

if [ $# -lt 1 ]; then
echo
echo "We need one argument"
echo "./unit_test.sh  mediafile refyuv"
echo "mediafile: dir|file"
echo "refyuv:    use for psnr"
echo "## Examples of usage:"
echo "./unit_test.sh  /path/to/h264/dir/"
echo "./unit_test.sh  /path/to/vp8/dir/"
echo "./unit_test.sh  /path/to/jpeg-file/dir/ /path/to/jpeg/ref/yuv/"
echo
exit 0
fi

verify_md5()
{
    file=$1
    currentmd5=$2
    refh264path="../md5/h264_jm.md5"
    refvp8path="../md5/vpx.md5"
    refh264md5=` awk  '$2 ~/^'${file}'/{print $1}' ${refh264path} | awk '{print $1}'`
    j=0
    if [ $refh264md5 ];then
        if [ ${refh264md5} = ${currentmd5} ];then
            echo "${file}    pass"  >> ${tmpass}
        else
            echo "${file}    fail"  >>  ${tmpfail}
            failnumber=$(($failnumber+1))
        fi
    else
        refvp8md5=`awk -F: '/'${file}'/{print $1}' ${refvp8path} |awk '{print $1}' `
        if [ $refvp8md5 ];then
            if [ ${refvp8md5} = ${currentmd5} ];then
                echo "${file}    pass"  >>  ${tmpass}
            else
                echo "${file}    fail"  >>  ${tmpfail}
                failnumber=$(($failnumber+1))
            fi
        else
            echo "${file}    has no md5"  >>  ${tmpfail}
            echo "${file} has no md5, you should use md5sum to calc md5 of the yuv file by software decoder"
            echo
            failnumber=$(($failnumber+1))
        fi
    fi
}

unit_test_md5()
{
    outputpath="${scriptpath}/yuv/"
    rm -rf ${outputpath}
    mkdir -p ${outputpath}
    tmpfail="${outputpath}/fail.result"
    tmpass="${outputpath}/pass.result"
    testfilepath=$1
    cd ${outputpath}
    filelist=`ls ${testfilepath}`
    echo "Unit test md5 result:" >> ${RESULT_LOG_FILE}
    i=0
    j=0
    echo "start test md5..."
    if [ -d ${testfilepath} ];then
        for file in $filelist
        do
            echo ----------------------------------------------------
            echo ${testfilepath}${file}
            ./../../tests/yamidecode -i ${testfilepath}${file}  -m 0 -f I420 -o ${outputpath}
            mv ${file}* ${file}
            currentmd5=`md5sum ${file} | awk '{print $1}'`
            verify_md5 ${file} ${currentmd5}
            i=$(($i+1))
            echo ${i}---${testfilepath}${file}
        done
    else
        ./../../tests/yamidecode -i ${testfilepath}  -m 0 -f I420 -o ${outputpath}
        file=${testfilepath##*/}
        mv ${file}* ${file}
        currentmd5=`md5sum ${file} | awk '{print $1}'`
        verify_md5 ${file} ${currentmd5}
        i=1
    fi
    openfile=`ls ${testfilepath} | wc -l`
    echo "open file ${openfile},  actual test ${i},   fail: ${failnumber}  ">>  ${RESULT_LOG_FILE}
    if [ $failnumber -gt 0 ];then
        cat ${tmpfail} >> ${RESULT_LOG_FILE}
    fi
    if [ -f ${tmpass} ];then
        cat ${tmpass} >> ${RESULT_LOG_FILE}
    fi
    cat ${RESULT_LOG_FILE}
    echo "The test result is saved to ${RESULT_LOG_FILE} "
}

get_w_h()
{
    fileyuv=$1
    file1=${fileyuv##*_}
    file2=${file1%.*}
    WIDTH=${file2%x*}
    HEIGHT=${file2#*x}
}

unit_test_psnr()
{
    outputpath="${scriptpath}/yuv/"
    rm -rf ${outputpath}
    mkdir -p ${outputpath}
    testfilepath=$1
    cd ${outputpath}
    filelist=`ls ${testfilepath}`
    echo  "Unit test psnr result:" >> ${RESULT_LOG_FILE}
    i=0
    echo "start test psnr..."
    if [ -d ${testfilepath} ];then
        for file in $filelist
        do
            echo ----------------------------------------------------
            echo ${testfilepath}${file}
            ./../../tests/yamidecode -i ${testfilepath}${file}  -m 0 -o ${outputpath} -f I420
            fileyuv=`ls ${file}*`
            get_w_h ${fileyuv}
            refyuvfile=${refyuv}${file}.yuv
            ./../psnr -i ${refyuvfile} -o ${outputpath}${fileyuv} -W ${WIDTH} -H ${HEIGHT}
            i=$(($i+1))
            echo ${i}---${testfilepath}${file}
        done
    else
        ./../../tests/yamidecode -i ${testfilepath}  -m 0 -o ${outputpath} -f I420
        file=${testfilepath##*/}
        fileyuv=`ls ${file}*`
        get_w_h ${fileyuv}
        ./../psnr -i ${refyuv} -o ${outputpath}${fileyuv} -W ${WIDTH} -H ${HEIGHT}
        i=1
    fi
    psnr_result="${outputpath}jpg_psnr.txt"
    if [ -f $psnr_result ];then
        failcount=`wc -l ${psnr_result} | awk '{print $1}' `
    else
        failcount=0
    fi
    openfile=`ls ${testfilepath} | wc -l `

    echo "open file ${openfile},   actual test ${i},    fail: ${failcount}  " >> ${RESULT_LOG_FILE}
    if [ $failcount -gt 0 ];then
        echo "the netx is the file of failed:" >> ${RESULT_LOG_FILE}
        cat ${outputpath}/jpg_psnr.txt >> ${RESULT_LOG_FILE}
    fi
    cat ${RESULT_LOG_FILE}
    cd ${scriptpath}
    echo "The unit jpg test result is saved to ${RESULT_LOG_FILE}"
}

get_webfile()
{
    mediafile=$1
    downloaddir="/tmp/yamitest/"
    http=`echo ${mediafile} | cut -c 1-4`
    ftp=`echo ${mediafile} | cut -c 1-3`
    if [ ${http} = "http" -o ${ftp} = "ftp" ];then
        rm -rf ${downloaddir}
        mkdir -p ${downloaddir}
        wget ${mediafile} -P ${downloaddir}
        mediafile=${downloaddir}
    fi
}

mediafile=$1
refyuv=$2

scriptpath=$(cd "$(dirname "$0")"; pwd)
echo $scriptpath
DAY=`date +"%Y-%m-%d-%H-%M"`
logfilename="test_result-"${DAY}".log"
if [ ! -d "${scriptpath}/log/" ];then
    mkdir -p "${scriptpath}/log/"
fi
export RESULT_LOG_FILE=${scriptpath}/log/${logfilename}
failnumber=0

get_webfile ${mediafile}
if [ $# -eq 1 ]; then
    unit_test_md5 ${mediafile}
else
    unit_test_psnr ${mediafile}
fi
