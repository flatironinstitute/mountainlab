#include "signalhandler.h"
#include <QMetaMethod>
#include <QSet>
#include <QtDebug>
#include <functional>

// For mac compilation
#ifndef _NSIG
#define _NSIG NSIG
#endif

static SignalHandler* slot_handler_instance = nullptr;

struct Handler {
    virtual ~Handler() {}
    size_t id;
    int priority = 0;
    SignalHandler::Signals set;
    virtual void execute() = 0;
};

struct SlotHandler : public Handler {
    QObject* receiver;
    const char* member;
    void execute()
    {
        if (member && member[0] == '1') {
            int methodIndex = receiver->metaObject()->indexOfMethod(member + 1);
            QMetaMethod method = receiver->metaObject()->method(methodIndex);
            method.invoke(receiver, Qt::DirectConnection);
        }
        else if (member && member[0] == '2') {
            qWarning("Emitting signals from SignalHandler is not supported");
        }
    }
};

struct FunctionHandler : public Handler {
    std::function<void(void)> callback;
    void execute()
    {
        if (callback)
            callback();
    }
};

SignalHandler::SignalHandler(QObject* parent)
    : QObject(parent)
{
    if (!slot_handler_instance)
        slot_handler_instance = this;
}

SignalHandler::~SignalHandler()
{
    qDeleteAll(m_handlers);
    if (slot_handler_instance == this)
        slot_handler_instance = nullptr;
}

bool HandlerCompare(Handler* h1, Handler* h2)
{
    if (h1->priority > h2->priority)
        return true;
    return false;
}

size_t SignalHandler::installHandler(int sig, QObject* receiver, const char* member, int priority)
{
    SlotHandler* sh = new SlotHandler;
    sh->receiver = receiver;
    sh->member = member;
    sh->priority = priority;
    sh->id = m_id++;

    sh->set = flagForSignal(sig);

    auto iter = qUpperBound(m_handlers.begin(), m_handlers.end(), sh, HandlerCompare);
    m_handlers.insert(iter, sh);
    //    if (!m_installedHandlers.contains(sig)) {
    sigaction_set(QSet<int>({ sig }));
    //    }
    return sh->id;
}

size_t SignalHandler::installHandler(int sig, SignalHandler::Closure cl, int priority)
{
    FunctionHandler* fh = new FunctionHandler;
    fh->callback = cl;
    fh->priority = priority;
    fh->set = flagForSignal(sig);
    fh->id = m_id++;

    auto iter = qUpperBound(m_handlers.begin(), m_handlers.end(), fh, HandlerCompare);
    m_handlers.insert(iter, fh);
    //    if (!m_installedHandlers.contains(sig)) {
    sigaction_set(QSet<int>({ sig }));
    //    }
    return fh->id;
}

size_t SignalHandler::installHandler(Signals set, QObject* receiver, const char* member, int priority)
{
    SlotHandler* sh = new SlotHandler;
    sh->receiver = receiver;
    sh->member = member;
    sh->priority = priority;
    sh->set = set;
    sh->id = m_id++;

    auto iter = qUpperBound(m_handlers.begin(), m_handlers.end(), sh, HandlerCompare);
    m_handlers.insert(iter, sh);
    sigaction_set(set);
    return sh->id;
}

size_t SignalHandler::installHandler(Signals set, SignalHandler::Closure cl, int priority)
{
    FunctionHandler* fh = new FunctionHandler;
    fh->callback = cl;
    fh->priority = priority;
    fh->set = set;
    fh->id = m_id++;

    auto iter = qUpperBound(m_handlers.begin(), m_handlers.end(), fh, HandlerCompare);
    m_handlers.insert(iter, fh);
    sigaction_set(set);
    return fh->id;
}

void SignalHandler::uninstallHandler(size_t id)
{
    for (int i = 0; i < m_handlers.size(); ++i) {
        if (m_handlers.at(i)->id == id) {
            Signals sigset = m_handlers.at(i)->set;
            delete m_handlers.at(i);
            m_handlers.removeAt(i);
            sigaction_unset(sigset);
            return;
        }
    }
}

SignalHandler::Signal SignalHandler::flagForSignal(int signum)
{
    switch (signum) {
    case SIGHUP:
        return SigHangUp;
    case SIGINT:
        return SigInterrupt;
    case SIGQUIT:
        return SigQuit;
    case SIGABRT:
        return SigAbort;
    case SIGFPE:
        return SigFloatingPoint;
    case SIGKILL:
        return SigKill;
    case SIGUSR1:
        return SigUser1;
    case SIGSEGV:
        return SigSegmentationFault;
    case SIGUSR2:
        return SigUser2;
    case SIGPIPE:
        return SigBrokenPipe;
    case SIGALRM:
        return SigAlarm;
    case SIGTERM:
        return SigTermination;
    case SIGCONT:
        return SigContinue;
    case SIGSTOP:
        return SigStop;
        //case SIGPWR: //apparently not supported in mac
        //    return SigPowerFailure;
    }
    return SigNone;
}

void SignalHandler::pause()
{
    /// FIXME: Not implemented
}

void SignalHandler::resume()
{
    /// FIXME: Not implemented
}

void SignalHandler::sigaction_set(QSet<int> signumset)
{
    struct sigaction new_action, old_action;

    new_action.sa_handler = SignalHandler::__handler;
    new_action.sa_flags = 0;
    sigemptyset(&new_action.sa_mask);

    foreach (int signum, signumset) {
        if (m_installedHandlers.contains(signum)) {
            m_installedHandlers[signum]++;
        }
        else {
            sigaction(signum, &new_action, &old_action);
            m_oldHandlers.insert(signum, old_action);
            m_installedHandlers.insert(signum, 1);
        }
    }
}

void SignalHandler::sigaction_set(Signals signumset)
{
    struct sigaction new_action, old_action;

    new_action.sa_handler = SignalHandler::__handler;
    new_action.sa_flags = 0;
    sigemptyset(&new_action.sa_mask);

    for (int signum = 1; signum <= _NSIG; ++signum) {
        if (!signumset.testFlag(flagForSignal(signum)))
            continue;
        if (m_installedHandlers.contains(signum)) {
            m_installedHandlers[signum]++;
        }
        else {
            sigaction(signum, &new_action, &old_action);
            m_oldHandlers.insert(signum, old_action);
            m_installedHandlers.insert(signum, 1);
        }
    }
}

void SignalHandler::sigaction_unset(QSet<int> signumset)
{
    foreach (int signum, signumset) {
        if (!m_installedHandlers.contains(signum))
            continue;
        if (--m_installedHandlers[signum] == 0) {
            sigaction(signum, &m_oldHandlers[signum], NULL);
            m_oldHandlers.remove(signum);
            m_installedHandlers.remove(signum);
        }
    }
}

void SignalHandler::sigaction_unset(Signals signumset)
{
    for (int signum = 1; signum <= _NSIG; ++signum) {
        if (!signumset.testFlag(flagForSignal(signum)))
            continue;
        if (!m_installedHandlers.contains(signum))
            continue;
        if (--m_installedHandlers[signum] == 0) {
            sigaction(signum, &m_oldHandlers[signum], NULL);
            m_oldHandlers.remove(signum);
            m_installedHandlers.remove(signum);
        }
    }
}

void SignalHandler::execute(int signum)
{
    Signal sigFlag = flagForSignal(signum);
    foreach (Handler* h, m_handlers) {
        if (h->set & sigFlag)
            h->execute();
    }
    emit signalled(signum);
}

void SignalHandler::__handler(int signum)
{
    slot_handler_instance->execute(signum);
}
