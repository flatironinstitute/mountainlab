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
