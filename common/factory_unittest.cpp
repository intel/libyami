/*
 * Copyright 2016 Intel Corporation
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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

// primary header
#include "factory.h"

// library headers
#include "common/unittest.h"

#define FACTORY_TEST(name) \
    TEST(FactoryTest, name)

class Base {
public:
    virtual ~Base() { }
};

class A : public Base { };
class B : public Base { };
class C : public Base { };

// Attempt to expose what is commonly referred to as the
// "static initialization order fiasco".
// If the unittest program segfaults during pre-main static
// initialization, then we've likely introduced an initialization
// order bug.
const bool rA = Factory<Base>::register_<A>("A");

FACTORY_TEST(Register) {
    EXPECT_TRUE(rA);
    EXPECT_TRUE(Factory<Base>::register_<B>("B"));
    EXPECT_TRUE(Factory<Base>::register_<B>("B2"));
    EXPECT_TRUE(Factory<Base>::register_<C>("C"));
    EXPECT_FALSE(Factory<Base>::register_<A>("A"));
    EXPECT_FALSE(Factory<Base>::register_<B>("B"));
    EXPECT_FALSE(Factory<Base>::register_<C>("C"));
}

FACTORY_TEST(Create) {
    Base* a = Factory<Base>::create("A");
    Base* z = Factory<Base>::create("Z");

    EXPECT_TRUE(a != NULL);
    EXPECT_TRUE(z == NULL);
    EXPECT_TRUE(dynamic_cast<const A*>(a) != NULL);
    EXPECT_TRUE(dynamic_cast<const B*>(a) == NULL);

    delete a;
}
