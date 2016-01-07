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

// primary header
#include "common/factory.h"

// library headers
#include "common/unittest.h"

// system headers
#include <vector>

template <class Base, class Derived>
class FactoryTest
    : public ::testing::Test {
protected:
    typedef std::vector<typename Factory<Base>::KeyType> FactoryKeys;

    void doFactoryTest(const FactoryKeys& keys) {
        ::testing::StaticAssertTypeEq<Base*, typename Factory<Base>::Type>();

        EXPECT_TRUE(Derived::s_registered);

        const typename FactoryKeys::const_iterator end(keys.end());
        for ( typename FactoryKeys::const_iterator key(keys.begin());
              key != end; ++key) {
            SCOPED_TRACE("key: " + *key);

            ASSERT_FALSE(Factory<Base>::template register_<Derived>(*key));

            Base* object(NULL);

            ASSERT_TRUE(object == NULL);

            object = Factory<Base>::create(*key);

            EXPECT_TRUE(object != NULL);

            EXPECT_TRUE(dynamic_cast<Derived*>(object));

            delete object;
        }
    }
};
