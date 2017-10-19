/******************************************************
** See the accompanying README and LICENSE files
** Author(s): Jeremy Magland
** Created: 7/4/2016
*******************************************************/

#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QDebug>
#include <QCoreApplication>
#include <QFileInfo>
#include <QDir>

#include "mlcommon.h"
#include "mlconfigpage.h"

////////////////////////////////////////////////////////////////////////////////////////
class Tempdir_Q1 : public MLConfigQuestion {
public:
    virtual ~Tempdir_Q1()
    {
    }
    QString ask() Q_DECL_OVERRIDE
    {
        QJsonObject gen = config()->value("general").toObject();
        QString temporary_path = gen["temporary_path"].toString();
        QString str = "Current temporary path: " + temporary_path + "\n";
        str += "Type new path or enter to keep same:";
        return str;
    }
    bool processResponse(QString str) Q_DECL_OVERRIDE
    {
        if (str.isEmpty())
            return true;
        QString new_path = str.trimmed();
        if (!QFileInfo(new_path).isAbsolute()) {
            qDebug().noquote() << "You must enter an absolute path.";
            return false;
        }
        if ((!QFile::exists(new_path)) || (!QFileInfo(new_path).isDir())) {
            qDebug().noquote() << "Directory does not exist: " + new_path;
            return false;
        }
        QJsonObject gen = config()->value("general").toObject();
        gen["temporary_path"] = new_path;
        (*config())["general"] = gen;
        return true;
    }
};

class Page_tempdir : public MLConfigPage {
public:
    Page_tempdir(QJsonObject* config)
        : MLConfigPage(config)
    {
        addQuestion(new Tempdir_Q1);
    };
    QString title() Q_DECL_OVERRIDE
    {
        return "Temporary Directory";
    }
    QString description() Q_DECL_OVERRIDE
    {
        return "This is the directory where all temporary and intermediate files are stored. "
               "It should be in a location with a lot of free disk space. You may safely "
               "delete the data periodically, but it is a good idea not to do this during "
               "processing.";
    }
};

QStringList json_array_to_stringlist(QJsonArray X)
{
    QStringList ret;
    foreach (QJsonValue val, X) {
        ret << val.toString();
    }
    return ret;
}
QJsonArray stringlist_to_json_array(QStringList X)
{
    QJsonArray ret;
    foreach (QString val, X) {
        ret << val;
    }
    return ret;
}

////////////////////////////////////////////////////////////////////////////////////////
class Prv_Q1 : public MLConfigQuestion {
public:
    virtual ~Prv_Q1()
    {
    }
    QString ask() Q_DECL_OVERRIDE
    {
        QJsonObject prv = config()->value("prv").toObject();
        QStringList local_search_paths = json_array_to_stringlist(prv["local_search_paths"].toArray());
        QString str = "Current prv search paths:\n     " + local_search_paths.join("\n     ");
        str += "\n";
        str += "Type in a path to add or delete, or press enter to keep those listed above:";
        return str;
    }
    bool processResponse(QString str) Q_DECL_OVERRIDE
    {
        QJsonObject prv = config()->value("prv").toObject();
        QStringList local_search_paths = json_array_to_stringlist(prv["local_search_paths"].toArray());
        if (str.isEmpty())
            return true;
        QString path = str.trimmed();
        if (path.isEmpty()) {
            return true;
        }
        else {
            if (!QFileInfo(path).isAbsolute()) {
                qDebug().noquote() << "You must enter an absolute path.";
                return false;
            }
            if (local_search_paths.contains(path)) {
                local_search_paths.removeAll(path);
            }
            else if ((!QFile::exists(path)) || (!QFileInfo(path).isDir())) {
                qDebug().noquote() << "Directory does not exist: " + path;
                return false;
            }
            else {
                local_search_paths << path;
            }
            prv["local_search_paths"] = stringlist_to_json_array(local_search_paths);
            (*config())["prv"] = prv;
            return false;
        }
    }
};

class Page_prv : public MLConfigPage {
public:
    Page_prv(QJsonObject* config)
        : MLConfigPage(config)
    {
        addQuestion(new Prv_Q1);
    };
    QString title() Q_DECL_OVERRIDE
    {
        return "PRV System";
    }
    QString description() Q_DECL_OVERRIDE
    {
        return "The prv system manages locating very large binary data files. "
               "In addition to default locations, you should select one or more directories where "
               "prv will search for your data files.";
    }
};

QString format_description(QString str, int line_length);
QJsonObject read_config();
void write_config(QJsonObject obj);
QString get_keyboard_response();
QJsonObject extend_object(QJsonObject obj1, QJsonObject obj2);
QJsonObject dupstend_object(QJsonObject obj1, QJsonObject obj2);

int main(int argc, char* argv[])
{
    QCoreApplication app(argc, argv);

    QJsonObject config = read_config();

    if ((argc==2)&&(QString(argv[1])=="tmp")) {
        QJsonObject gen = config.value("general").toObject();
        QString temporary_path = gen["temporary_path"].toString();
        qDebug().noquote() << temporary_path;
        return 0;
    }

    QList<MLConfigPage*> pages;
    pages << new Page_tempdir(&config);
    pages << new Page_prv(&config);

    qDebug().noquote() << "___MountainLab interactive configuration utility___";
    qDebug().noquote() << "";

    foreach (MLConfigPage* page, pages) {
        qDebug().noquote() << "****************************************************";
        qDebug().noquote() << "*** " + page->title() + " ***";
        QString descr = format_description(page->description(), 60);
        qDebug().noquote() << descr;
        qDebug().noquote() << "";
        for (int i = 0; i < page->questionCount(); i++) {
            QString str = page->question(i)->ask();
            qDebug().noquote() << str;
            QString resp = get_keyboard_response();
            if (!page->question(i)->processResponse(resp)) {
                qDebug().noquote() << "";
                i--;
            }
        }
    }

    qDeleteAll(pages);

    write_config(config);

    return 0;
}

inline QString read_text_file(const QString& path)
{
    QFile f(path);
    if (!f.open(QFile::ReadOnly | QFile::Text))
        return "";
    QTextStream in(&f);
    return in.readAll();
}

inline void write_text_file(const QString& path, const QString& txt)
{
    QDir().mkpath(QFileInfo(path).dir().absolutePath());
    QFile f(path);
    if (!f.open(QFile::WriteOnly | QFile::Text))
        return;
    QTextStream out(&f);
    out << txt;
}

QJsonObject read_config()
{
    //QString fname1 = MLUtil::defaultConfigPath().absoluteFilePath();
    QString fname2 = MLUtil::userConfigPath().absoluteFilePath();
    //qDebug() << fname1;
    //qDebug() << fname2;
    //QString json1 = read_text_file(fname1);

    QString json1 = "{"
                    "        \"general\":{"
                    "                \"temporary_path\":\"/tmp\","
                    "                \"max_cache_size_gb\":40"
                    "        },"
                    "        \"mountainprocess\":{"
                    "                \"max_num_simultaneous_processes\":2,"
                    "                \"processor_paths\":[\"cpp/mountainprocess/processors\",\"user/processors\",\"packages\"],"
                    "                \"mpdaemonmonitor_url\":\"http://mpdaemonmonitor.herokuapp.com\","
                    "                \"mpdaemon_name\":\"\",\"mpdaemon_secret\":\"\""
                    "        },"
                    "        \"prv\":{"
                    "                \"local_search_paths\":[\"examples\"],"
                    "                \"servers\":["
                    "                        {\"name\":\"datalaboratory\",\"passcode\":\"\",\"host\":\"http://datalaboratory.org\",\"port\":8005},"
                    "                        {\"name\":\"river\",\"passcode\":\"\",\"host\":\"http://river.simonsfoundation.org\",\"port\":60001},"
                    "                        {\"name\":\"localhost\",\"passcode\":\"\",\"host\":\"http://localhost\",\"port\":8080}"
                    "                ]"
                    "        },"
                    "        \"kron\":{"
                    "                \"dataset_paths\":[\"cpp/mountainsort/example_datasets\",\"user/datasets\"],"
                    "                \"pipeline_paths\":[\"cpp/mountainsort/pipelines\",\"user/pipelines\",\"packages\"],"
                    "                \"view_program_paths\":[\"cpp/mountainsort/view_programs\",\"/user/view_programs\"]"
                    "        }"
                    "}";

    QString json2 = fname2.isEmpty() ? QString() : read_text_file(fname2);
    if (json2.isEmpty())
        json2 = "{}";
    QJsonObject obj1 = QJsonDocument::fromJson(json1.toUtf8()).object();
    QJsonObject obj2 = QJsonDocument::fromJson(json2.toUtf8()).object();
    QJsonObject ret = extend_object(obj1, obj2);
    return ret;
}

void write_config(QJsonObject obj)
{
    //QString fname1 = MLUtil::defaultConfigPath().absoluteFilePath();
    QString fname2 = MLUtil::userConfigPath().absoluteFilePath();
    if (fname2.isEmpty())
        fname2 = MLUtil::userConfigPath(MLUtil::ConfigPathType::Preferred)
                     .absoluteFilePath();
    //qDebug() << Q_FUNC_INFO << fname2;
    //QString json1 = read_text_file(fname1);
    QString json1 = "{"
                    "        \"general\":{"
                    "                \"temporary_path\":\"/tmp\","
                    "                \"max_cache_size_gb\":40"
                    "        },"
                    "        \"mountainprocess\":{"
                    "                \"max_num_simultaneous_processes\":2,"
                    "                \"processor_paths\":[\"cpp/mountainprocess/processors\",\"user/processors\",\"packages\"],"
                    "                \"mpdaemonmonitor_url\":\"http://mpdaemonmonitor.herokuapp.com\","
                    "                \"mpdaemon_name\":\"\",\"mpdaemon_secret\":\"\""
                    "        },"
                    "        \"prv\":{"
                    "                \"local_search_paths\":[\"examples\"],"
                    "                \"servers\":["
                    "                        {\"name\":\"datalaboratory\",\"passcode\":\"\",\"host\":\"http://datalaboratory.org\",\"port\":8005},"
                    "                        {\"name\":\"river\",\"passcode\":\"\",\"host\":\"http://river.simonsfoundation.org\",\"port\":60001},"
                    "                        {\"name\":\"localhost\",\"passcode\":\"\",\"host\":\"http://localhost\",\"port\":8080}"
                    "                ]"
                    "        },"
                    "        \"kron\":{"
                    "                \"dataset_paths\":[\"cpp/mountainsort/example_datasets\",\"user/datasets\"],"
                    "                \"pipeline_paths\":[\"cpp/mountainsort/pipelines\",\"user/pipelines\",\"packages\"],"
                    "                \"view_program_paths\":[\"cpp/mountainsort/view_programs\",\"/user/view_programs\"]"
                    "        }"
                    "}";
    QJsonObject obj1 = QJsonDocument::fromJson(json1.toUtf8()).object();
    QJsonObject obj2 = dupstend_object(obj1, obj);
    QString json2 = QJsonDocument(obj2).toJson(QJsonDocument::Indented);
    write_text_file(fname2, json2);
}

QString get_keyboard_response()
{
    char str[1000];
    char* ret = fgets(str, 1000, stdin);
    (void)ret;
    return QString(str).trimmed();
}

QJsonObject extend_object(QJsonObject obj1, QJsonObject obj2)
{
    QJsonObject ret;
    QStringList keys1 = obj1.keys();
    QStringList keys2 = obj2.keys();
    foreach (QString key, keys1) {
        if (obj2.contains(key)) {
            if (obj1[key].isObject()) {
                ret[key] = extend_object(obj1[key].toObject(), obj2[key].toObject());
            }
            else {
                ret[key] = obj2[key];
            }
        }
        else {
            ret[key] = obj1[key];
        }
    }
    foreach (QString key, keys2) {
        if (!obj1.contains(key)) {
            ret[key] = obj2[key];
        }
    }
    return ret;
}

bool dupstend_match(QJsonValue V1, QJsonValue V2)
{
    if (V1.isObject()) {
        QJsonObject obj1 = V1.toObject();
        QJsonObject obj2 = V2.toObject();
        QStringList keys1 = obj1.keys();
        if (keys1.count() != obj2.keys().count())
            return false;
        foreach (QString key, keys1) {
            if (!obj2.contains(key))
                return false;
            if (!dupstend_match(obj1[key], obj2[key]))
                return false;
        }
        return true;
    }
    else {
        return (V1 == V2);
    }
}

//dupstend means check to see which values of obj2 are consistent with those in obj1 and then don't use those.
QJsonObject dupstend_object(QJsonObject obj1, QJsonObject obj2)
{
    QJsonObject ret;
    QStringList keys1 = obj1.keys();
    QStringList keys2 = obj2.keys();
    foreach (QString key, keys1) {
        if (obj2.contains(key)) {
            if (dupstend_match(obj1[key], obj2[key])) {
                //don't include it
            }
            else if (obj1[key].isObject()) {
                ret[key] = dupstend_object(obj1[key].toObject(), obj2[key].toObject());
            }
            else {
                ret[key] = obj2[key];
            }
        }
        else {
            ret[key] = obj1[key];
        }
    }
    foreach (QString key, keys2) {
        if (!obj1.contains(key)) {
            //don't include it
        }
    }
    return ret;
}

QString format_description(QString str, int line_length)
{
    QString ret;
    int i = 0;
    while (i < str.count()) {
        int j = i + line_length;
        if (j >= str.count())
            j = str.count() - 1;
        QString line = str.mid(i, j - i + 1);
        int aa = line.lastIndexOf(" ");
        if (aa > 0) {
            j = i + aa - 1;
        }
        line = line.mid(0, j - i + 1);
        ret += line.trimmed() + "\n";
        i += line.count();
    }
    return ret;
}
