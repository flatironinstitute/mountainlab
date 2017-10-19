/******************************************************
** See the accompanying README and LICENSE files
** Author(s): Witold Wysota
**
** Based on QtCreator code
*******************************************************/

#ifndef OBJECTREGISTRY_H
#define OBJECTREGISTRY_H

#include <QObject>
#include <QReadWriteLock>
#include <QSharedPointer>
#include "mlprivate.h"

class ObjectRegistryPrivate;
class ObjectRegistry : public QObject, public MLPublic<ObjectRegistryPrivate> {
    Q_OBJECT
public:
    explicit ObjectRegistry(QObject* parent = 0);
    ~ObjectRegistry();

    static void addObject(QObject*);
    static void addAutoReleasedObject(QObject*);
    static void removeObject(QObject*);
    static QObjectList allObjects();

    template <typename T>
    static T* getObject()
    {
        if (!instance()) {
#ifdef SHOW_OBJECT_REGISTRY_WARNINGS
            qWarning("No ObjectRegistry instance present");
#endif
            return nullptr;
        }
        QReadLocker locker(listMutex());
        QObjectList all = allObjects();
        foreach (QObject* o, all) {
            if (T* obj = qobject_cast<T*>(o))
                return obj;
        }
        return Q_NULLPTR;
    }
    template <typename T>
    static QList<T*> getObjects()
    {
        if (!instance()) {
#ifdef SHOW_OBJECT_REGISTRY_WARNINGS
            qWarning("No ObjectRegistry instance present");
#endif
            return QList<T*>();
        }
        QReadLocker locker(listMutex());
        QObjectList all = allObjects();
        QList<T*> result;
        foreach (QObject* o, all) {
            if (T* obj = qobject_cast<T*>(o))
                result.append(obj);
        }
        return result;
    }

    template <typename T, typename P>
    static T* getObject(P pred)
    {
        if (!instance()) {
#ifdef SHOW_OBJECT_REGISTRY_WARNINGS
            qWarning("No ObjectRegistry instance present");
#endif
            return nullptr;
        }
        QReadLocker locker(listMutex());
        QObjectList all = allObjects();
        foreach (QObject* o, all) {
            if (T* obj = qobject_cast<T*>(o)) {
                if (pred(obj))
                    return obj;
            }
        }
        return Q_NULLPTR;
    }

    template <typename T, typename P>
    static QList<T*> getObjects(P pred)
    {
        if (!instance()) {
#ifdef SHOW_OBJECT_REGISTRY_WARNINGS
            qWarning("No ObjectRegistry instance present");
#endif
            return QList<T*>();
        }
        QReadLocker locker(listMutex());
        QObjectList all = allObjects();
        QList<T*> result;
        foreach (QObject* o, all) {
            if (T* obj = qobject_cast<T*>(o))
                if (pred(obj))
                    result.append(obj);
        }
        return result;
    }
    static QObject* getObjectByName(const QString& name);
    static QObject* getObjectByClassName(const QString& className);

    static ObjectRegistry* instance();
signals:
    void objectAdded(QObject* o);
    void objectAboutToBeRemoved(QObject* o);
    void objectRemoved(QObject* o);

private:
    static QReadWriteLock* listMutex();
    QReadWriteLock m_listMutex;
};

#endif // OBJECTREGISTRY_H
