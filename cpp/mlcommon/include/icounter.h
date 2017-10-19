#ifndef ICOUNTER_H
#define ICOUNTER_H

#include <QObject>
#include <QReadWriteLock>
#include <QVariant>
#include <QHash>

#include "mlcommon.h"

class ICounterBase : public QObject {
    Q_OBJECT
public:
    enum Type {
        Unknown,
        Integer,
        Double,
        Variant
    };
    Q_ENUM(Type)
    ICounterBase(const QString& name, QObject* parent = 0);
    virtual Type type() const = 0;
    virtual QString name() const;
    virtual QString label() const;
    virtual QVariant genericValue() const = 0;
    virtual QVariant add(const QVariant&) = 0;
    template <typename T>
    T value() const { return genericValue().value<T>(); }
signals:
    void valueChanged();

private:
    QString m_name;
};

class IAggregateCounter : public ICounterBase {
    Q_OBJECT
public:
    using ICounterBase::ICounterBase;
    QVariant add(const QVariant&);
    QList<ICounterBase*> counters() const;
    void addCounter(ICounterBase*);
    void addCounters(const QList<ICounterBase*>&);
protected slots:
    virtual void updateValue() = 0;

private:
    QList<ICounterBase*> m_counters;
};

template <typename T, bool integral = false>
class ICounterImpl : public ICounterBase {
public:
    using ICounterBase::ICounterBase;
    Type type() const { return Unknown; }
    QVariant add(const QVariant& inc)
    {
        return QVariant::fromValue<T>(add(inc.value<T>()));
    }

    T add(const T& inc)
    {
        m_lock.lockForWrite();
        T old = m_value;
        m_value += inc;
        m_lock.unlock();
        if (m_value != old)
            emit valueChanged();
        return m_value;
    }
    T value() const
    {
        QReadLocker locker(&m_lock);
        return m_value;
    }
    QVariant genericValue() const
    {
        return QVariant::fromValue<T>(value());
    }

private:
    mutable QReadWriteLock m_lock;
    T m_value = T();
};

template <typename T>
class ICounterImpl<T, true> : public ICounterBase {
public:
    using ICounterBase::ICounterBase;
    Type type() const { return Integer; }
    QVariant add(const QVariant& inc)
    {
        return QVariant::fromValue<T>(add(inc.value<T>()));
    }
    T add(const T& inc)
    {
        if (inc == 0)
            return m_value;
        T val = (m_value += inc);
        emit valueChanged();
        return val;
    }
    T value() const
    {
        return m_value;
    }
    QVariant genericValue() const
    {
        return QVariant::fromValue<T>(value());
    }

private:
    QAtomicInteger<T> m_value;
};

template <typename T>
using ICounter = ICounterImpl<T, std::is_integral<T>::value>;

using IIntCounter = ICounter<int>;
using IDoubleCounter = ICounter<double>;

class CounterGroup : public QObject {
    Q_OBJECT
public:
    CounterGroup(const QString& n, QObject* parent = 0)
        : QObject(parent)
        , m_name(n)
    {
    }
    void setName(const QString& n) { m_name = n; }
    QString name() const { return m_name; }
    virtual QStringList availableCounters() const = 0;
    virtual ICounterBase* counter(const QString& name) const = 0;

private:
    QString m_name;
};

class ICounterManager : public QObject {
    Q_OBJECT
public:
    virtual QStringList availableCounters() const = 0;
    virtual ICounterBase* counter(const QString& name) const = 0;

    virtual QStringList availableGroups() const = 0;
    virtual CounterGroup* group(const QString& name) const = 0;
signals:
    void countersChanged();
    void groupsChanged();

protected:
    ICounterManager(QObject* parent = 0);
};

class CounterManager : public ICounterManager {
public:
    CounterManager(QObject* parent = 0);

    QStringList availableCounters() const override;
    ICounterBase* counter(const QString& name) const override;
    void setCounters(QList<ICounterBase*> counters);
    void addCounter(ICounterBase* counter);
    void removeCounter(ICounterBase* counter);

    QStringList availableGroups() const;
    CounterGroup* group(const QString& name) const;
    void addGroup(CounterGroup* group);
    void removeGroup(CounterGroup* group);

private:
    QHash<QString, ICounterBase*> m_counters;
    QStringList m_counterNames;
    mutable QReadWriteLock m_countersLock;

    QHash<QString, CounterGroup*> m_groups;
    QStringList m_groupNames;
    mutable QReadWriteLock m_groupsLock;
};

class CounterProxy : public ICounterBase {
    Q_OBJECT
public:
    CounterProxy(const QString& name, QObject* parent = 0);
    CounterProxy(const QString& name, ICounterBase* c, QObject* parent = 0);
    void setBaseCounter(ICounterBase* c);
    ICounterBase* baseCounter() const;
    virtual Type type() const;
    virtual QString label() const;
    virtual QVariant genericValue() const;
    virtual QVariant add(const QVariant& v);
protected slots:
    virtual void updateValue();

private:
    ICounterBase* m_base = nullptr;
};

//template<typename T>
class SimpleAggregateCounter : public IAggregateCounter {
public:
    enum Operator {
        None,
        Add,
        Subtract,
        Multiply,
        Divide,
        Mean
    };
    SimpleAggregateCounter(const QString& name, Operator op, ICounterBase* c1, ICounterBase* c2)
        : SimpleAggregateCounter(name, op, { c1, c2 })
    {
    }

    SimpleAggregateCounter(const QString& name, Operator op, QList<ICounterBase*>& counters)
        : IAggregateCounter(name)
        , m_operator(op)
    {
        foreach (ICounterBase* cntr, counters) {
            addCounter(cntr);
        }
    }

    SimpleAggregateCounter(const QString& name, Operator op, const std::initializer_list<ICounterBase*>& counters)
        : IAggregateCounter(name)
        , m_operator(op)
    {
        foreach (ICounterBase* cntr, counters) {
            addCounter(cntr);
        }
    }

    Type type() const { return Integer; }
    QVariant genericValue() const { return QVariant::fromValue<int64_t>(value()); }
    int64_t value() const { return m_value; }
    Operator operatorType() const { return m_operator; }

protected:
    void updateValue()
    {
        m_value = calculate(operatorType());
        emit valueChanged();
    }
    int64_t calculate(Operator op) const
    {
        if (counters().empty())
            return 0;
        int64_t val = 0;
        switch (op) {
        case None:
            return 0;
        case Add: {
            foreach (ICounterBase* cntr, counters())
                val += cntr->value<int64_t>();
            return val;
        }
        case Subtract: {
            val = counters().first()->value<int64_t>();
            for (int i = 1; i < counters().size(); ++i)
                val -= counters().at(i)->value<int64_t>();
            return val;
        }
        case Multiply: {
            val = 1;
            foreach (ICounterBase* cntr, counters())
                val *= cntr->value<int64_t>();
            return val;
        }
        case Divide: {
            val = counters().first()->value<int64_t>();
            for (int i = 1; i < counters().size(); ++i) {
                int64_t cval = counters().at(i)->value<int64_t>();
                if (cval == 0)
                    continue;
                val /= cval;
            }
            return val;
        }
        case Mean:
            return calculate(Add) / counters().size();
        }
        return 0;
    }

private:
    int64_t m_value = 0;
    Operator m_operator;
};

#endif // ICOUNTER_H
