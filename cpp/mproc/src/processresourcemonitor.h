/*
 * Copyright 2016-2017 Flatiron Institute, Simons Foundation
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
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
