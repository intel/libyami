import os
import sys
import time
import shutil
from yamilib import libyami

def decode(yamidecode, yamiencode, psnr, testfile, outputname, deformat, setmd5, assignmd5):
    if deformat == '-decode':
        outputname = outputname+os.sep+os.path.basename(testfile) 
        DecodeObject = libyami(yamidecode, yamiencode, psnr, testfile, outputname)
        if not DecodeObject.yamidecodefunc(DecodeObject.testfile, DecodeObject.outputname):
            return
        if not setmd5:
            ref_md5 = os.path.dirname(testfile)+os.sep+'bits.md5'
        else:
            ref_md5 = assignmd5
        DecodeObject.compare_md5_yuv(ref_md5)
        os.remove(outputname)
    elif deformat == '-encode':
        DecodeObject = libyami(yamidecode, yamiencode, psnr, testfile, outputname)
        if not DecodeObject.yamidecodefunc(DecodeObject.testfile, DecodeObject.outputname):
            return
        
if __name__ == '__main__':
    if len(sys.argv) < 3:
        print('Pls input: python decode.py document|dirctory -decode|-encode [md5file]')
        sys.exit(-1)
    if not os.path.exists(sys.argv[1]):
        print(sys.argv[1] + ' does not exist')
        sys.exit(-1)
    mediafile = os.path.abspath(sys.argv[1])
    deformat = sys.argv[2]
    issetmd5 = False
    if len(sys.argv) >= 4:
        if os.path.isfile(sys.argv[3]):
            assignmd5 = os.path.abspath(sys.argv[3])
            issetmd5 = True
    else:
        assignmd5 = ' '

    decodepath = os.path.dirname(os.path.abspath(sys.argv[0]))
    outputdirectory = 'yuv'
    outputdirectory = os.path.join(decodepath, outputdirectory)
    if not os.path.isdir(outputdirectory):
        os.makedirs(outputdirectory)
    else:
        shutil.rmtree(outputdirectory)
        os.makedirs(outputdirectory)    

    yamidirectory = decodepath[0:decodepath.rfind('/')]+os.sep+'tests/'
    psnr = decodepath + os.sep + 'psnr'
    yamidecode = yamidirectory + 'yamidecode'
    yamiencode = yamidirectory + 'yamiencode'

    if os.path.isfile(mediafile) and not mediafile.endswith('I420') and not mediafile.endswith('md5'):
        libyami.open_number += 1        
        decode(yamidecode, yamiencode, psnr, mediafile, outputdirectory, deformat, issetmd5, assignmd5)        
    for root,dirs,files in os.walk(mediafile):
        for testfile in files:
            testfile = root + os.sep + testfile
            if os.path.isfile(testfile) and not testfile.endswith('I420') and not testfile.endswith('md5'):
                libyami.open_number += 1
                decode(yamidecode, yamiencode, psnr, testfile, outputdirectory, deformat, issetmd5, assignmd5)
    if deformat == '-decode':
        logdirectory = 'log'
        logdirectory = os.path.join(decodepath, logdirectory)
        if not os.path.isdir(logdirectory):
            os.makedirs(logdirectory)
        logfile = 'test-result-'+time.strftime('%Y-%m-%d-%H-%M-%S')+'.log'
        logfile = logdirectory + os.sep + logfile
        logfilehandler = open(logfile, 'a')
        logfilehandler.write('libyami decode result:\n')
        logfilehandler.write('open:'+str(libyami.open_number)+',    pass:'+str(libyami.pass_number)+',    fail:'+str(libyami.fail_number)+'\n')
        logfilehandler.write('\n\nfail files:\n')
        for num in range(0, len(libyami.logfaillist)):
            logfilehandler.write(libyami.logfaillist[num]+'\n')  
        logfilehandler.write('\n\npass files:\n')
        for num in range(0, len(libyami.logpasslist)):
            logfilehandler.write(libyami.logpasslist[num]+'\n') 
        logfilehandler.close()
