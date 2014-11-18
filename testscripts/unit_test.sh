#!/bin/sh

print_help()
{
    echo "Usage: unit_test.sh [mediafile] [refyuv]  "
    echo "Use md5 or psnr to test the validity of yami codec"
    echo ""
    echo "mediafile: dir|file"
    echo "refyuv:    use for psnr"
    echo ""
    echo "## Examples of usage:"
    echo "unit_test.sh  /path/to/h264/dir/"
    echo "unit_test.sh  /path/to/vp8/dir/"
    echo "unit_test.sh  /path/to/jpeg-file/dir/ /path/to/jpeg/ref/yuv/"
    echo "unit_test.sh  --encode /path/to/yuv/dir/ "
    echo "unit_test.sh  http/web/path/"
    echo "unit_test.sh  git clone/web/path/"
    echo
    exit 0
}

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
    failnumber=0
    echo "start test md5..."
    if [ -d ${testfilepath} ];then
        for file in $filelist
        do
            echo ----------------------------------------------------
            echo ${testfilepath}${file}
            ${yamipath}/tests/yamidecode -i ${testfilepath}${file}  -m 0 -f I420 -o ${outputpath}
            mv ${file}* ${file}
            currentmd5=`md5sum ${file} | awk '{print $1}'`
            verify_md5 ${file} ${currentmd5}
            i=$(($i+1))
            echo ${i}---${testfilepath}${file}
        done
    else
        ${yamipath}/tests/yamidecode -i ${testfilepath}  -m 0 -f I420 -o ${outputpath}
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
    testfilepath=$1
    codec=$2
    rm_output=$3
    echo "rm_output=${rm_output}"
    outputpath="${scriptpath}/yuv/"
    if [ ${rm_output} = 1 ];then
        rm -rf ${outputpath}
    else
        rm "${outputpath}jpg_psnr.txt"
    fi
    mkdir -p ${outputpath}
    cd ${outputpath}
    filelist=`ls ${testfilepath}`
    if [ ${codec} = "decode" ];then
        echo  "Unit test jpeg result:" >> ${RESULT_LOG_FILE}
    else
        echo  "Unit test h264 encode result:" >> ${RESULT_LOG_FILE}
    fi
    i=0
    echo "start test psnr..."
    if [ -d ${testfilepath} ];then
        for file in $filelist
        do
            echo ----------------------------------------------------
            echo ${testfilepath}${file}
            if [ ${codec} = "decode" ];then
                ${yamipath}/tests/yamidecode -i ${testfilepath}${file}  -m 0 -o ${outputpath} -f I420
                fileyuv=`ls ${file}*`
                get_w_h ${fileyuv}
                tmpfile=${file%%.*}
                refyuvfile=`ls ${refyuv}${tmpfile}*`
                ${yamipath}/testscripts/psnr -i ${refyuvfile} -o ${outputpath}${fileyuv} -W ${WIDTH} -H ${HEIGHT}
            else
                get_w_h ${file}
                ${yamipath}/tests/yamiencode -i ${testfilepath}${file} -s I420 -W ${WIDTH} -H ${HEIGHT} -c AVC -o ./${file}.264
                ${yamipath}/tests/yamidecode -i ${file}.264  -m 0 -o ${outputpath} -f I420
                refyuvfile=`ls ${outputpath}${file}.264_*`
                ${yamipath}/testscripts/psnr -i ${testfilepath}${file} -o ${refyuvfile} -W ${WIDTH} -H ${HEIGHT}
            fi
            i=$(($i+1))
            echo ${i}---${testfilepath}${file}
        done
    else
        file=${testfilepath##*/}
        if [ ${codec} = "decode" ];then
            ${yamipath}/tests/yamidecode -i ${testfilepath}  -m 0 -o ${outputpath} -f I420
            file=${testfilepath##*/}
            fileyuv=`ls ${file}*`
            get_w_h ${fileyuv}
            ${yamipath}/testscripts/psnr -i ${refyuv} -o ${outputpath}${fileyuv} -W ${WIDTH} -H ${HEIGHT}
        else
            get_w_h ${file}
            ${yamipath}/tests/yamiencode -i ${testfilepath} -s I420 -W ${WIDTH} -H ${HEIGHT} -c AVC -o ./${file}.264
            ${yamipath}/tests/yamidecode -i ${file}.264  -m 0 -o ${outputpath} -f I420
            refyuvfile=`ls ${outputpath}${file}.264_*`
            ${yamipath}/testscripts/psnr -i ${testfilepath} -o ${refyuvfile} -W ${WIDTH} -H ${HEIGHT}
        fi
        i=1
    fi
    failcount=0
    openfile=`ls ${testfilepath} | wc -l `
    psnr_result="${outputpath}jpg_psnr.txt"
    if [ -f $psnr_result ];then
        passcount=`grep "pass" ${psnr_result} |wc -l `
        failcount=$((${i}-${passcount}))
        echo "open file ${openfile},   actual test ${i},    pass: ${passcount}    fail: ${failcount}  " >> ${RESULT_LOG_FILE}
        cat ${outputpath}/jpg_psnr.txt >> ${RESULT_LOG_FILE}
    else
        echo "open file ${openfile},   actual test ${i},    create ${psnr_result} fail  " >> ${RESULT_LOG_FILE}
    fi
    echo "The unit test result is saved to ${RESULT_LOG_FILE}"
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

git_clone_video()
{
    if test -e bat_video_content/video_auto_pull ; then
        if ! (cd bat_video_content && tsocks git pull ) ; then
            echo "git pull failed, (re)move bat_video_content/video_auto_pull to disable pulling"
            exit 1
        fi
    fi

    if ! test -e bat_video_content ; then
        echo "No bat_video_content checkout, press enter to download one with git or CTRL+C to abort"
        read tmp
        if ! git clone $1 ; then
            rm -rf bat_video_content
            echo "Failed to get a bat_video_content checkout"
            exit 1
        fi
        touch bat_video_content/video_auto_pull
    fi

    mediafile=${scriptpath}/bat_video_content/
    refyuv=${scriptpath}/bat_video_content/yuv/
}

is_git_path()
{
    git=`echo $1 | cut -c 1-3`
    if [ ${git} = "git" ];then
        return 1
    else
        return 0
    fi
}

get_encodestream_test()
{
    outputpath="${scriptpath}/yuv/"
    rm -rf ${outputpath}
    mkdir -p ${outputpath}
    cd ${outputpath}
    mediapath=$1
    encodestream="AUD_MW_E.264 BA_MW_D.264 CI_MW_D.264 FREXT01_JVC_D.264 HCAMFF1_HHI.264 MIDR_MW_D.264 NL1_Sony_D.jsv SVA_BA2_D.264"
    for file in $encodestream
        do
            ${yamipath}/tests/yamidecode -i ${mediapath}${file}  -m 0 -o ${outputpath} -f I420
            fileyuv=`ls ${file}*`
            if [ $? -ne 0 ];then
                exit 0
            fi
            unit_test_psnr ${outputpath}${fileyuv} "encode" 0
        done
    rm -rf ${outputpath}
}

recursion_dir()
{
    mediafile=$1
    filelist=`ls ${mediafile}`
    if [ -d ${mediafile} ];then
        for file1 in $filelist
        do
            if [ -d ${mediafile}${file1} ];then
                if [ ${file1} = "h264" ];then
                    isrecursion=1
                    unit_test_md5 ${mediafile}${file1}/
                    get_encodestream_test ${mediafile}${file1}/
                else
                    if [ ${file1} = "vp8" ];then
                        isrecursion=1
                        unit_test_md5 ${mediafile}${file1}/
                    else
                        if [ ${file1} = "jpg" ];then
                            isrecursion=1
                            refyuv=${mediafile}/yuv/
                            unit_test_psnr ${mediafile}${file1}/ "decode" 1
                        fi
                    fi
                fi
            fi
        done
        if [ ${isrecursion} -eq 0 ]; then
            unit_test_md5 ${mediafile}
        fi
    else
        unit_test_md5 ${mediafile}
    fi
}

if [ $# -eq 0 ]; then
    print_help
else
    if [ $# -eq 1 ]; then
        if [ $1 = "-h" -o $1 = "--help" ]; then
            print_help
        fi
    fi
fi


mediafile=$1
refyuv=$2

scriptpath=$(cd "$(dirname "$0")"; pwd)
yamipath=${scriptpath%/*}
failnumber=0
DAY=`date +"%Y-%m-%d-%H-%M"`
logfilename="test_result-"${DAY}".log"
if [ ! -d "${scriptpath}/log/" ];then
    mkdir -p "${scriptpath}/log/"
fi
export RESULT_LOG_FILE=${scriptpath}/log/${logfilename}

is_git_path ${mediafile}
if [ $? = 1 ];then
    git_clone_video ${mediafile}
    recursion_dir ${mediafile}
else
    isrecursion=0
    get_webfile ${mediafile}
    if [ -d ${mediafile} ];then
        mediafile=$(cd "$mediafile"; pwd)
    fi
    if [ $# -eq 1 ]; then
        recursion_dir ${mediafile}/
    else
        if [ $1 = "--encode" ]; then
            mediafile=$2
            unit_test_psnr ${mediafile}/ "encode" 1
        else
            unit_test_psnr ${mediafile}/ "decode" 1
        fi
    fi
fi

cat ${RESULT_LOG_FILE}
echo "The unit jpg test result is saved to ${RESULT_LOG_FILE}"
