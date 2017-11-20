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

#include "mlconfigpage.h"
#include <QDebug>

MLConfigPage::MLConfigPage(QJsonObject* config)
{
    m_config = config;
}

MLConfigPage::~MLConfigPage()
{
    qDeleteAll(m_questions);
    m_questions.clear();
}

int MLConfigPage::questionCount() const
{
    return m_questions.count();
}

MLConfigQuestion* MLConfigPage::question(int num) const
{
    return m_questions[num];
}

void MLConfigPage::addQuestion(MLConfigQuestion* question)
{
    question->m_config = m_config;
    m_questions << question;
}
