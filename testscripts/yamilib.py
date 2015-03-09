import os
import sys
import hashlib
import string

class libyami:
    fail_number = 0
    open_number = 0
    pass_number = 0
    logpasslist = []
    logfaillist = []
    def __init__(self, yamidecode, yamiencode, psnr, testfile, outputname):
        self.yamidecode = yamidecode
        self.yamiencode = yamiencode
        self.psnr = psnr
        self.testfile = testfile
        self.testfilebasename = os.path.basename(testfile)
        self.outputname = outputname
        self.DECODE_PSNR = 40
        self.ENCODE_PSNR = 35
        
    def yamiencodefunc(self, inputfile, outputfile):
        W_H = self.get_w_h(os.path.basename(inputfile))
        yamiencodeCmd = self.yamiencode+' -i '+inputfile+' -s I420 -W '+str(W_H[0])+' -H '+str(W_H[1])+' -c AVC -o '+outputfile
        os.system(yamiencodeCmd)
        
    def yamidecodefunc(self, inputfile, outputfile):
        yamidecodeCmd = self.yamidecode+' -i '+inputfile+' -m 0 -f I420 -o '+outputfile
        if os.system(yamidecodeCmd) != 0:
            self.logfaillist.append(os.path.basename(inputfile))
            libyami.fail_number = libyami.fail_number + 1
            return False
        return True
            
    def psnrfunc(self, ref_yuv, decodefile, W_H, standardpsnr):
        psnrCmd = self.psnr+' -i '+ref_yuv+' -o '+decodefile+' -W '+str(W_H[0])+' -H '+str(W_H[1])+' -s '+str(standardpsnr)
        os.system(psnrCmd)

    def get_w_h(self, string):
        string_list = string.split('_')
        string_list = string_list[len(string_list) - 1].split('.')
        W_H_list = string_list[0].split('x')
        return W_H_list

    def compare_md5_yuv(self, ref_md5):
        currentmd5 = self.md5sum()
        if self.verify_md5(currentmd5, ref_md5):
            self.logpasslist.append(self.testfilebasename)
            libyami.pass_number = libyami.pass_number + 1
        else:
            self.logfaillist.append(self.testfilebasename)
            libyami.fail_number = libyami.fail_number + 1

    def md5sum(self):
        fmd5value = hashlib.md5()
        fd = open(self.outputname, 'rb')
        fmd5value.update(fd.read())
        fd.close()
        return fmd5value.hexdigest()          

    def verify_md5(self, md5value, ref_md5):
        returnvalue = False
        fd = open(ref_md5, 'r')
        lines = fd.readlines() 
        for strline in lines:
            if md5value in strline and self.testfilebasename in strline:
                returnvalue = True
                break
        fd.close()
        return returnvalue
