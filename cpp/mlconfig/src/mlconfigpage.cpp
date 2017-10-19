/******************************************************
** See the accompanying README and LICENSE files
** Author(s): Jeremy Magland
** Created: 3/8/2017
*******************************************************/

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
