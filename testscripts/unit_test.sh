#!/bin/bash

print_help()
{
    echo "Usage: unit_test.sh [mediafile] [refyuv]  "
    echo "Use md5 or psnr to test the validity of yami codec"
    echo ""
    echo "mediafile: dir|file"
    echo "refyuv:    use for psnr"
    echo ""
    echo "## Examples of usage:"
    echo "unit_test.sh  /path/to/video_content/(with child folders which contain h264,vp8,jpg,yuv folder)"
    echo "unit_test.sh  /path/to/h264/dir/"
    echo "unit_test.sh  /path/to/vp8/dir/"
    echo "unit_test.sh  /path/to/jpeg-file/dir/ /path/to/jpeg/ref/yuv/"
    echo "unit_test.sh  --encode /path/to/yuv/dir/ "
    echo "unit_test.sh  --encode /path/of/h264/ (test 10 specific h264 files or 10 random h264 files) "
    echo "unit_test.sh  http/web/path/"
    echo "unit_test.sh  git clone/web/path/"
    echo
    exit 0
}

verify_md5()
{
    file=$1
    currentmd5=$2
    refmd5=` awk  '$2 ~/^'${file}'/{print $1}' ${refmd5file} | awk '{print $1}'`
    j=0
    if [ $refmd5 ];then
        if [ ${refmd5} = ${currentmd5} ];then
            echo "${file}    pass"  >> ${tmpass}
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
}

single_md5()
{
    refmd5file=`ls ${testfilepath}bits.md5`
    if [ ${refmd5file} ];then
        if [ ${refmd5file} != ${testfilepath}${file} ];then
            ${yamipath}/tests/yamidecode -i ${testfilepath}${file} -m -2
            currentmd5=`grep whole ${file}.md5 | awk '{print $5}'`
            refmd5=` awk  '$2 ~/^'${file}'/{print $1}' ${refmd5file} | awk '{print $1}'`
            if [ "$currentmd5" = "$refmd5" ]; then
                echo "${file}    pass"  >> ${tmpass}
            else
                echo "${file}    fail"  >>  ${tmpfail}
                failnumber=$(($failnumber+1))
            fi
            i=$(($i+1))
            echo ${i}---${testfilepath}${file}
            rm -f ${file}.md5
        else
            openfile=$(($openfile-1))
            echo "${testfilepath}${file} is md5 file"
        fi
    else
        echo "${testfilepath} has no md5 file" 2>&1 | tee  ${tmpfail}
        echo "you should use md5sum to calc md5 of the yuv file by software decoder, and add it to ${testfilepath}"
        failnumber=$(($failnumber+1))
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
    echo -e "\nUnit test md5 result:" >> ${RESULT_LOG_FILE}
    i=0
    j=0
    failnumber=0
    echo "start test md5..."
    if [ -d ${testfilepath} ];then
        openfile=`ls ${testfilepath} | wc -l`
        for file in $filelist
        do
            echo ----------------------------------------------------
            echo ${testfilepath}${file}
            single_md5
        done
    else
        file=${testfilepath##*/}
        testfilepath=${testfilepath%/*}/
        single_md5
        i=1
        openfile=1
    fi
    echo "open file ${openfile},  actual test ${i},   fail: ${failnumber}  ">>  ${RESULT_LOG_FILE}
    if [ $failnumber -gt 0 ];then
        cat ${tmpfail} >> ${RESULT_LOG_FILE}
    fi
    if [ -f ${tmpass} ];then
        cat ${tmpass} >> ${RESULT_LOG_FILE}
    fi
    echo -e "The test result is saved to ${RESULT_LOG_FILE} \n"
}

get_w_h()
{
    fileyuv=$1
    file1=${fileyuv##*_}
    file2=${file1%.*}
    WIDTH=${file2%x*}
    HEIGHT=${file2#*x}
}

single_psnr()
{
    standardpsnr=$1
    ${yamipath}/tests/yamidecode -i ${testfilename}  -m 0 -o ${outputpath} -f I420
    fileyuv=`ls ${outfile}*`
    get_w_h ${fileyuv}
    ${yamipath}/testscripts/psnr -i ${refyuvfile} -o ${outputpath}${fileyuv} -W ${WIDTH} -H ${HEIGHT} -s ${standardpsnr}
    rm -f ${fileyuv}
}

encode_and_psnr()
{
    get_w_h ${file}
    ${yamipath}/tests/yamiencode -i ${testfilepath}${file} -s I420 -W ${WIDTH} -H ${HEIGHT} -c AVC -o ./${file}.264
    refyuvfile=${testfilepath}${file}
    testfilename=${outputpath}${file}.264
    outfile=${file}.264_
    single_psnr ${encodepsnr}
    rm -f ${testfilepath}${file} ${file}.264
}

print_psnr_result()
{
    failcount=0
    psnr_result="${outputpath}jpg_psnr.txt"
    if [ -f $psnr_result ];then
        passcount=`grep "pass" ${psnr_result} |wc -l `
        failcount=$((${i}-${passcount}))
        echo "open file ${openfile},   actual test ${i},    pass: ${passcount}    fail: ${failcount}  " >> ${RESULT_LOG_FILE}
        cat ${outputpath}/jpg_psnr.txt >> ${RESULT_LOG_FILE}
    else
        echo "open file ${openfile},   actual test ${i},    create ${psnr_result} fail  " >> ${RESULT_LOG_FILE}
    fi
    echo -e "The unit test result is saved to ${RESULT_LOG_FILE}\n"
}

unit_test_psnr()
{
    testfilepath=$1
    codec=$2
    rm_output_print=$3
    echo "rm_output=${rm_output_print}"
    outputpath="${scriptpath}/yuv/"
    if [ ${rm_output_print} = 1 ];then
        rm -rf ${outputpath}
        mkdir -p ${outputpath}
    fi
    cd ${outputpath}
    filelist=`ls ${testfilepath}`
    if [ ${codec} = "decode" ];then
        echo -e "\nUnit test jpeg result:" >> ${RESULT_LOG_FILE}
    else
        if [ ${rm_output_print} = 1 ];then
            echo -e "\nUnit test h264 encode result:" >> ${RESULT_LOG_FILE}
        fi
    fi
    i=0
    echo "start test psnr..."
    if [ -d ${testfilepath} ];then
        for file in $filelist
        do
            echo ----------------------------------------------------
            echo ${testfilepath}${file}
            if [ ${codec} = "decode" ];then
                tmpfile=${file%%.*}
                refyuvfile=`ls ${refyuv}${tmpfile}*`
                testfilename=${testfilepath}${file}
                outfile=${file}
                single_psnr ${decodepsnr}
            else
                encode_and_psnr
            fi
            i=$(($i+1))
            echo ${i}---${testfilepath}${file}
        done
    else
        file=${testfilepath##*/}
        testfilepath=${testfilepath%/*}/
        if [ ${codec} = "decode" ];then
            refyuvfile=${refyuv}
            testfilename=${testfilepath}${file}
            outfile=${file}
            single_psnr ${decodepsnr}
        else
            encode_and_psnr
        fi
        testfilepath=${testfilename}
        i=1
    fi
    if [ ${rm_output_print} = 1 ];then
        openfile=`ls ${testfilepath} | wc -l `
        print_psnr_result
    fi
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

random()
{
    min=$1;
    max=$2-$1;
    num=$(date +%s+%N);
    ((retnum=num%max+min));
    echo $retnum;
}

is_random()
{
    for n in `seq $leftfile`
    do
        if [ ${random_num[$n]} -eq $m ];then
            echo 1
            return 1
        fi
    done
    echo 0
}

decode_and_psnr()
{
    echo ${j}------${file}
    ${yamipath}/tests/yamidecode -i ${mediapath}${file}  -m 0 -o ${outputpath} -f I420
    fileyuv=`ls ${file}*`
    if [ $? -ne 0 ];then
        return 1
    fi
    j=$(($j+1))
    unit_test_psnr ${outputpath}${fileyuv} "encode" 0
}

get_encodestream_test()
{
    outputpath="${scriptpath}/yuv/"
    rm -rf ${outputpath}
    mkdir -p ${outputpath}
    cd ${outputpath}
    mediapath=$1
    j=1
    echo  -e "\nUnit test h264 encode result:" >> ${RESULT_LOG_FILE}
    encodestream="AUD_MW_E.264 BA_MW_D.264 CI_MW_D.264 FREXT01_JVC_D.264 HCAMFF1_HHI.264 MIDR_MW_D.264 NL1_Sony_D.jsv SVA_BA2_D.264 blue_sky.264 stockholm.264"
    for file in $encodestream
    do
        decode_and_psnr
    done
    filecount=`ls ${mediapath} | wc -l `
    filelist=`ls ${mediapath}`
    leftfile=$((11-$j))
    if [ $j -lt 11 ];then
        if [ $filecount -lt 10 ];then
            for file in $filelist
            do
                decode_and_psnr
            done
        else
            for index in `seq $leftfile`
            do
                random_num[$index]=$(random 1 $filecount)
                echo "random number : ${random_num[$index]}"
            done
            m=0
            for file in $filelist
            do
                m=$(($m+1))
                israndom=$(is_random)
                if [ ${israndom} -eq 1 ];then
                    decode_and_psnr
                fi
            done
        fi
    fi
    validcount=$(($j-1))
    i=${validcount}
    openfile=${validcount}
    print_psnr_result
}

recursion_dir()
{
    mediafile=$1
    filelist=`ls ${mediafile}`
    k=0
    if [ -d ${mediafile} ];then
        for file1 in $filelist
        do
            k=$(($k+1))
            echo ${k}----${file1}
            if [ -d ${mediafile}${file1} ];then
                if [ ${file1} = "jpg" ];then
                    isrecursion=1
                    refyuv=${mediafile}/yuv/
                    unit_test_psnr ${mediafile}${file1}/ "decode" 1
                else
                    if [ ${file1} != "yuv" ];then
                        isrecursion=1
                        unit_test_md5 ${mediafile}${file1}/
                    fi
                fi
                if [ ${file1} = "h264" ];then
                    isrecursion=1
                    get_encodestream_test ${mediafile}${file1}/
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

get_abs_path()
{
    mediafile1=$1
    if [ $1 = "--encode" ]; then
        return 1;
    fi
    if [ -d ${mediafile1} ];then
        mediafile1=$(cd "$mediafile1"; pwd)/
    else
        filename=${mediafile1##*/}
        path=${mediafile1%/*}
        mediafile1=$(cd "$path"; pwd)/${filename}
    fi
    echo $mediafile1
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

decodepsnr=40
encodepsnr=35
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
    mediafile=$(get_abs_path ${mediafile})
    if [ $# -eq 1 ]; then
        recursion_dir ${mediafile}
    else
        if [ $1 = "--encode" ]; then
            mediafile=$2
            mediafile=$(get_abs_path ${mediafile})
            tmp=${mediafile%/}
            ish264=${tmp##*/}
            echo "mediafile=${mediafile}, ish264=${ish264}"
            if [ ${ish264} = "h264" ]; then
                get_encodestream_test ${mediafile}
            else
                unit_test_psnr ${mediafile} "encode" 1
            fi
        else
            refyuv=$(get_abs_path ${refyuv})
            unit_test_psnr ${mediafile} "decode" 1
        fi
    fi
fi

cat ${RESULT_LOG_FILE}
echo -e "The unit test result is saved to ${RESULT_LOG_FILE}\n"
