/******************************************************
** See the accompanying README and LICENSE files
** Author(s): Jeremy Magland, Witold Wysota
** Created: 4/30/2016
*******************************************************/

#ifndef TASKPROGRESS_H
#define TASKPROGRESS_H

#include <QDateTime>
#include <QObject>
#include <QString>
#include <QDebug>
#include <QAbstractItemModel>
#include <QReadWriteLock>
#include "mlcommon.h"

struct TaskProgressLogMessage {
    enum Type {
        Log,
        Error
    };

    QString message;
    QDateTime time;
    Type type;
    TaskProgressLogMessage()
        : type(Log)
    {
    }
    TaskProgressLogMessage(const QString& msg, Type t = Log)
        : message(msg)
        , time(QDateTime::currentDateTime())
        , type(t)
    {
    }
    TaskProgressLogMessage(const QDateTime& dt, const QString& msg, Type t = Log)
        : message(msg)
        , time(dt)
        , type(t)
    {
    }
};

struct TaskInfo {
    QSet<QString> tags;
    QString label;
    QString description;
    QList<TaskProgressLogMessage> log_messages;
    QString error;
    double progress;
    QDateTime start_time;
    QDateTime end_time;
};

Q_DECLARE_METATYPE(TaskInfo)

class TaskProgress;

class MLDebug : public QDebug {
public:
    MLDebug(const MLDebug& other);
    ~MLDebug();

private:
    enum Mode {
        Log,
        Error
    };
    MLDebug(TaskProgress* tp, Mode m);

    TaskProgress* m_tp;
    QString m_string;
    Mode m_mode;
    friend class TaskProgress;
};

class TaskProgress : public QObject {
    Q_OBJECT
    Q_PROPERTY(QString label READ label WRITE setLabel)
    Q_PROPERTY(QString description READ description WRITE setDescription)
public:
    enum StandardCategory {
        None = 0,
        Download = (1 << 0),
        Calculate = (1 << 1),
        Process = (1 << 2)
    };
    Q_DECLARE_FLAGS(StandardCategories, StandardCategory)

    TaskProgress();
    TaskProgress(const QString& label);
    TaskProgress(StandardCategories tags, const QString& label);
    ~TaskProgress();
    QString label() const;
    QString description() const;
    QSet<QString> tags() const;
    void setLabel(const QString& label);
    void setDescription(const QString& description);
    void addTag(StandardCategory);
    void addTag(const QString& tag);
    void removeTag(StandardCategory);
    void removeTag(const QString& tag);
    bool hasTag(StandardCategory) const;
    bool hasTag(const QString& tag) const;
    double progress() const;
    MLDebug log();
    MLDebug error();
public slots:
    void log(const QString& log_message);
    void error(const QString& error_message);
    void setProgress(double pct);

protected:
    QStringList catsToString(StandardCategories) const;
    QString catToString(StandardCategory) const;

private:
    TaskInfo m_info;
    int m_id;
};

Q_DECLARE_OPERATORS_FOR_FLAGS(TaskProgress::StandardCategories);

namespace TaskManager {

class TagsFilterProxyModel;

class TaskProgressAgent : public QObject {
    Q_OBJECT
public:
    virtual const TaskInfo& taskInfo() const = 0;
signals:
    void changed();
};

class TaskProgressModel;

class TaskProgressMonitor : public QObject {
    Q_OBJECT
public:
    virtual int count() const = 0;
    virtual TaskProgressAgent* at(int index) const = 0;
    virtual int indexOf(TaskProgressAgent*) const = 0;
    static TaskProgressMonitor* globalInstance();

    virtual TaskProgressModel* model() const = 0;
};

class TaskProgressAgentPrivate;

class TaskProgressModel : public QAbstractItemModel {
    Q_OBJECT
public:
    enum {
        ProgressRole = Qt::UserRole,
        StartTimeRole,
        EndTimeRole,
        TagsRole,
        TagsSetRole,
        LogRole,
        IndentedLogRole,
        StatusRole,
        LogTypeRole,
        HasErrorRole
    };

    enum Status {
        Active,
        Canceled,
        Finished
    };

    enum LogType {
        Log,
        Error
    };
    Q_ENUM(LogType)

    TaskProgressModel(QObject* parent = 0);
    ~TaskProgressModel();
    QModelIndex index(int row, int column = 0, const QModelIndex& parent = QModelIndex()) const Q_DECL_OVERRIDE;
    QModelIndex parent(const QModelIndex& child) const Q_DECL_OVERRIDE;
    int rowCount(const QModelIndex& parent = QModelIndex()) const Q_DECL_OVERRIDE;
    int columnCount(const QModelIndex& parent = QModelIndex()) const Q_DECL_OVERRIDE;
    QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const Q_DECL_OVERRIDE;
    QVariant taskData(const QModelIndex& index, int role = Qt::DisplayRole) const;
    QVariant logData(const QModelIndex& index, int role = Qt::DisplayRole) const;

    bool isActive(const QModelIndex& task) const;
    bool isCompletedWithin(const QModelIndex& task, int time) const;
    bool isTask(const QModelIndex& idx) const;

protected:
    QString assembleLog(const TaskInfo& task, const QString& prefix = QString()) const;
    QString singleLog(const TaskProgressLogMessage& msg, const QString& prefix = QString()) const;

    QList<TaskProgressAgentPrivate*> m_data; // active first, completed later
    size_t m_activeCount = 0;
};

class TaskProgressMonitorFilter : public QObject {
public:
    TaskProgressMonitorFilter(QObject* parent = 0);
    TaskProgressMonitorFilter(const QString& tag, QObject* parent = 0);
    TaskProgressMonitorFilter(const QStringList& tags, QObject* parent = 0);
    void addTag(const QString& tag);
    int count() const;
    TaskProgressAgent* at(int index) const;
    int indexOf(TaskProgressAgent* a) const;

    QAbstractItemModel* model() const;

private:
    TagsFilterProxyModel* m_proxy;
    TaskProgressModel* m_base;
};
}
#endif // TASKPROGRESS_H
