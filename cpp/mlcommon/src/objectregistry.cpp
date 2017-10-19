#include "objectregistry.h"

#include "mlcommon.h"

class ObjectRegistryPrivate : public MLPrivate<ObjectRegistry> {
public:
    using MLPrivate::MLPrivate;
    QObjectList allObjects;
    QObjectList autoReleasedObjects;
};

#ifdef GLOBAL_REGISTRY
Q_GLOBAL_STATIC(ObjectRegistry, _ml_object_registry)
#else
namespace {
static ObjectRegistry* _ml_object_registry = nullptr;
}
#endif

ObjectRegistry::ObjectRegistry(QObject* parent)
    : QObject(parent)
    , MLPublic(new ObjectRegistryPrivate(this))
{
#ifndef GLOBAL_REGISTRY
    if (_ml_object_registry) {
#ifdef SHOW_OBJECT_REGISTRY_WARNINGS
        qWarning("No ObjectRegistry instance present");
#endif
        return;
    }
    _ml_object_registry = this;
#endif
}

ObjectRegistry::~ObjectRegistry()
{
    // release all autoreleased objects in reverse order
    QWriteLocker lock(listMutex());
    for (int i = d->autoReleasedObjects.size() - 1; i >= 0; --i) {
        delete d->autoReleasedObjects.at(i);
    }
#ifndef GLOBAL_REGISTRY
    if (_ml_object_registry == this) {
        _ml_object_registry = nullptr;
    }
#endif
}

/*!
 * \brief The addObject method adds \a o to the common object registry
 *
 *        Ownership of the object remains with the caller
 */
void ObjectRegistry::addObject(QObject* o)
{
    if (!o)
        return;
    if (!instance()) {
#ifdef SHOW_OBJECT_REGISTRY_WARNINGS
        qWarning("No ObjectRegistry instance present");
#endif
        return;
    }
    QWriteLocker lock(listMutex());
    instance()->d->allObjects << o;
    emit instance()->objectAdded(o);
}

/*!
 * \brief The addAutoReleasedObject method adds \a o to the common object registry
 *
 *        Ownership of the object is transferred to the registry. The object will
 *        be deleted when the registry is destroyed.
 */
void ObjectRegistry::addAutoReleasedObject(QObject* o)
{
    if (!o)
        return;
    if (!instance()) {
#ifdef SHOW_OBJECT_REGISTRY_WARNINGS
        qWarning("No ObjectRegistry instance present");
#endif
        return;
    }
    QWriteLocker lock(listMutex());
    instance()->d->allObjects << o;
    instance()->d->autoReleasedObjects << o;
    emit instance()->objectAdded(o);
}

/*!
 * \brief The removeObject method removes \a o from the common object registry.
 *
 *        The object being removed is not deleted.
 */
void ObjectRegistry::removeObject(QObject* o)
{
    if (!o)
        return;
    if (!instance()) {
#ifdef SHOW_OBJECT_REGISTRY_WARNINGS
        qWarning("No ObjectRegistry instance present");
#endif
        return;
    }
    QWriteLocker lock(listMutex());
    int idx = instance()->d->allObjects.indexOf(o);
    if (idx < 0)
        return;
    instance()->d->autoReleasedObjects.removeOne(o);
    emit instance()->objectAboutToBeRemoved(o);
    instance()->d->allObjects.removeAt(idx);
    emit instance()->objectRemoved(o);
}

ObjectRegistry* ObjectRegistry::instance()
{
    return _ml_object_registry;
}

QReadWriteLock* ObjectRegistry::listMutex()
{
    return &instance()->m_listMutex;
}

QObjectList ObjectRegistry::allObjects()
{
    if (!instance()) {
#ifdef SHOW_OBJECT_REGISTRY_WARNINGS
        qWarning("No ObjectRegistry instance present");
#endif
        return QObjectList();
    }
    return instance()->d->allObjects;
}

QObject* ObjectRegistry::getObjectByName(const QString& name)
{
    if (!instance()) {
#ifdef SHOW_OBJECT_REGISTRY_WARNINGS
        qWarning("No ObjectRegistry instance present");
#endif
        return nullptr;
        ;
    }
    QReadLocker lock(listMutex());
    QObjectList all = allObjects();
    foreach (QObject* o, all) {
        if (o->objectName() == name)
            return o;
    }
    return Q_NULLPTR;
}

QObject* ObjectRegistry::getObjectByClassName(const QString& className)
{
    if (!instance()) {
#ifdef SHOW_OBJECT_REGISTRY_WARNINGS
        qWarning("No ObjectRegistry instance present");
#endif
        return nullptr;
    }
    QReadLocker lock(listMutex());
    QObjectList all = allObjects();
    foreach (QObject* o, all) {
        const QMetaObject* mo = o->metaObject();
        if (mo->className() == className)
            return o;
    }
    return Q_NULLPTR;
}
