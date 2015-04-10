import os
import sys
import string
import time
def v4l2config(buildpath, prefix, handler, *options):
    rootpath = os.getcwd()
    os.chdir(buildpath)
    configoptions = ''
    for i in range(len(options)):
        configoptions += ' --enable-' + options[i]
    configCmd = './autogen.sh --prefix=' + os.getenv(prefix) + configoptions + ' && make -j8 && make install'
    if os.system(configCmd) != 0:
        handler.write('\n\n\nbuild '+os.path.basename(os.getcwd())+' '+configoptions+' fail\n')
        os.chdir(rootpath)
        return False
    handler.write('\n\n\nbuild '+os.path.basename(os.getcwd())+' '+configoptions+' pass\n')
    os.chdir(rootpath)
    return True
def get_w_h(string):
    string_list = string.split('_')
    string_list = string_list[len(string_list) - 1].split('.')
    W_H_list = string_list[0].split('x')
    return W_H_list
def v4l2decodetest(v4l2decode, testfile, opt, num, handler):
    v4l2decodeCmd = v4l2decode + ' -i ' + testfile + ' -' + opt + ' ' + num
    message = os.path.basename(v4l2decode)+' '+os.path.basename(testfile)+' -'+opt+' '+num
    if os.system(v4l2decodeCmd) != 0:
        handler.write('\n'+message+' fail\n')
    else:
        handler.write('\n'+message+' pass\n')
def v4l2encodetest(v4l2encode, testfile, aformat, W, H, handler):
    v4l2encodeCmd = v4l2encode + ' -i ' + testfile + ' -s ' + aformat + ' -W ' + str(W) + ' -H ' + str(H) + ' -c AVC -o ' + testfile + '.h264'
    message = os.path.basename(v4l2encode) + ' ' + os.path.basename(testfile) + ' avc test'
    if os.system(v4l2encodeCmd) != 0:
        handler.write('\n'+message+' fail\n')
    else:
        handler.write('\n'+message+' pass\n')
        os.remove(testfile + '.h264')
def v4l2parse(testfile, decode, encode, mlist, aformat, handler):
    if aformat == 'w':
        if testfile.endswith('.jpg') or testfile.endswith('.mjpeg'):
            v4l2decodetest(decode, testfile, aformat, '0', handler)
        elif not testfile.endswith('.yuv') and not testfile.endswith('.I420'):
            v4l2decodetest(decode, testfile, aformat, '1', handler)
    elif aformat == 'm':
        if testfile.endswith('.yuv') or testfile.endswith('.I420') or testfile.endswith('.NV12'):
            W_H = get_w_h(os.path.basename(testfile))
            if encode.endswith('v4l2encode'):
                v4l2encodetest(encode, testfile, 'I420', W_H[0], W_H[1], handler)
            else:
                v4l2encodetest(encode, testfile, 'NV12', W_H[0], W_H[1], handler)
        elif not testfile.endswith('.jpg') and not testfile.endswith('.mjpeg') and not testfile.endswith('.ivf'):
            for i in range(len(mlist)):
                v4l2decodetest(decode, testfile, aformat, mlist[i], handler)
def traverse(mediafile, decode, encode, mlist, aformat, handler):
    if os.path.isfile(mediafile):
        v4l2parse(mediafile, decode, encode, mlist, aformat, handler)
    for root,dirs,files in os.walk(mediafile):
        for testfile in files:
            testfile = root + os.sep + testfile
            if os.path.isfile(testfile):
                v4l2parse(testfile, decode, encode, mlist, aformat, handler)
def v4l2test(yamidirectory, yamidecode, yamiencode, v4l2decode, v4l2encode, capidecode, capiencode, mediafile, handler):
    yamilist = ['-1', '0', '1', '2', '3', '4']
    v4l2list = ['0', '3', '4']
    capilist = ['-1', '0', '1', '2', '3', '4']
    if v4l2config(yamidirectory, 'LIBYAMI_PREFIX', handler, 'tests'):
        traverse(mediafile, yamidecode, yamiencode, yamilist, 'w', handler)
        if v4l2config(yamidirectory, 'LIBYAMI_PREFIX', handler, 'tests', 'tests-gles'):
            traverse(mediafile, yamidecode, yamiencode, yamilist, 'm', handler)
    if v4l2config(yamidirectory, 'LIBYAMI_PREFIX', handler, 'tests', 'capi'):
        traverse(mediafile, capidecode, capiencode, capilist, 'w', handler)
        if v4l2config(yamidirectory, 'LIBYAMI_PREFIX', handler, 'tests', 'capi', 'tests-gles'):
            traverse(mediafile, capidecode, capiencode, capilist, 'm', handler)
    if v4l2config(yamidirectory, 'LIBYAMI_PREFIX', handler, 'tests', 'v4l2', 'tests-gles'):
        traverse(mediafile, v4l2decode, v4l2encode, v4l2list, 'w', handler)
        handler.write('\n')
        traverse(mediafile, v4l2decode, v4l2encode, v4l2list, 'm', handler)
if __name__=='__main__':
    if len(sys.argv) < 2:
        print('Pls input: python v4l2test.py document|dirctory')
        sys.exit(-1)
    if not os.path.exists(sys.argv[1]):
        print(sys.argv[1] + ' does not exist')
        sys.exit(-1)
    mediafile = os.path.abspath(sys.argv[1])
    decodepath = os.path.dirname(os.path.abspath(sys.argv[0]))
    logdirectory = 'log'
    logdirectory = os.path.join(decodepath, logdirectory)
    if not os.path.isdir(logdirectory):
        os.makedirs(logdirectory)
    logfile = 'test-result-'+time.strftime('%Y-%m-%d-%H-%M-%S')+'.log'
    logfile = logdirectory + os.sep + logfile
    logfilehandler = open(logfile, 'a')
    logfilehandler.write('v4l2 test result:\n')

    yamidirectory = decodepath[0:decodepath.rfind('/')]
    yamitestdirectory = decodepath[0:decodepath.rfind('/')]+os.sep+'tests'
    yamidecode = yamitestdirectory + os.sep + 'yamidecode'
    yamiencode = yamitestdirectory + os.sep + 'yamiencode'
    v4l2decode = yamitestdirectory + os.sep + 'v4l2decode'
    v4l2encode = yamitestdirectory + os.sep + 'v4l2encode'
    capidecode = yamitestdirectory + os.sep + 'decodecapi'
    capiencode = yamitestdirectory + os.sep + 'encodecapi'
    v4l2test(yamidirectory, yamidecode, yamiencode, v4l2decode, v4l2encode, capidecode, capiencode, mediafile, logfilehandler)
    logfilehandler.close()
    
