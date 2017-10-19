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
