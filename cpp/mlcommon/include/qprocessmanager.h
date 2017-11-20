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
#ifndef QPROCESSMANAGER_H
#define QPROCESSMANAGER_H

#include <QMutex>
#include <QProcess>
#include <QSet>
#include <QSharedPointer>
#include <QStack>

class QProcessManager : public QObject {
    Q_OBJECT
    Q_PROPERTY(int count READ count NOTIFY countChanged)
public:
    explicit QProcessManager(QObject* parent = 0);
    ~QProcessManager();
    int count() const;

    QSharedPointer<QProcess> start(const QString& program, const QStringList& arguments, QIODevice::OpenMode mode = QIODevice::ReadWrite);
    QSharedPointer<QProcess> start(const QString& command, QIODevice::OpenMode mode = QIODevice::ReadWrite);
    QSharedPointer<QProcess> start(QProcess* process, QIODevice::OpenMode mode = QIODevice::ReadWrite);

    void remove(const QSharedPointer<QProcess>& ptr);
signals:
    void countChanged(int);
public slots:
    void closeAll();
    QSharedPointer<QProcess> closeLast();
private slots:
    void processDestroyed(QObject*);
    void processFinished();

private:
    QSharedPointer<QProcess> registerProcess(QProcess*);
    void unregisterProcess(QProcess*);
    QStack<QSharedPointer<QProcess> > m_stack;
    mutable QMutex m_lock;
};

#endif // QPROCESSMANAGER_H
