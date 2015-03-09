import os
import sys
import time
import shutil
from yamilib import libyami

def encode(yamidecode, yamiencode, psnr, testfile, output):
    outputname = output + os.sep + os.path.basename(testfile) + '.h264'
    decodename = output + os.sep + os.path.basename(testfile)
    DecodeObject = libyami(yamidecode, yamiencode, psnr, testfile, outputname)
    DecodeObject.yamiencodefunc(DecodeObject.testfile, DecodeObject.outputname)
    DecodeObject.yamidecodefunc(DecodeObject.outputname, decodename)
    W_H = DecodeObject.get_w_h(testfile)
    DecodeObject.psnrfunc(testfile, decodename, W_H, DecodeObject.ENCODE_PSNR)
    os.remove(outputname)
    os.remove(decodename)
    os.remove(testfile)
    
if __name__ == '__main__':
    if len(sys.argv) < 2:
        print('Pls input: python encode.py document|dirctory')
        sys.exit(-1)
    if not os.path.exists(sys.argv[1]):
        print(sys.argv[1] + ' does not exist')
        sys.exit(-1)
    mediafile = os.path.abspath(sys.argv[1])
    decodepath = os.path.dirname(os.path.abspath(sys.argv[0]))
    outputdirectory = 'encodeyuv'
    outputdirectory = os.path.join(decodepath, outputdirectory)
    if not os.path.isdir(outputdirectory):
        os.makedirs(outputdirectory)
    else:
        shutil.rmtree(outputdirectory)
        os.makedirs(outputdirectory)    

    yamidirectory = decodepath[0:decodepath.rfind('/')] + os.sep + 'tests/'
    psnr = decodepath + os.sep + 'psnr'
    yamidecode = yamidirectory + 'yamidecode'
    yamiencode = yamidirectory + 'yamiencode'

    if os.path.isfile(mediafile):
        libyami.open_number += 1        
        encode(yamidecode, yamiencode, psnr, mediafile, outputdirectory)        
    for root,dirs,files in os.walk(mediafile):
        for testfile in files:
            libyami.open_number += 1
            testfile = root + os.sep + testfile
            encode(yamidecode, yamiencode, psnr, testfile, outputdirectory)
            
    os.chdir(outputdirectory)
    if os.path.exists('jpg_psnr.txt'):
        fpsnr = open('jpg_psnr.txt', 'r')
        lines = fpsnr.readlines()
        for line in lines:
            if 'fail' in line:
                libyami.fail_number = libyami.fail_number + 1
                libyami.logfaillist.append(line)
            else:
                libyami.pass_number = libyami.pass_number + 1
                libyami.logpasslist.append(line)
        fpsnr.close()
    logdirectory = 'log'
    logdirectory = os.path.join(decodepath, logdirectory)
    if not os.path.isdir(logdirectory):
        os.makedirs(logdirectory)
    logfile = 'test-result-' + time.strftime('%Y-%m-%d-%H-%M-%S') + '.log'
    logfile = logdirectory + os.sep + logfile
    logfilehandler = open(logfile, 'a')
    logfilehandler.write('libyami encode result:\t')
    logfilehandler.write('open:'+str(libyami.open_number)+',    pass:'+str(libyami.pass_number)+',    fail:'+str(libyami.fail_number)+'\n')
    logfilehandler.write('\n\nfail files:\n')
    for num in range(0, len(libyami.logfaillist)):
        logfilehandler.write(libyami.logfaillist[num].split()[0].split(':')[0]+'\n')
    logfilehandler.write('\n\npass files:\n')
    for num in range(0, len(libyami.logpasslist)):
        logfilehandler.write(libyami.logpasslist[num].split()[0].split(':')[0]+'\n')
    logfilehandler.close()


