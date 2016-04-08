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

#ifndef __COMMON_FACTORY_H__
#define __COMMON_FACTORY_H__

#include <map>
#include <string>
#include <vector>

template < class T >
class Factory {
public:
    typedef T* Type;
    typedef Type (*Creator) (void);
    typedef std::string KeyType;
    typedef std::vector<KeyType> Keys;
    typedef std::map<KeyType, Creator> Creators;
    typedef typename Creators::iterator iterator;
    typedef typename Creators::const_iterator const_iterator;

    /**
     * Register class C with @param key.  Returns true if class C is
     * successfully registered using @param key.  If @param key is already
     * registered, returns false and does not register class C with
     * @param key.
     */
    template < class C >
    static bool register_(const KeyType& key)
    {
        std::pair<iterator, bool> result =
            getCreators().insert(std::make_pair(key, create<C>));
        return result.second;
    }

    /**
     * Create and return a new object that is registered with @param key.
     * If @param key does not exist, then returns NULL.  The caller is
     * responsible for managing the lifetime of the returned object.
     */
    static Type create(const KeyType& key)
    {
        Creators& creators = getCreators();
        const const_iterator creator(creators.find(key));
        if (creator != creators.end())
            return creator->second();
        return NULL;
    }

    static Keys keys()
    {
        Keys result;
        const Creators& creators = getCreators();
        const const_iterator endIt(creators.end());
        for (const_iterator it(creators.begin()); it != endIt; ++it)
            result.push_back(it->first);
        return result;
    }

private:
    template < class C >
    static Type create()
    {
        return new C;
    }
    static Creators& getCreators()
    {
        static Creators creators;
        return creators;
    }
};

#endif // __COMMON_FACTORY_H__
