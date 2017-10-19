#ifndef MLLOGMASTER_H
#define MLLOGMASTER_H

#include <QLoggingCategory>

class MLLogMasterPrivate;
class MLLogMaster {
public:
    friend class MLLogMasterPrivate;
    MLLogMaster();
    virtual ~MLLogMaster();
    void install();

private:
    MLLogMasterPrivate* d;
};

#endif // MLLOGMASTER_H
