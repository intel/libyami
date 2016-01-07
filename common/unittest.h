/*
 * Copyright (C) 2015 Intel Corporation. All rights reserved.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef unittest_h
#define unittest_h

/*
 * The following #define prevents gtest from using it's own tr1 tuple
 * implementation. This is necessary since we use C++11 and/or the system
 * provided tr1/tuple[.h] in this project. The gtest header appears to
 * incorrectly detect the availability of the system tr1/tuple[.h] and thus
 * tries to use it's own implementation of it... and thus will cause compile
 * failures due to duplicate definitions of tr1::tuple.
 */
#define GTEST_USE_OWN_TR1_TUPLE 0

#include <gtest/gtest.h>

#endif // unittest_h
