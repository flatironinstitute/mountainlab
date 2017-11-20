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
#ifndef MLCONFIGPAGE_H
#define MLCONFIGPAGE_H

#include <QJsonObject>
#include <QList>
#include <QString>
#include "mlconfigquestion.h"

class MLConfigPage {
public:
    MLConfigPage(QJsonObject* config);
    virtual ~MLConfigPage();

    virtual QString title() = 0;
    virtual QString description() = 0;

    int questionCount() const;
    MLConfigQuestion* question(int num) const;

protected:
    void addQuestion(MLConfigQuestion* question);

private:
    QJsonObject* m_config = 0;
    QList<MLConfigQuestion*> m_questions;
};

#endif // MLCONFIGPAGE_H
