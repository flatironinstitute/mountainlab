#ifndef PROCESSRESOURCEMONITOR_H
#define PROCESSRESOURCEMONITOR_H

#include "processormanager.h"

class ProcessResourceMonitorPrivate;
class ProcessResourceMonitor {
public:
    friend class ProcessResourceMonitorPrivate;
    ProcessResourceMonitor();
    virtual ~ProcessResourceMonitor();
    void setPid(int pid);
    void setProcessor(const MLProcessor& MLP);
    void setCLP(const QVariantMap& clp);

    bool withinLimits(QString* errstr);

private:
    ProcessResourceMonitorPrivate* d;
};

#endif // PROCESSRESOURCEMONITOR_H
