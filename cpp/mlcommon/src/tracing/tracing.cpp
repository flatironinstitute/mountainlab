#include "tracing/tracing.h"

#include <QCoreApplication>
#include <QFile>
#include <QFileInfo>
#include <QJsonDocument>
#include <QJsonObject>
#include <thread>
#include <QtDebug>

namespace Trace {
TracingSystem* TracingSystem::m_instance = nullptr;

EventManager::EventManager()
{
}

EventManager::~EventManager()
{
    flush();
}

void EventManager::flush()
{
    if (m_events.isEmpty())
        return;
    QVector<Event*> eventsToFlush;
    qSwap(eventsToFlush, m_events);
    //        // do flush
    //        // options:
    //        // 1. flush synchronously
    TracingSystem::instance()->flush(eventsToFlush);
    //        // 2. spawn a thread and flush there
    //        // 3. have a global per-pid thread that handles flush
}

void CompleteEvent::serialize(QJsonObject& doc) const
{
    DurationEvent::serialize(doc);
    doc.insert("dur", (qint64)m_dur.count());
}

void InstantEvent::serialize(QJsonObject& doc) const
{
    Event::serialize(doc);
    doc.insert("s", QString((char)m_s));
}

QString Event::name() const
{
    return m_name;
}

void Event::setName(const QString& name)
{
    m_name = name;
}

QString Event::cat() const
{
    return m_cat;
}

void Event::setCat(const QString& cat)
{
    m_cat = cat;
}

Event::Timestamp Event::ts() const
{
    return m_ts;
}

void Event::setTs(const Event::Timestamp& ts)
{
    m_ts = ts;
}

Event::Timestamp Event::tts() const
{
    return m_tts;
}

void Event::setTts(const Event::Timestamp& tts)
{
    m_tts = tts;
}

Event::Pid Event::pid() const
{
    return m_pid;
}

void Event::setPid(const Event::Pid& pid)
{
    m_pid = pid;
}

Event::Tid Event::tid() const
{
    return m_tid;
}

void Event::setTid(const Event::Tid& tid)
{
    m_tid = tid;
}

QVariantMap Event::args() const
{
    return m_args;
}

void Event::setArgs(const QVariantMap& args)
{
    m_args = args;
}

void Event::setArg(const QString& name, const QVariant& value)
{
    m_args.insert(name, value);
}

QString Event::cname() const
{
    return m_cname;
}

void Event::setCname(const QString& cname)
{
    m_cname = cname;
}

void Event::serialize(QJsonObject& json) const
{
    if (!name().isEmpty())
        json.insert("name", name());
    if (!cat().isEmpty())
        json.insert("cat", cat());
    json.insert("ph", QString(eventType()));
    json.insert("pid", (qint64)pid());
    json.insert("tid", (qint64)tid());
    if (ts() >= 0)
        json.insert("ts", (qint64)ts());
    if (!cname().isEmpty())
        json.insert("cname", cname());
    if (!args().isEmpty())
        json.insert("args", QJsonObject::fromVariantMap(args()));
}

void CounterEvent::serialize(QJsonObject& json) const
{
    Event::serialize(json);
    if (m_idSet)
        json.insert("id", id());
}

bool TracingSystem::isEnabled(const QString& category) const
{
    QReadLocker rlocker(&m_categoryLock);
    if (m_enabledCategories.contains(category))
        return true;
    if (m_disabledCategories.contains(category))
        return false;
    rlocker.unlock();
    if (checkCategoryPattern(category)) {
        QWriteLocker wlocker(&m_categoryLock);
        m_enabledCategories.insert(category);
        return true;
    }
    else {
        QWriteLocker wlocker(&m_categoryLock);
        m_disabledCategories.insert(category);
        m_categoryLock.unlock();
        return false;
    }
    return false;
}

void TracingSystem::setEnabled(const QStringList& patterns)
{
    m_patterns.clear();
    foreach (const QString& pattern, patterns) {
        m_patterns << QRegExp(pattern, Qt::CaseInsensitive, QRegExp::Wildcard);
    }
}

void TracingSystem::setEnabled(const QString& pattern)
{
    setEnabled(pattern.split(','));
    //    m_patterns.clear();
    //    m_patterns << QRegExp(pattern, Qt::CaseInsensitive, QRegExp::Wildcard);
}

bool TracingSystem::trace_categoryEnabled(const QString& category)
{
    if (!m_instance)
        return false;
    bool res = TracingSystem::instance()->isEnabled(category);
    return res;
}

void TracingSystem::trace_counter(const QString& category, const QString& name, QVector<QPair<QString, qreal> > series)
{
    TracingSystem* inst = TracingSystem::instance();
    if (!inst)
        return;
    high_resolution_clock::time_point t2 = high_resolution_clock::now();
    microseconds time_span = duration_cast<microseconds>(t2.time_since_epoch());
    qint64 ts = time_span.count();
    auto ctr = new CounterEvent(name, ts);
    ctr->setCat(category);
    for (auto serie : series)
        ctr->setValue(serie.first, serie.second);
    inst->eventManager()->append(ctr);
}

void TracingSystem::trace_begin(const QString& category, const QString& name, const ArgsVector& args)
{
    TracingSystem* inst = TracingSystem::instance();
    if (!inst)
        return;
    high_resolution_clock::time_point t2 = high_resolution_clock::now();
    microseconds time_span = duration_cast<microseconds>(t2.time_since_epoch());
    qint64 ts = time_span.count();
    auto dur = new DurationEventB(name, ts);
    dur->setCat(category);
    for (auto arg : args) {
        dur->setArg(arg.first, arg.second);
    }
    inst->eventManager()->append(dur);
}

void TracingSystem::trace_end(const ArgsVector& args)
{
    TracingSystem* inst = TracingSystem::instance();
    if (!inst)
        return;
    high_resolution_clock::time_point t2 = high_resolution_clock::now();
    microseconds time_span = duration_cast<microseconds>(t2.time_since_epoch());
    qint64 ts = time_span.count();
    auto dur = new DurationEventE(ts);
    for (auto arg : args) {
        dur->setArg(arg.first, arg.second);
    }
    inst->eventManager()->append(dur);
}

void TracingSystem::trace_end(const QString& category, const QString& name, const ArgsVector& args)
{
    TracingSystem* inst = TracingSystem::instance();
    if (!inst)
        return;
    high_resolution_clock::time_point t2 = high_resolution_clock::now();
    microseconds time_span = duration_cast<microseconds>(t2.time_since_epoch());
    qint64 ts = time_span.count();
    auto dur = new DurationEventE(ts);
    dur->setName(name);
    dur->setCat(category);
    for (auto arg : args) {
        dur->setArg(arg.first, arg.second);
    }
    inst->eventManager()->append(dur);
}

void TracingSystem::trace_threadname(const QString& name)
{
    TracingSystem* inst = TracingSystem::instance();
    if (!inst)
        return;
    if (!inst->isEnabled())
        return;
    auto meta = new MetadataEvent("thread_name");
    meta->setArg("name", name);
    inst->eventManager()->append(meta);
}

void TracingSystem::trace_processname(const QString& name)
{
    TracingSystem* inst = TracingSystem::instance();
    if (!inst)
        return;
    if (!inst->isEnabled())
        return;
    auto meta = new MetadataEvent("process_name");
    meta->setArg("name", name);
    inst->eventManager()->append(meta);
}

void TracingSystem::trace_instant(const QString& category, const QString& name, char scope)
{
    TracingSystem* inst = TracingSystem::instance();
    if (!inst)
        return;
    high_resolution_clock::time_point t2 = high_resolution_clock::now();
    microseconds time_span = duration_cast<microseconds>(t2.time_since_epoch());
    qint64 ts = time_span.count();
    auto instant = new InstantEvent(name, scope, ts);
    instant->setCat(category);
    inst->eventManager()->append(instant);
}

void TracingSystem::trace_instant(const QString& category, const QString& name, char scope, const TracingSystem::ArgsVector& args)
{
    TracingSystem* inst = TracingSystem::instance();
    if (!inst)
        return;
    high_resolution_clock::time_point t2 = high_resolution_clock::now();
    microseconds time_span = duration_cast<microseconds>(t2.time_since_epoch());
    qint64 ts = time_span.count();
    auto instant = new InstantEvent(name, scope, ts);
    instant->setCat(category);
    for (auto arg : args) {
        instant->setArg(arg.first, arg.second);
    }
    inst->eventManager()->append(instant);
}

void TracingSystem::init()
{
    QByteArray ba = qgetenv("TRACE_MASTER");
    if (ba.isEmpty()) {
        initMaster();
    }
    else {
        initSlave();
    }
}

void TracingSystem::initMaster()
{
    const QStringList& args = QCoreApplication::arguments();
    QString categoriesStr;
    for (int i = 1; i < args.size(); ++i) {
        if (args[i] == "--trace-enabled") {
            m_enabled = true;
            continue;
        }
        if (args[i].startsWith("--trace-file=")) {
            m_traceFilePath = args[i].mid(13);
            continue;
        }
        if (args[i] == "--trace-file") {
            if (i + 1 < args.size()) {
                m_traceFilePath = args[++i];
            }
            continue;
        }
        if (args[i].startsWith("--trace-categories=")) {
            categoriesStr = args[i].mid(19);
            continue;
        }
        if (args[i] == "--trace-categories") {
            if (i + 1 < args.size()) {
                categoriesStr = args[++i];
            }
            continue;
        }
    }
    if (!isEnabled()) {
        if (qgetenv("TRACE_ENABLED").toInt() > 0) {
            m_enabled = true;
            if (categoriesStr.isEmpty())
                categoriesStr = qgetenv("TRACE_CATEGORIES");
        }
    }
    if (!isEnabled())
        return;
    if (m_traceFilePath.isEmpty() && !qgetenv("TRACE_FILE").isEmpty())
        m_traceFilePath = qgetenv("TRACE_FILE");
    if (m_traceFilePath.isEmpty() && !QCoreApplication::applicationName().isEmpty()) {
        m_traceFilePath = QCoreApplication::applicationName() + ".trace";
    }
    if (m_traceFilePath.isEmpty()) {
        m_traceFilePath = "application.trace";
    }
    m_traceFilePath = QFileInfo(m_traceFilePath).absoluteFilePath();

    // tell slaves about tracing
    qputenv("TRACE_MASTER", QByteArray::number(QCoreApplication::applicationPid()));
    qputenv("TRACE_FILE", m_traceFilePath.toLocal8Bit());
    qputenv("TRACE_CATEGORIES", categoriesStr.toLocal8Bit());
    setEnabled(categoriesStr);

    LockedFile file(m_traceFilePath);
    file.lock(LockedFile::ExclusiveLock);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate | QIODevice::Text)) {
        qCritical("Device failed to open");
    }
    else {
        file.write("[\n");
        file.close();
    }
    file.unlock();
}

void TracingSystem::initSlave()
{
    m_traceFilePath = qgetenv("TRACE_FILE");
    QString categoriesStr = qgetenv("TRACE_CATEGORIES");
    if (!categoriesStr.isEmpty())
        setEnabled(categoriesStr);
}

void TracingSystem::doFlush(const QVector<Event*>& events)
{
    QMutexLocker locker(&m_mutex);
    LockedFile file(m_traceFilePath);
    file.lock(LockedFile::ExclusiveLock);
    if (!file.open(QIODevice::Append | QIODevice::Text)) {
        qCritical("Device failed to open in append mode");
    }
    QTextStream stream(&file);
    foreach (Event* e, events) {
        QJsonObject obj;
        e->serialize(obj);
        stream << "  " << QJsonDocument(obj).toJson(QJsonDocument::Compact) << ',' << endl;
    }
    file.close();
    file.unlock();
}

void TracingSystem::flush(QVector<Event*> events)
{
    doFlush(events);
    qDeleteAll(events);
}

bool TracingSystem::checkCategoryPattern(const QString& categoryName) const
{
    if (!isEnabled())
        return false;
    if (m_patterns.isEmpty())
        return true;
    foreach (const QRegExp& rx, m_patterns) {
        if (rx.exactMatch(categoryName))
            return true;
    }
    return false;
}

bool TracingSystem::isEnabled() const
{
    return m_enabled;
}
}
