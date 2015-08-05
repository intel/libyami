import os
import sys
import hashlib
import string
import shutil
import time
class libyami:
    decodepsnr = 40
    encodepsnr = 33
    decodefiltes = ['h264','264','26l','jsv','jvt','avc','ivf','jpeg','mjpg','jpg']
    encodefiltes = ['I420','NV12','yuv']
    def __init__(self, decode, encode, psnr):
        self.decode = decode
        self.encode = encode
        self.psnr   = psnr
        self.failnumber  = 0
        self.opennumber  = 0
        self.passnumber  = 0
        self.passlist = []
        self.faillist = []
    def setdeencode(self, decode, encode):
        self.decode = decode
        self.encode = encode
    def setcompmode(self, compmode):
        self.compmode = compmode
    def setplaymode(self, playmode):
        self.playmode = playmode
    def setsavemode(self, savemode):
        self.savemode = savemode
    def setcodemode(self, codemode):
        self.codemode = codemode
    def setmd5mode(self, refmd5, mode):
        self.refmd5 = refmd5
        self.hasmd5 = mode
    def setrefmd5(self, refmd5):
        self.refmd5 = refmd5
    def setcurrentinput(self, currentinput):
        self.currentinput = currentinput
        self.currentinputbase = os.path.basename(currentinput)
    def setcurrentoutput(self, currentoutput):
        self.currentoutput = currentoutput
    def setoutputdir(self, outputdir):
        if not os.path.exists(outputdir) or not os.path.isdir(outputdir):
            os.mkdir(outputdir)
        self.outputdir = os.path.abspath(outputdir)
    def yamiplay(self, mediafile, outputdir, codemode, playmode, savemode, compmode):
        self.setcodemode(codemode)
        self.setsavemode(savemode)
        self.setplaymode(playmode)
        self.setcompmode(compmode)
        if os.path.isfile(mediafile):
            self.play(mediafile, outputdir)
        for root,dirs,files in os.walk(mediafile):
            for testfile in files:
                testfile = os.path.join(root, testfile)
                if os.path.isfile(testfile):
                    self.play(testfile, outputdir)
        if codemode == 'E' and self.compmode:
            self.parsepsnrresult()
    def play(self, mediafile, outputdir):
        self.setcurrentinput(mediafile)
        self.setoutputdir(outputdir)
        if self.codemode == 'D' and self.passfilter(mediafile, libyami.decodefiltes):
            self.opennumber += 1
            self.decodetest()
        elif self.codemode == 'E' and self.passfilter(mediafile, libyami.encodefiltes):
            self.opennumber += 1
            self.encodetest()
    def decodetest(self):
        message = ['']
        if not self.savemode:
            self.setcurrentoutput(os.path.join(self.outputdir, self.currentinputbase))
        else:
            self.setcurrentoutput(self.outputdir)
        for i in range(1, len(self.playmode)):
            if self.decodefunc(self.playmode[0], self.playmode[i], message):
                if self.playmode[0] == 'm' and self.playmode[i]== '0' and self.compmode:
                    self.writetologlist(message[0], self.comparemd5(0))
                elif self.playmode[0] == 'm' and self.playmode[i]== '-2' and self.compmode:
                    self.writetologlist(message[0], self.comparemd5(-2))
                else:
                    self.writetologlist(message[0], True)
            else:
                self.writetologlist(message[0], False)
            if not self.savemode and os.path.exists(self.currentoutput):
                os.remove(self.currentoutput)
    def encodetest(self):
        message = ['']
        self.setcurrentoutput(os.path.join(self.outputdir, self.currentinputbase+'.h264'))
        if not self.encodefunc(message):
            self.writetologlist(message[0], False)
            return
        if not self.compmode:
            os.remove(self.currentoutput)
            self.writetologlist(message[0], True)
            return
        rawI420 = self.currentinput
        self.setcurrentinput(self.currentoutput)
        self.setcurrentoutput(os.path.join(self.outputdir, os.path.basename(rawI420)))
        if not self.decodefunc('m', '0', message):
            return
        self.setcurrentinput(rawI420)
        self.psnrfunc()
        if not self.savemode:
            os.remove(self.currentoutput)
            os.remove(self.currentoutput+'.txt')
            os.remove(self.currentoutput+'.h264')
    def decodefunc(self, mode, modevalue, message):
        if mode == 'm' and modevalue == '0':
            decodeCmd = self.decode+' -i '+self.currentinput+' -'+mode+' '+modevalue+' -o '+self.currentoutput
        else:
            decodeCmd = self.decode+' -i '+self.currentinput+' -'+mode+' '+modevalue + ' -w 0'
        message[0] = self.currentinputbase + '\r'
        if os.system(decodeCmd) != 0:
            return False
        return True
    def encodefunc(self, message):
        W_H = self.get_w_h(self.currentinputbase)
        encodeCmd = self.encode+' -i '+self.currentinput+' -s I420 -W '+str(W_H[0])+' -H '+str(W_H[1])+' -o '+self.currentoutput
        message[0] = os.path.basename(self.encode) + '\r'
        if os.system(encodeCmd) != 0:
            return False
        return True
    def psnrfunc(self):
        W_H = self.get_w_h(self.currentinputbase)
        psnrCmd = self.psnr+' -i '+self.currentinput+' -o '+self.currentoutput+' -W '+str(W_H[0])+' -H '+str(W_H[1])+' -s '+str(libyami.encodepsnr)
        os.system(psnrCmd)
    def get_w_h(self, filename):
        filenamelist = filename.split('_')
        filenamelist = filenamelist[len(filenamelist) - 1].split('.')
        W_H = filenamelist[0].split('x')
        return W_H
    def comparemd5(self, mode):
        currentmd5 = self.getmd5sum(mode)
        return self.verifymd5(currentmd5)
    def getmd5sum(self, mode):
        if mode == 0: #calc md5 with dumped file
            fmd5value = hashlib.md5()
            fd = open(self.currentoutput, 'rb')
            fmd5value.update(fd.read())
            fd.close()
            return fmd5value.hexdigest()
        else:
            md5file = os.path.basename(self.currentinput) + '.md5'
            fd = open(md5file, 'r')
            for line in fd.readlines():
                line = line.strip('\n')
                if 'whole' in line:
                    strlist = line.split()
                    break
            fd.close()
            return strlist[4]
    def verifymd5(self, md5value):
        returnvalue = False
        refmd5 = os.path.join(os.path.dirname(self.currentinput),'bits.md5')
        fd = open(refmd5, 'r')
        lines = fd.readlines()
        for strline in lines:
            if md5value in strline and self.currentinputbase in strline:
                returnvalue = True
                break
        fd.close()
        return returnvalue
    def passfilter(self, mediafile, filters):
        for i in range(0, len(filters)):
            if mediafile.endswith(filters[i]):
                return True
        return False
    def writetologlist(self, message, checkvalue):
        if checkvalue:
            self.passlist.append(message)
            self.passnumber += 1
        else:
            self.faillist.append(message)
            self.failnumber += 1
        md5file = os.path.basename(self.currentinput) + '.md5'
        if os.path.exists(md5file):
            os.remove(md5file)
    def parsepsnrresult(self):
        psnrfile = os.path.join(self.outputdir, 'jpg_psnr.txt')
        if not os.path.exists(psnrfile):
            return
        fpsnr = open(psnrfile, 'r')
        lines = fpsnr.readlines()
        for line in lines:
            line = line[0:line.find(':')]
            if 'fail' in line:
                self.writetologlist(line, False)
            else:
                self.writetologlist(line, True)
        fpsnr.close()
    def writetologfile(self):
        logdirectory = os.path.join(os.getcwd(), 'log')
        if not os.path.exists(logdirectory):
            os.makedirs(logdirectory)
        logfile = 'test-result-'+time.strftime('%Y-%m-%d-%H-%M-%S')+'.log'
        logfile = os.path.join(logdirectory, logfile)
        logfilehandler = open(logfile, 'a')
        logfilehandler.write('open file:'+str(self.opennumber)+'\npass test:'+str(self.passnumber)+'\nfail test:'+str(self.failnumber))
        logfilehandler.write("\n\nfail test as follow:\n")
        for i in range(len(self.faillist)):
            logfilehandler.write(self.faillist[i]+'\n')
        logfilehandler.write("\n\npass test as follow:\n")
        for i in range(len(self.passlist)):
            logfilehandler.write(self.passlist[i]+'\n')
        logfilehandler.close()
