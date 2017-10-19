#include "qprocessmanager.h"
#include <QMutexLocker>
#include <QDebug>

/*!
 * \class QProcessManager
 * \brief The QProcessManager class manages instances of QProcess.
 *
 * The class came to be as a result of a bug/problem in Qt that interrupting
 * a process does not kill external processes it might have spawned.
 *
 * To circumvent this problem processes can be started using this class.
 * A signal handler has to be installed so that when the process is signalled,
 * it calls QProcessManager::closeAll() on the manager instance.
 */

static void processDeleter(QProcess* process)
{
    if (!process->parent())
        process->deleteLater();
}

QProcessManager::QProcessManager(QObject* parent)
    : QObject(parent)
    , m_lock(QMutex::Recursive)
{
}

QProcessManager::~QProcessManager()
{
    closeAll();
}

int QProcessManager::count() const
{
    QMutexLocker locker(&m_lock);
    return m_stack.size();
}

QSharedPointer<QProcess> QProcessManager::start(const QString& program, const QStringList& arguments, QIODevice::OpenMode mode)
{
    QSharedPointer<QProcess> ptr = registerProcess(new QProcess);
    ptr->start(program, arguments, mode);
    return ptr;
}

QSharedPointer<QProcess> QProcessManager::start(const QString& command, QIODevice::OpenMode mode)
{
    QSharedPointer<QProcess> ptr = registerProcess(new QProcess);
    ptr->start(command, mode);
    return ptr;
}

QSharedPointer<QProcess> QProcessManager::start(QProcess* process, QIODevice::OpenMode mode)
{
    QSharedPointer<QProcess> ptr = registerProcess(process);
    ptr->open(mode);
    return ptr;
}

void QProcessManager::remove(const QSharedPointer<QProcess>& ptr)
{
    QMutexLocker locker(&m_lock);

    if (m_stack.removeOne(ptr)) {
        unregisterProcess(ptr.data());
        emit countChanged(m_stack.size());
    }
    locker.unlock();
}

void QProcessManager::closeAll()
{
    QMutexLocker locker(&m_lock);
    QStack<QSharedPointer<QProcess> > processes;
    qSwap(m_stack, processes);
    emit countChanged(m_stack.size());
    locker.unlock();
    foreach (QSharedPointer<QProcess> process, processes) {
        qDebug().noquote() << "QProcessManager:: killing process: " << process->processId() << process->program() + process->arguments().join(" ");
        unregisterProcess(process.data());
        process->kill();
    }
}

QSharedPointer<QProcess> QProcessManager::closeLast()
{
    QMutexLocker locker(&m_lock);
    if (m_stack.isEmpty())
        return QSharedPointer<QProcess>();
    QSharedPointer<QProcess> process = m_stack.pop();
    unregisterProcess(process.data());
    emit countChanged(m_stack.size());
    locker.unlock();
    process->close();
    return process;
}

void QProcessManager::processDestroyed(QObject* o)
{
    QProcess* process = static_cast<QProcess*>(o);
    QMutexLocker locker(&m_lock);
    for (int i = 0; i < m_stack.size(); ++i) {
        if (m_stack.at(i) == process) {
            unregisterProcess(process);
            m_stack.removeAt(i);
            return;
        }
    }
}

void QProcessManager::processFinished()
{
    QProcess* process = qobject_cast<QProcess*>(sender());
    QMutexLocker locker(&m_lock);
    for (int i = 0; i < m_stack.size(); ++i) {
        if (m_stack.at(i) == process) {
            unregisterProcess(process);
            m_stack.removeAt(i);
            return;
        }
    }
}

QSharedPointer<QProcess> QProcessManager::registerProcess(QProcess* process)
{
    connect(process, SIGNAL(destroyed(QObject*)), this, SLOT(processDestroyed(QObject*)));
    connect(process, SIGNAL(finished(int)), this, SLOT(processFinished()));
    QSharedPointer<QProcess> ptr(process, processDeleter);
    QMutexLocker locker(&m_lock);
    m_stack.push(ptr);
    emit countChanged(m_stack.size());
    return ptr;
}

void QProcessManager::unregisterProcess(QProcess* process)
{
    process->disconnect(this);
}
