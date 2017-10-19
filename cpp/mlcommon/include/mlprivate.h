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
