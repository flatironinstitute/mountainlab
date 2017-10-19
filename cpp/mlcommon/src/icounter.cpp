#include "icounter.h"
#include <QHash>
#include <objectregistry.h>

/*!
 * \class ICounterBase
 * \brief Base class for all counters
 *
 */

/*!
 * \brief ICounterBase::label
 * \return
 */
QString ICounterBase::label() const
{
    return genericValue().toString();
}

/*!
 * \brief IAggregateCounter::add
 * \return
 */
QVariant IAggregateCounter::add(const QVariant&)
{
    return genericValue();
}

/*!
 * \brief IAggregateCounter::counters
 * \return
 */
QList<ICounterBase*> IAggregateCounter::counters() const
{
    return m_counters;
}

/*!
 * \brief IAggregateCounter::addCounter
 * \param c
 */
void IAggregateCounter::addCounter(ICounterBase* c)
{
    m_counters << c;
    connect(c, SIGNAL(valueChanged()), this, SLOT(updateValue()));
    updateValue();
}

/*!
 * \brief IAggregateCounter::addCounters
 * \param list
 */
void IAggregateCounter::addCounters(const QList<ICounterBase*>& list)
{
    foreach (ICounterBase* c, list) {
        m_counters << c;
        connect(c, SIGNAL(valueChanged()), this, SLOT(updateValue()));
    }
    updateValue();
}

/*!
 * \brief ICounterManager::ICounterManager
 * \param parent
 */
ICounterManager::ICounterManager(QObject* parent)
    : QObject(parent)
{
}

/*!
 * \brief CounterManager::CounterManager
 * \param parent
 * \internal
 */
CounterManager::CounterManager(QObject* parent)
    : ICounterManager(parent)
{
}

QStringList CounterManager::availableCounters() const
{
    QReadLocker locker(&m_countersLock);
    return m_counterNames;
}

ICounterBase* CounterManager::counter(const QString& name) const
{
    QReadLocker locker(&m_countersLock);
    return m_counters.value(name, nullptr);
}

void CounterManager::setCounters(QList<ICounterBase*> counters)
{
    QWriteLocker locker(&m_countersLock);
    for (ICounterBase* counter : counters) {
        const QString& name = counter->name();
        m_counters.insert(name, counter);
        m_counterNames.append(name);
    }
    locker.unlock();
    emit countersChanged();
}

void CounterManager::addCounter(ICounterBase* counter)
{
    const QString& name = counter->name();
    QWriteLocker locker(&m_countersLock);
    if (m_counterNames.contains(name))
        return;
    m_counters.insert(name, counter);
    m_counterNames.append(name);
    locker.unlock();
    emit countersChanged();
}

void CounterManager::removeCounter(ICounterBase* counter)
{
    const QString& name = counter->name();
    QWriteLocker locker(&m_countersLock);
    if (!m_counterNames.removeOne(name))
        return;
    m_counters.remove(name);
    locker.unlock();
    emit countersChanged();
}

void CounterManager::addGroup(CounterGroup* group)
{
    QWriteLocker locker(&m_groupsLock);
    if (m_groupNames.contains(group->name()))
        return;
    m_groups.insert(group->name(), group);
    m_groupNames.append(group->name());
    locker.unlock();
    emit groupsChanged();
}

void CounterManager::removeGroup(CounterGroup* group)
{
    QWriteLocker locker(&m_groupsLock);
    if (!m_groupNames.removeOne(group->name()))
        return;
    m_groups.remove(group->name());
    locker.unlock();
    emit groupsChanged();
}

CounterGroup* CounterManager::group(const QString& name) const
{
    QReadLocker locker(&m_groupsLock);
    return m_groups.value(name, nullptr);
}

QStringList CounterManager::availableGroups() const
{
    QReadLocker locker(&m_groupsLock);
    return m_groupNames;
}

/*!
 * \brief ICounterBase::ICounterBase
 * \param name
 * \param parent
 */
ICounterBase::ICounterBase(const QString& name, QObject* parent)
    : QObject(parent)
    , m_name(name)
{
}

/*!
 * \brief ICounterBase::name
 * \return
 */
QString ICounterBase::name() const
{
    return m_name;
}

/*!
 * \brief CounterProxy::CounterProxy
 * \param name
 * \param parent
 */
CounterProxy::CounterProxy(const QString& name, QObject* parent)
    : ICounterBase(name, parent)
{
}

/*!
 * \brief CounterProxy::CounterProxy
 * \param name
 * \param c
 * \param parent
 */
CounterProxy::CounterProxy(const QString& name, ICounterBase* c, QObject* parent)
    : ICounterBase(name, parent)
{
    setBaseCounter(c);
}

/*!
 * \brief CounterProxy::setBaseCounter
 * \param c
 */
void CounterProxy::setBaseCounter(ICounterBase* c)
{
    if (m_base) {
        disconnect(c, SIGNAL(valueChanged()), this, SLOT(updateValue()));
    }
    m_base = c;
    if (c) {
        connect(c, SIGNAL(valueChanged()), this, SLOT(updateValue()));
    }
}

/*!
 * \brief CounterProxy::baseCounter
 * \return
 */
ICounterBase* CounterProxy::baseCounter() const { return m_base; }

/*!
 * \brief CounterProxy::type
 * \return
 */
ICounterBase::Type CounterProxy::type() const
{
    return baseCounter() ? baseCounter()->type() : Unknown;
}

/*!
 * \brief CounterProxy::label
 * \return
 */
QString CounterProxy::label() const
{
    return baseCounter() ? baseCounter()->label() : QString();
}

/*!
 * \brief CounterProxy::genericValue
 * \return
 */
QVariant CounterProxy::genericValue() const
{
    return baseCounter() ? baseCounter()->genericValue() : QVariant();
}

/*!
 * \brief CounterProxy::add
 * \param v
 * \return
 */
QVariant CounterProxy::add(const QVariant& v)
{
    if (!baseCounter())
        return 0;
    return baseCounter()->add(v);
}

/*!
 * \brief CounterProxy::updateValue
 */
void CounterProxy::updateValue()
{
    emit valueChanged();
}
