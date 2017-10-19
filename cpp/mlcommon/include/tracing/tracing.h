#ifndef TRACING_H
#define TRACING_H

#include <QCoreApplication>
#include <QFile>
#include <QMutex>
#include <QString>
#include <QTextStream>
#include <QThreadStorage>
#include <QVector>
#include <chrono>
#include <sys/file.h>
#include <thread>
#include <QtDebug>
#include <QReadWriteLock>

using namespace std::chrono;

namespace Trace {

/*
{
  "name": "myName",
  "cat": "category,list",
  "ph": "B",
  "ts": 12345,
  "pid": 123,
  "tid": 456,
  "args": {
    "someArg": 1,
    "anotherArg": {
      "value": "my value"
    }
  }
}
*/

class Event {
public:
    typedef uint32_t Pid;
    typedef uint32_t Tid;
    typedef int64_t Timestamp;
    Event(Timestamp ts, Tid tid = 0, Pid pid = 0)
        : m_pid(pid)
        , m_tid(tid)
        , m_ts(ts)
    {
        if (m_pid == 0)
            m_pid = QCoreApplication::applicationPid();
        if (m_tid == 0) {
            static std::hash<std::thread::id> hasher;
            m_tid = hasher(std::this_thread::get_id());
        }
    }
    virtual ~Event() {}
    virtual char eventType() const = 0;
    QString name() const;
    void setName(const QString& name);

    QString cat() const;
    void setCat(const QString& cat);

    Timestamp ts() const;
    void setTs(const Timestamp& ts);

    Timestamp tts() const;
    void setTts(const Timestamp& tts);

    Pid pid() const;
    void setPid(const Pid& pid);

    Tid tid() const;
    void setTid(const Tid& tid);

    QVariantMap args() const;
    void setArgs(const QVariantMap& args);
    void setArg(const QString& name, const QVariant& value);

    QString cname() const;
    void setCname(const QString& cname);

    virtual void serialize(QJsonObject& doc) const;

protected:
    QVariantMap& argsRef() { return m_args; }

private:
    QString m_name;
    QString m_cat;
    Pid m_pid;
    Tid m_tid;
    Timestamp m_ts;
    Timestamp m_tts;
    QVariantMap m_args;
    QString m_cname;
};

class DurationEvent : public Event {
public:
    DurationEvent(Timestamp ts, Tid tid = 0, Pid pid = 0)
        : Event(ts, tid, pid)
    {
    }
};

class DurationEventB : public DurationEvent {
public:
    DurationEventB(const QString& name, Timestamp ts, Tid tid = 0, Pid pid = 0)
        : DurationEvent(ts, tid, pid)
    {
        setName(name);
    }
    virtual char eventType() const override { return 'B'; }
};

class DurationEventE : public DurationEvent {
public:
    DurationEventE(Timestamp ts, Tid tid = 0, Pid pid = 0)
        : DurationEvent(ts, tid, pid)
    {
    }
    virtual char eventType() const override { return 'E'; }
};

class CompleteEvent : public DurationEvent {
public:
    CompleteEvent(Timestamp ts, std::chrono::microseconds dur, Tid tid = 0, Pid pid = 0)
        : DurationEvent(ts, tid, pid)
        , m_dur(dur)
    {
    }
    CompleteEvent(Timestamp ts, qint64 dur, Tid tid = 0, Pid pid = 0)
        : DurationEvent(ts, tid, pid)
        , m_dur(dur)
    {
    }
    virtual char eventType() const override { return 'X'; }

private:
    std::chrono::microseconds m_dur;

    // Event interface
public:
    virtual void serialize(QJsonObject& doc) const override;
};

class InstantEvent : public Event {
public:
    enum Scope { Thread = 't',
        Process = 'p',
        Global = 'g' };
    InstantEvent(const QString& name, Timestamp ts, Tid tid = 0, Pid pid = 0)
        : Event(ts, tid, pid)
    {
        setName(name);
        m_s = Thread;
    }
    InstantEvent(const QString& name, Scope s, Timestamp ts, Tid tid = 0, Pid pid = 0)
        : Event(ts, tid, pid)
    {
        setName(name);
        m_s = s;
    }
    InstantEvent(const QString& name, char s, Timestamp ts, Tid tid = 0, Pid pid = 0)
        : Event(ts, tid, pid)
    {
        setName(name);
        m_s = (Scope)s;
    }

    virtual char eventType() const override { return 'i'; }
    virtual void serialize(QJsonObject& doc) const override;

private:
    Scope m_s;
};

class CounterEvent : public Event {
public:
    CounterEvent(const QString& name, Timestamp ts, Tid tid = 0, Pid pid = 0)
        : Event(ts, tid, pid)
    {
        setName(name);
    }
    void setValue(const QString& series, QVariant value)
    {
        argsRef().insert(series, value);
    }
    QVariant value(const QString& series) const
    {
        return args().value(series, QVariant());
    }
    void setId(int id)
    {
        m_id = id;
        m_idSet = true;
    }
    int id() const { return m_id; }

    virtual char eventType() const override { return 'C'; }
    void serialize(QJsonObject& json) const;

private:
    int m_id = 0;
    bool m_idSet = false;
};

class MetadataEvent : public Event {
public:
    MetadataEvent(const QString& name, Tid tid = 0, Pid pid = 0)
        : Event(-1, tid, pid)
    {
        setName(name);
    }

    virtual char eventType() const override { return 'M'; }
};

class EventManager {
public:
    EventManager();
    ~EventManager();
    void append(Event* e)
    {
        m_events << e;
        if (m_events.size() > 100)
            flush();
    }
    void flush();

private:
    QVector<Event*> m_events;
};

class TracingSystem {
public:
    using ArgsVector = QVector<QPair<QString, QVariant> >;
    TracingSystem()
    {
        if (m_instance) {
            qWarning("TracingSystem instance already present");
            return;
        }
        m_instance = this;
        init();
    }
    ~TracingSystem()
    {
        if (m_manager.hasLocalData()) {
            m_manager.setLocalData(nullptr);
        }
        if (m_instance == this)
            m_instance = nullptr;
    }
    EventManager* eventManager()
    {
        if (!m_manager.hasLocalData())
            m_manager.setLocalData(new EventManager);
        return m_manager.localData();
    }

    bool isEnabled(const QString& category) const;
    void setEnabled(const QStringList& patterns);
    void setEnabled(const QString& pattern);
    static TracingSystem* instance() { return m_instance; }
    static bool trace_categoryEnabled(const QString& category);
    static void trace_counter(const QString& category, const QString& name, QVector<QPair<QString, qreal> > series);
    static void trace_begin(const QString& category, const QString& name, const ArgsVector& args = {});
    static void trace_end(const ArgsVector& args = {});
    static void trace_end(const QString& category, const QString& name, const ArgsVector& args = {});
    static void trace_threadname(const QString& name);
    static void trace_processname(const QString& name);
    static void trace_instant(const QString& category, const QString& name, char scope = 't');
    static void trace_instant(const QString& category, const QString& name, char scope, const ArgsVector& args);

    class Scope {
    public:
        Scope() {}
        ~Scope()
        {
            if (m_init) {
                Trace::TracingSystem::trace_end(category, name);
            }
        }

        void init(const QString& cat, const QString& nam)
        {
            m_init = true;
            category = cat;
            name = nam;
        }

    private:
        bool m_init = false;
        QString name;
        QString category;
    };

    bool isEnabled() const;

protected:
    void init();
    void initMaster();
    void initSlave();
    void doFlush(const QVector<Event*>& events);
    void flush(QVector<Event*> events);
    bool checkCategoryPattern(const QString& categoryName) const;

private:
    bool m_enabled = false;
    QString m_traceFilePath;
    static TracingSystem* m_instance;
    QThreadStorage<EventManager*> m_manager;
    QMutex m_mutex;
    QVector<QRegExp> m_patterns;
    mutable QSet<QString> m_enabledCategories;
    mutable QSet<QString> m_disabledCategories;
    mutable QReadWriteLock m_categoryLock;
    friend class EventManager;
};

} // namespace Trace

class LockedFile : public QFile {
public:
    LockedFile()
        : QFile()
    {
    }
    LockedFile(const QString& filePath)
        : QFile(filePath)
    {
    }
    enum LockType {
        SharedLock,
        ExclusiveLock
    };

    void lock(LockType lt)
    {
        flock(handle(), lt == SharedLock ? LOCK_SH : LOCK_EX);
    }
    void unlock()
    {
        flock(handle(), LOCK_UN);
    }
};

#define RANDOM_VARIABLE3(p, q) \
    trace_tracing_system_##p##q
#define RANDOM_VARIABLE2(p, q) \
    RANDOM_VARIABLE3(p, q)
#define RANDOM_VARIABLE(prefix) \
    RANDOM_VARIABLE2(prefix, __LINE__)

#define TRACE_EVENT_BEGIN0(category, name) \
    INTERNAL_TRACE_EVENT_ADD(begin, category, name, {})
#define TRACE_EVENT_BEGIN1(category, name, arg1, val1) \
    INTERNAL_TRACE_EVENT_ADD(begin, category, name, { { arg1, val1 } })
#define TRACE_EVENT_BEGIN2(category, name, arg1, val1, arg2, val2) \
    INTERNAL_TRACE_EVENT_ADD(begin, category, name, { { arg1, val1 }, { arg2, val2 } })

#define TRACE_EVENT_END0(category, name) \
    INTERNAL_TRACE_EVENT_ADD(end, category, name, {})
#define TRACE_EVENT_END1(category, name, arg1, val1) \
    INTERNAL_TRACE_EVENT_ADD(end, category, name, { { arg1, val1 } })
#define TRACE_EVENT_END2(category, name, arg1, val1, arg2, val2) \
    INTERNAL_TRACE_EVENT_ADD(end, category, name, { { arg1, val1 }, { arg2, val2 } })

#define TRACE_EVENT_COUNTER1(category, name, value) \
    INTERNAL_TRACE_EVENT_ADD(counter, category, name, { { "value", value } })

#define TRACE_EVENT_COUNTER2(category, name, val1nam, val1val, val2nam, val2val) \
    INTERNAL_TRACE_EVENT_ADD(counter, category, name, { { val1nam, val1val }, { val2nam, val2val } })

#define TRACE_EVENT_INSTANT0(category, name) \
    INTERNAL_TRACE_EVENT_ADD(instant, category, name, 't')

#define TRACE_EVENT_INSTANT1(category, name, arg1, val1) \
    INTERNAL_TRACE_EVENT_ADD(instant, category, name, 't', { { arg1, val1 } })
#define TRACE_EVENT_INSTANT2(category, name, arg1, val1, arg2, val2) \
    INTERNAL_TRACE_EVENT_ADD(instant, category, name, 't', { { arg1, val1 }, { arg2, val2 } })

#define TRACE_EVENT0(category, name) \
    INTERNAL_TRACE_EVENT_ADD_SCOPE(category, name, {})

#define TRACE_EVENT1(category, name, arg1, val1) \
    INTERNAL_TRACE_EVENT_ADD_SCOPE(category, name, { { arg1, val1 } })

#define TRACE_EVENT2(category, name, arg1, val1, arg2, val2) \
    INTERNAL_TRACE_EVENT_ADD_SCOPE(category, name, { { arg1, val1 }, { arg2, val2 } })

#define INTERNAL_TRACE_EVENT_ADD(type, category, name, ...)                    \
    do {                                                                       \
        if (Trace::TracingSystem::trace_categoryEnabled(category)) {           \
            Trace::TracingSystem::trace_##type(category, name, ##__VA_ARGS__); \
        }                                                                      \
    } while (0)

#define INTERNAL_TRACE_EVENT_ADD_SCOPE(category, name, ...)               \
    Trace::TracingSystem::Scope RANDOM_VARIABLE(traceScope);              \
    if (Trace::TracingSystem::trace_categoryEnabled(category)) {          \
        Trace::TracingSystem::trace_begin(category, name, ##__VA_ARGS__); \
        RANDOM_VARIABLE(traceScope).init(category, name);                 \
    }

#if 0
#define TRACE_COUNTER1(name, series, value) Tracing::instance()->writeCounter(name, series, value);

#define TRACE_COUNTER(name, series, value) Tracing::instance()->writeCounter(name, series, value);
#define TRACE_BEGIN(name, args, cats) Tracing::instance()->writeBegin(name, args, cats);
#define TRACE_END(name, args, cats) Tracing::instance()->writeEnd(name, args, cats);

#define TRACE_BEGIN0(name, cats) Tracing::instance()->writeBegin(#name, QVariantMap(), cats);
#define TRACE_END0(name, cats) Tracing::instance()->writeEnd(#name, QVariantMap(), cats);

#define TRACE_SCOPE0(name, cat) TracingScope scope_##name(#name, QVariantMap(), cat);
#define TRACE_SCOPE1(name, key, val, cat) TracingScope scope_##name(#name, { { key, val } }, cat);
#define TRACE_SCOPE2(name, key1, val1, key2, val2, cat) TracingScope scope_##name(#name, { { key1, val1 }, { key2, val2 } }, cat);
#define TRACE_SCOPE3(name, key1, val1, key2, val2, key3, val3, cat) TracingScope scope_##name(#name, { { key1, val1 }, { key2, val2 }, { key3, val3 } }, cat);
#endif
#endif // TRACING_H
