/*
 * Copyright 2016-2017 Flatiron Institute, Simons Foundation
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
#ifndef MLPRIVATE_H
#define MLPRIVATE_H

#ifdef QT_CORE_LIB
#include <QSharedPointer>
#endif

/*
 * Usage:
 *
 * class FooPrivate;
 * class Foo : public MLPublic<FooPrivate> {
 * public:
 *   Foo() : MLPublic(new FooPrivate) { d->fooPrivateMember(); }
 * };
 *
 *
 * class FooPrivate : public MLPrivate<Foo> {
 * using MLPrivate::MLPrivate; // inherit constructor
 * public:
 *   int firstmember;
 *   int othermember;
 * };
 *
 * Foo() : d(new FooPrivate(this) {}
 */

template <typename T>
class MLPrivate {
public:
    MLPrivate(T* qq)
        : q(qq)
    {
    }

private:
    T* q;
};

template <typename T>
class MLPublic {
public:
    MLPublic(T* dd)
        : d(dd)
    {
    }
#ifndef QT_CORE_LIB
    ~MLPublic()
    {
        delete d;
    }
#endif
protected:
#ifdef QT_CORE_LIB
    QSharedPointer<T> d;
#else
    T* d;
#endif
};

#endif // MLPRIVATE_H
