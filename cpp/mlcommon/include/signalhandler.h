#ifndef SIGNALHANDLER_H
#define SIGNALHANDLER_H

#include <QMap>
#include <QObject>
#include <signal.h>
#include <functional>
#include <QVector>
#include <QSet>

class Handler;
class SignalHandler : public QObject {
    Q_OBJECT
public:
    using Closure = std::function<void(void)>;
    enum Signal {
        SigNone = 0,
        SigHangUp = 0x1,
        SigInterrupt = 0x2,
        SigQuit = 0x4,
        SigAbort = 0x8,
        SigFloatingPoint = 0x10,
        SigKill = 0x20,
        SigUser1 = 0x40,
        SigSegmentationFault = 0x80,
        SigUser2 = 0x100,
        SigBrokenPipe = 0x200,
        SigAlarm = 0x400,
        SigTermination = 0x800,
        SigContinue = 0x1000,
        SigStop = 0x2000,
        SigPowerFailure = 0x4000

    };
    Q_DECLARE_FLAGS(Signals, Signal)
    explicit SignalHandler(QObject* parent = 0);
    ~SignalHandler();

    size_t installHandler(int sig, QObject*, const char*, int priority = 0);
    size_t installHandler(int sig, Closure, int priority = 0);

    size_t installHandler(Signals, QObject*, const char*, int priority = 0);
    size_t installHandler(Signals, Closure, int priority = 0);

    void uninstallHandler(size_t id);

    static Signal flagForSignal(int);

signals:
    void signalled(int);
public slots:
    void pause();
    void resume();

protected:
    void sigaction_set(QSet<int> signum);
    void sigaction_set(Signals signum);
    void sigaction_unset(QSet<int> signum);
    void sigaction_unset(Signals signum);
    void execute(int signum);

private:
    static void __handler(int);
    QVector<Handler*> m_handlers;
    QMap<int, int> m_installedHandlers;
    QMap<int, struct sigaction> m_oldHandlers;
    size_t m_id = 0;
};

Q_DECLARE_OPERATORS_FOR_FLAGS(SignalHandler::Signals)

#endif // SIGNALHANDLER_H
