[![Build Status](https://travis-ci.org/01org/libyami.svg?branch=apache)](https://travis-ci.org/01org/libyami)
[![Coverity Scan Build Status](https://scan.coverity.com/projects/11605/badge.svg)](https://scan.coverity.com/projects/01org-libyami)

libyami
-------
Yet Another Media Infrastructure.

It is YUMMY to your video experience on Linux like platform.


Yami is core building block for media solution. it parses video stream

and decodes them leverage hardware acceleration.


  * Copyright (c) 2010, The WebM Project authors.

  * Copyright (C) 2011-2017 Intel Corporation

  * Copyright (C) 2015-2016 Alibaba


License
-------
libyami libraries are available under the terms of the

Apache License 2.0


Overview
--------
libyami consists of several libraries:

  * `codecparsers`: it is bit stream parser,
  * `common`: common objects/operation to work with vaapi (hw acceleration interface)
  * `decoder`: video decoder implementation
  * `encoder`: video encoder implementation
  * `vpp`: video post process implementation


Features
--------
  * MPEG-2, VC-1, WMV 9 (WMV3), H.264, HEVC (H.265), VP8, VP9, and JPEG ad-hoc decoders
  * H.264, HEVC (H.265), VP8, VP9, and JPEG ad-hoc encoders
  * Sharpening, Denoise, Deinterlace, Hue, Saturation, Brightness, Contrast, CSC and scaling


Requirements
------------
Hardware requirements

  * Intel Sandybridge, Ivybridge, Haswell, Broadwell, Skylake, Kaby Lake(HD Graphics)
  * Intel Baytrail, Braswell, Apollo Lake


Sources
-------
Git repository for work-in-progress changes is available at:

<https://github.com/01org/libyami>


Demos, Examples and Test Applications
---------------------------------------------------
The libyami-utils project provides various example, test and demo
applications that use libyami.  For more details, please refer to

    https://github.com/01org/libyami-utils


Simple api demo application
---------------------------
https://github.com/01org/libyami-utils/blob/master/examples/simpleplayer.cpp


FFmpeg integration
--------------------------
You can refer to https://github.com/01org/ffmpeg_libyami for FFmpeg integration.

You can report FFmpeg related issue to https://github.com/01org/ffmpeg_libyami/issues


Build instructions
------------------
https://github.com/01org/libyami/wiki/Build


Docs
----
http://01org.github.io/libyami_doxygen/index.html


Testing
-------

Unit Tests

  The gtest framework library <https://github.com/google/googletest> is required
  in order to write and compile unit tests in libyami.  The gtest project does
  not recommend using a pre-compiled gtest package, unfortunately.  So you will
  need to compile the gtest library from source.  The latest release can be
  downloaded from the release archive at:

    <https://github.com/google/googletest/releases>

  The gtest documentation can be found in their source tree under `docs` (online
  or in the downloaded source archive).

  To build gtest you need:

     cd ${GTEST_DIR}

     autoreconf -v --install

     ./configure --disable-shared --enable-static

     make


  To enable the unit tests, when configuring libyami you need to specify:

    --enable-gtest=${GTEST_DIR}

  where ${GTEST_DIR} is the path to where you compiled the gtest library.


Contributing
------------
Create pull request at https://github.com/01org/libyami/compare


Code style
----------
https://github.com/01org/libyami/wiki/Coding-Style


Review process
--------------
  Create pull requests at <https://github.com/01org/libyami/compare>

  We highly recommend that unit tests accompany your contributed patches.  See
  "Testing" section above.  However, we do understand that not everything can
  be tested by a unit test. So use your best judgement to determine if a unit
  test is appropriate for your contribution.  The maintainer(s) reserve the
  right to refuse submission's without unit tests, when reasonable, or if a
  submission causes existing unit tests to regress.


Mail list
---------
libyami@lists.01.org


Reporting Bugs
--------------
Bugs can be reported in the github system at:

  <https://github.com/01org/libyami/issues/new>


FAQ
---
https://github.com/01org/libyami/wiki/FAQ
