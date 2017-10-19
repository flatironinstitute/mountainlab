#include "processresourcemonitor.h"
#include <QJsonDocument>

#include <QLoggingCategory>

Q_LOGGING_CATEGORY(PRM, "mproc.prm")

class ProcessResourceMonitorPrivate {
public:
    ProcessResourceMonitor* q;

    int m_pid = 0;
    MLProcessor m_processor;
    QVariantMap m_clp;
};

ProcessResourceMonitor::ProcessResourceMonitor()
{
    d = new ProcessResourceMonitorPrivate;
    d->q = this;
}

ProcessResourceMonitor::~ProcessResourceMonitor()
{
    delete d;
}

void ProcessResourceMonitor::setPid(int pid)
{
    d->m_pid = pid;
}

void ProcessResourceMonitor::setProcessor(const MLProcessor& MLP)
{
    d->m_processor = MLP;
}

void ProcessResourceMonitor::setCLP(const QVariantMap& clp)
{
    d->m_clp = clp;
}

QString execute_and_read_stdout_2(QString cmd)
{
    QProcess P;
    P.start(cmd);
    P.waitForStarted();
    P.waitForFinished(-1);
    return P.readAllStandardOutput();
}

struct ProcStat {
    double rss = 0;
    double etime = 0;
    double cputime = 0;
    double cpu_pct = 0;
};
ProcStat get_proc_stat(int pid)
{
    ProcStat ret;
    QString cmd0 = QString("procstat %1").arg(pid);
    QString tmp = execute_and_read_stdout_2(cmd0);
    QJsonObject obj = QJsonDocument::fromJson(tmp.toUtf8()).object();
    QJsonObject mm = obj["maxima"].toObject();
    ret.rss = mm["rss"].toDouble();
    ret.etime = mm["etime"].toDouble();
    ret.cputime = mm["cputime"].toDouble();
    ret.cpu_pct = mm["%cpu"].toDouble();
    return ret;
}

struct Limits {
    double max_ram_gb = 0;
    double max_etime_sec = 0;
    double max_cputime_sec = 0;
    double max_cpu_pct = 0;
};
Limits get_limits_from_clp(const QVariantMap& clp)
{
    Limits ret;
    ret.max_ram_gb = clp.value("_max_ram_gb", 0).toDouble();
    ret.max_etime_sec = clp.value("_max_etime_sec", 0).toDouble();
    ret.max_cputime_sec = clp.value("_max_cputime_sec", 0).toDouble();
    ret.max_cpu_pct = clp.value("_max_cpu_pct", 0).toDouble();
    return ret;
}

bool ProcessResourceMonitor::withinLimits(QString* errstr)
{
    if (!d->m_pid)
        return true;
    Limits L = get_limits_from_clp(d->m_clp);

    if ((L.max_ram_gb > 0) || (L.max_etime_sec > 0) || (L.max_cputime_sec > 0) || (L.max_cpu_pct > 0)) {
        double cpu_pct_tolerance = 20;
        ProcStat PC = get_proc_stat(d->m_pid);
        //d->m_peak_ram_usage_gb=qMax(d->m_peak_ram_usage).arg(PC.rss/1e6);
        if ((L.max_ram_gb) && (PC.rss / 1e6 > L.max_ram_gb)) {
            *errstr = QString("Process RAM exceeds limit: %1 > %2 GB").arg(PC.rss / 1e6).arg(L.max_ram_gb);
            qCWarning(PRM) << *errstr;
            return false;
        }
        else if ((L.max_etime_sec) && (PC.etime > L.max_etime_sec)) {
            *errstr = QString("Elapsed time exceeds limit: %1 > %2 sec").arg(PC.etime).arg(L.max_etime_sec);
            qCWarning(PRM) << *errstr;
            return false;
        }
        else if ((L.max_cputime_sec) && (PC.cputime > L.max_cputime_sec)) {
            *errstr = QString("Elapsed cpu time exceeds limit: %1 > %2 sec").arg(PC.cputime).arg(L.max_cputime_sec);
            qCWarning(PRM) << *errstr;
            return false;
        }
        else if ((L.max_cpu_pct) && (PC.cpu_pct > L.max_cpu_pct + cpu_pct_tolerance)) {
            *errstr = QString("CPU usage exceeds limit: %1% > %2%").arg(PC.cpu_pct).arg(L.max_cpu_pct);
            qCWarning(PRM) << *errstr;
            return false;
        }
    }
    return true;
}
