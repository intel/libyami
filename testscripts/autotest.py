#! /usr/bin/env python
import os
import sys
import shutil
from yamilib import libyami
if __name__ == '__main__':
    if len(sys.argv) < 5:
        print('Pls autotest.py file|dirctory decode|encode|v4l2 outdir savemode')
        sys.exit(-1)
    if not os.path.exists(sys.argv[1]):
        print(sys.argv[1] + ' does not exist')
        sys.exit(-1)
    mediafiles = os.path.abspath(sys.argv[1])
    testformat = sys.argv[2]
    testoutput = sys.argv[3]
    savemode = True if int(sys.argv[4])==1 else False
    autotestpath = os.path.dirname(os.path.abspath(sys.argv[0]))
    yamitestpath = os.path.join(autotestpath[0:autotestpath.rfind('/')],'tests')
    yamidecode = os.path.join(yamitestpath, 'yamidecode')
    yamiencode = os.path.join(yamitestpath, 'yamiencode')
    v4l2decode = os.path.join(yamitestpath, 'v4l2decode')
    v4l2encode = os.path.join(yamitestpath, 'v4l2encode')
    capidecode = os.path.join(yamitestpath, 'decodecapi')
    capiencode = os.path.join(yamitestpath, 'encodecapi')
    psnr       = os.path.join(autotestpath, 'psnr')
    decodemodelist = ['m', '0', '-2']
    encodemodelist = ['m', '0']
    v4l2wmodelist  = ['w', '0']
    v4l2mmodelist  = ['m', '0', '3', '4']

    if testformat == 'decode':
        yamiobject = libyami(yamidecode, yamiencode, psnr)
        yamiobject.yamiplay(mediafiles, testoutput, 'D', decodemodelist, savemode, not savemode)
        yamiobject.writetologfile()
    elif testformat == 'encode':
        yamiobject = libyami(yamidecode, yamiencode, psnr)
        yamiobject.yamiplay(mediafiles, testoutput, 'E', encodemodelist, savemode, True)
        yamiobject.writetologfile()
    elif testformat == 'v4l2':
        yamiobject = libyami(yamidecode, yamiencode, psnr)
        yamiobject.yamiplay(mediafiles, testoutput, 'D', v4l2wmodelist, savemode, False)
        yamiobject.yamiplay(mediafiles, testoutput, 'D', v4l2mmodelist, savemode, False)
        yamiobject.yamiplay(mediafiles, testoutput, 'E', v4l2mmodelist, savemode, False)
        yamiobject.setdeencode(capidecode, capiencode)
        yamiobject.yamiplay(mediafiles, testoutput, 'D', v4l2wmodelist, savemode, False)
        yamiobject.yamiplay(mediafiles, testoutput, 'D', v4l2mmodelist, savemode, False)
        yamiobject.yamiplay(mediafiles, testoutput, 'E', v4l2mmodelist, savemode, False)
        yamiobject.setdeencode(v4l2decode, v4l2encode)
        yamiobject.yamiplay(mediafiles, testoutput, 'D', v4l2wmodelist, savemode, False)
        yamiobject.yamiplay(mediafiles, testoutput, 'D', v4l2mmodelist, savemode, False)
        yamiobject.yamiplay(mediafiles, testoutput, 'E', v4l2mmodelist, savemode, False)
        yamiobject.writetologfile()
