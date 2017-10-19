#include "mllogmaster.h"
#include "mprocmain.h"
#include "processormanager.h"
#include "processresourcemonitor.h"

#include <QCoreApplication>
#include <mlcommon.h>
#include <objectregistry.h>
#include <qprocessmanager.h>
#include <QDir>
#include <QLoggingCategory>
#include <QJsonDocument>
#include <QJsonArray>
#include <QCryptographicHash>
#include "cachemanager.h"
#include "handle_request.h"
#include "mllogmaster.h"

#include "signal.h"
#include <unistd.h>

Q_LOGGING_CATEGORY(MP, "mproc.main")

bool initialize_processor_manager(ProcessorManager& PM, QString* error_str);

int main(int argc, char* argv[])
{
    QCoreApplication app(argc, argv);
    CLParams CLP(argc, argv);

    MLLogMaster LM;
    LM.install();

    //ObjectRegistry registry;

    //The qprocess manager (very different from the ProcessManager!)
    //QProcessManager* qprocessManager = new QProcessManager;
    //registry.addAutoReleasedObject(qprocessManager);
    signal(SIGINT, sig_handler);
    signal(SIGKILL, sig_handler);
    signal(SIGTERM, sig_handler);

    // If _working_path is specified then we change the current directory
    QString working_path = CLP.named_parameters.value("_working_path").toString();
    if (working_path.isEmpty())
        working_path = QDir::currentPath();
    if (!working_path.isEmpty()) {
        if (!QDir::setCurrent(working_path)) {
            qCWarning(MP) << "Unable to set working path to: " << working_path;
            return -1;
        }
    }

    // for convenience
    QString arg1 = CLP.unnamed_parameters.value(0);
    QString arg2 = CLP.unnamed_parameters.value(1);
    QString arg3 = CLP.unnamed_parameters.value(2);

    // this may have been important
    setbuf(stdout, NULL);

    if (arg1 == "list-processors") { //Provide a human-readable list of the available processors
        if (list_processors())
            return 0;
        else
            return -1;
    }
    else if (arg1 == "spec") { // Show the spec for a single processor
        if (spec(arg2))
            return 0;
        else
            return -1;
    }
    else if (arg1 == "test") { // Run test for a single processor
        if (test_processor(arg2))
            return 0;
        else
            return -1;
    }
    else if ((arg1 == "exec") || (arg1 == "run") || (arg1 == "queue")) {
        CacheManager::globalInstance()->removeExpiredFiles();
        int ret = exec_run_or_queue(arg1, arg2, CLP.named_parameters);
        return ret;
    }
    else if (arg1 == "handle-request") {
        QString request_fname = arg2;
        QString response_fname = arg3;
        if (request_fname.isEmpty()) {
            print_usage();
            return -1;
        }
        QJsonObject response;
        response["success"] = false; //assume the worst

        QString prvbucket_path = CLP.named_parameters["prvbucket_path"].toString();

        QString request_json = TextFile::read(request_fname);
        if (request_json.isEmpty()) {
            response["error"] = "Request file is empty or does not exist.";
        }
        else {
            QJsonParseError parse_error;
            QJsonObject request = QJsonDocument::fromJson(request_json.toUtf8(), &parse_error).object();
            if (parse_error.error != QJsonParseError::NoError) {
                response["error"] = "Error parsing request json.";
            }
            else {
                ProcessorManager PM;
                QString errstr;
                if (!initialize_processor_manager(PM, &errstr)) {
                    response["error"] = "Failed to initialize process manager: " + errstr;
                }
                else {
                    response = handle_request(request, prvbucket_path, &PM);
                }
            }
        }
        QString response_json = QJsonDocument(response).toJson();
        if (response_fname.isEmpty()) {
            printf("%s\n", response_json.toUtf8().data());
            return 0;
        }
        else {
            qCInfo(MP) << "Writing response file: " + response_fname + QString(" size of json response: %1 bytes").arg(response_json.count());
            if (!TextFile::write(response_fname, response_json)) {
                qCWarning(MP) << "Error writing response to file";
                return -1;
            }
            return 0;
        }
    }
    else {
        print_usage(); //print usage information
        return -1;
    }

    return 0;
}

void sig_handler(int signum)
{
    (void)signum;
    set_terminate_requested(true);
    /*
    (void)signum;
    QProcessManager* manager = ObjectRegistry::getObject<QProcessManager>();
    if (manager) {
        manager->closeAll();
    }
    abort();
    */
}

void print_usage()
{
    printf("Usage:\n");
    printf("mproc exec [processor_name] --[param1]=[val1] --[param2]=[val2] ... [--_force_run] [--_request_num_threads=4]\n");
    printf("mproc run [processor_name] --[param1]=[val1] --[param2]=[val2] ... [--_force_run] [--_request_num_threads=4]\n");
    printf("mproc queue [processor_name] --[param1]=[val1] --[param2]=[val2] ... [--_force_run] [--_request_num_threads=4]\n");
    printf("mproc list-processors\n");
    printf("mproc spec [processor_name]\n");
    printf("mproc test [processor_name]\n");
}

bool initialize_processor_manager(ProcessorManager& PM, QString* error_str)
{
    // Load the processor paths
    QStringList processor_paths = MLUtil::configResolvedPathList("mountainprocess", "processor_paths");
    if (processor_paths.isEmpty()) {
        qCCritical(MP) << "No processor paths found.";
        *error_str = "No processor paths found.";
        return false;
    }

    PM.setProcessorPaths(processor_paths);
    PM.reloadProcessors();
    return true;
}

bool list_processors()
{
    ProcessorManager PM;
    QString errstr;
    if (!initialize_processor_manager(PM, &errstr))
        return false;

    QStringList pnames = PM.processorNames();
    qSort(pnames);
    foreach (QString pname, pnames) {
        qDebug().noquote() << pname;
    }
    return true;
}

void silent_message_output(QtMsgType type, const QMessageLogContext& context, const QString& msg)
{
    Q_UNUSED(type)
    Q_UNUSED(context)
    Q_UNUSED(msg)
    return;
}

bool spec(QString arg2)
{
    qInstallMessageHandler(silent_message_output);
    ProcessorManager PM;
    QString errstr;
    if (!initialize_processor_manager(PM, &errstr)) {
        return false;
    }
    if (!arg2.isEmpty()) {
        MLProcessor MLP = PM.processor(arg2);
        QString json = QJsonDocument(MLP.spec).toJson(QJsonDocument::Indented);
        printf("%s\n", json.toLatin1().data());
    }
    else {
        QJsonArray processors_array;
        QStringList pnames = PM.processorNames();
        foreach (QString pname, pnames) {
            MLProcessor MLP = PM.processor(pname);
            processors_array.push_back(MLP.spec);
        }
        QJsonObject obj;
        obj["processors"] = processors_array;
        QString json = QJsonDocument(obj).toJson(QJsonDocument::Indented);
        printf("%s\n", json.toLatin1().data());
    }
    return true;
}

void test_processor(const MLProcessor& MLP)
{
    QJsonObject spec = MLP.spec;
    if (!spec["has_test"].toBool()) {
        printf("%s\n", QString("Processor %1 has no test").arg(MLP.name).toUtf8().data());
    }
    else {
        QString exe = MLP.mp_file_name;
        int ret = system(QString(exe + " test " + MLP.name).toUtf8().data());
        (void)ret;
    }
}

bool test_processor(QString arg2)
{
    ProcessorManager PM;
    QString errstr;
    if (!initialize_processor_manager(PM, &errstr)) {
        return false;
    }
    if (arg2.isEmpty()) {
        //test them all
        QStringList names = PM.processorNames();
        foreach (QString pname, names) {
            printf("_________ %s _________\n", pname.toUtf8().data());
            MLProcessor MLP = PM.processor(pname);
            test_processor(MLP);
            printf("\n");
        }
    }
    else {
        MLProcessor MLP = PM.processor(arg2);
        test_processor(MLP);
    }
    return true;
}

void finalize(QString arg1, const MLProcessor& MLP, const QMap<QString, QVariant>& clp, const MLProcessInfo& info)
{
    if (((arg1 == "run") || (arg1 == "queue")) && (info.exit_code == 0)) {
        record_completed_process(MLP, clp);
    }

    if (!clp.value("_process_output").toString().isEmpty()) {
        write_process_output_file(clp.value("_process_output").toString(), info);
    }
}

bool cache_outputs(const MLProcessor& MLP)
{
    QJsonObject spec = MLP.spec;
    if (spec.contains("opts")) {
        QJsonObject opts = spec["opts"].toObject();
        if (opts.contains("cache_output")) {
            if (!opts["cache_output"].toBool()) {
                return false;
            }
        }
    }
    return true; //by default
}

int exec_run_or_queue(QString arg1, QString arg2, const QMap<QString, QVariant>& clp)
{
    MLProcessInfo info;

    QString processor_name = arg2;
    ProcessorManager PM;
    QString error_str;
    MLProcessor MLP;
    if (!initialize_processor_manager(PM, &error_str)) {
        info.exit_code = -1;
        info.error = error_str;
        finalize(arg1, MLP, clp, info);
        return info.exit_code;
    }
    MLP = PM.processor(processor_name);
    if (MLP.name != processor_name) {
        info.exit_code = -1;
        info.error = "Unable to find processor: " + processor_name;
        qCWarning(MP) << info.error;
        finalize(arg1, MLP, clp, info);
        return info.exit_code;
    }

    if (!PM.checkParameters(processor_name, clp, &error_str)) {
        info.exit_code = -1;
        info.error = error_str;
        finalize(arg1, MLP, clp, info);
        return info.exit_code;
    }

    bool force_run = clp.contains("_force_run");
    if (!cache_outputs(MLP)) {
        force_run = true;
    }

    if (((arg1 == "run") || (arg1 == "queue")) && (!force_run)) {
        if (process_already_completed(MLP, clp)) {
            qDebug().noquote() << "Process already completed: " + processor_name;
            info.exit_code = 0;
            finalize(arg1, MLP, clp, info);
            return info.exit_code;
        }
    }

    QString monitor_file_name;
    if (arg1 == "queue") {
        bool already_completed;
        monitor_file_name = wait_until_ready_to_run(MLP, clp, &already_completed, force_run);
        if (already_completed) {
            qDebug().noquote() << "Process already completed: " + processor_name;
            info.exit_code = 0;
            finalize(arg1, MLP, clp, info);
            return info.exit_code;
        }
        if (monitor_file_name.isEmpty()) {
            qDebug().noquote() << "Process ended by system: " + processor_name;
            info.exit_code = -1;
            info.error = "Process ended by system: " + processor_name;
            finalize(arg1, MLP, clp, info);
            return info.exit_code;
        }
    }
    else {
        monitor_file_name = CacheManager::globalInstance()->makeLocalFile("monitor_file_" + MLUtil::makeRandomId() + ".json");
        TextFile::write(monitor_file_name, "dummy");
    }

    {
        QTime timer;
        timer.start();
        launch_process_and_wait(MLP, clp, monitor_file_name, info);
        double elapsed_sec = timer.elapsed() * 1.0 / 1000;
        if (info.exit_code == 0) {
            qDebug().noquote() << QString("Process completed successfully: %1 (Elapsed: %2 sec)").arg(processor_name).arg(elapsed_sec);
            finalize(arg1, MLP, clp, info);
            return info.exit_code;
        }
        else {
            qCWarning(MP).noquote() << info.error;
            qCWarning(MP).noquote() << QString("Process returned with non-zero exit code (%1): %2 (Elapsed: %3 sec)").arg(info.exit_code).arg(processor_name).arg(elapsed_sec);
            finalize(arg1, MLP, clp, info);
            return info.exit_code;
        }
    }
}

QStringList get_local_search_paths_2()
{
    QStringList local_search_paths = MLUtil::configResolvedPathList("prv", "local_search_paths");
    QString temporary_path = MLUtil::tempPath();
    if (!temporary_path.isEmpty()) {
        local_search_paths << temporary_path;
    }
    return local_search_paths;
}

QString resolve_file_name_prv(QString fname)
{
    if (fname.endsWith(".prv")) {
        QString txt = TextFile::read(fname);
        QJsonParseError err;
        QJsonObject obj = QJsonDocument::fromJson(txt.toUtf8(), &err).object();
        if (err.error != QJsonParseError::NoError) {
            qCWarning(MP) << "Error parsing .prv file: " + fname;
            return "";
        }
        QString path0 = MLUtil::locatePrv(obj, get_local_search_paths_2());
        if (path0.isEmpty()) {
            qCWarning(MP) << "Unable to locate prv file originally at: " + obj["original_path"].toString();
            return "";
        }
        return path0;
    }
    else
        return fname;
}

QVariantMap resolve_file_names_in_inputs(const MLProcessor& MLP, const QVariantMap& parameters_in, bool* success, QString* errstr)
{
    (*success) = true;
    QVariantMap parameters = parameters_in;

    foreach (MLParameter P, MLP.inputs) {
        QStringList list = MLUtil::toStringList(parameters[P.name]);
        if (list.count() == 1) {
            parameters[P.name] = resolve_file_name_prv(list[0]);
            if ((!list[0].isEmpty()) && (parameters[P.name].toString().isEmpty())) {
                (*success) = false;
                *errstr = "Error resolving prv: " + list[0];
                return QVariantMap();
            }
        }
        else {
            QVariantList list2;
            foreach (QString str, list) {
                QString str2 = resolve_file_name_prv(str);
                if ((!str.isEmpty()) && (str2.isEmpty())) {
                    (*success) = false;
                    *errstr = "Error resolving prv: " + str;
                    return QVariantMap();
                }
                list2 << str2;
            }
            parameters[P.name] = list2;
        }
    }
    return parameters;
}

void set_defaults_for_optional_parameters(const MLProcessor& MLP, QVariantMap& params)
{
    QStringList pkeys = MLP.parameters.keys();
    foreach (QString key, pkeys) {
        MLParameter pp = MLP.parameters[key];
        if (pp.optional) {
            if ((!params.contains(key)) || (params[key].toString().isEmpty()))
                params[key] = pp.default_value;
        }
    }
}

void remove_output_files(const MLProcessor& MLP, const QMap<QString, QVariant>& clp)
{
    QStringList okeys = MLP.outputs.keys();
    foreach (QString key, okeys) {
        QString fname = clp.value(key).toString();
        if ((!fname.isEmpty()) && (QFile::exists(fname)))
            QFile::remove(fname);
    }
}

void launch_process_and_wait(const MLProcessor& MLP, const QMap<QString, QVariant>& clp_in, QString monitor_file_name, MLProcessInfo& info)
{
    bool success;
    QString errstr;
    QVariantMap clp = resolve_file_names_in_inputs(MLP, clp_in, &success, &errstr);
    if (!success) {
        info.exit_code = -1;
        info.error = errstr;
        return;
    }

    set_defaults_for_optional_parameters(MLP, clp);

    QString id = MLUtil::makeRandomId();
    QString tempdir = CacheManager::globalInstance()->makeLocalFile("tempdir_" + id, CacheManager::ShortTerm);
    MLUtil::mkdirIfNeeded(tempdir);
    if (!QFile::exists(tempdir)) {
        qCWarning(MP) << "Error creating temporary directory for process: " + tempdir;
        info.exit_code = -1;
        info.error = "Error creating temporary directory for process: " + tempdir;
        return;
    }

    QString exe_command = MLP.exe_command;
    exe_command.replace(QRegExp("\\$\\(basepath\\)"), MLP.basepath);
    exe_command.replace(QRegExp("\\$\\(tempdir\\)"), tempdir);
    {
        QString ppp;
        {
            QStringList keys = MLP.inputs.keys();
            foreach (QString key, keys) {
                QStringList list = MLUtil::toStringList(clp[key]);
                exe_command.replace(QRegExp(QString("\\$%1\\$").arg(key)), list.value(0)); //note that only the first file name is inserted here
                foreach (QString str, list) {
                    ppp += QString("--%1=%2 ").arg(key).arg(str);
                }
            }
        }
        {
            QStringList keys = MLP.outputs.keys();
            foreach (QString key, keys) {
                QStringList list = MLUtil::toStringList(clp[key]);
                exe_command.replace(QRegExp(QString("\\$%1\\$").arg(key)), list.value(0)); //note that only the first file name is inserted here
                foreach (QString str, list) {
                    if (key != "console_out")
                        ppp += QString("--%1=%2 ").arg(key).arg(str);
                }
            }
        }
        {
            QStringList keys = MLP.parameters.keys();
            foreach (QString key, keys) {
                exe_command.replace(QRegExp(QString("\\$%1\\$").arg(key)), clp[key].toString());
                ppp += QString("--%1=%2 ").arg(key).arg(clp[key].toString());
            }
        }

        int rnt = clp.value("_request_num_threads").toInt();
        if (rnt) {
            ppp += QString("--_request_num_threads=%1 ").arg(rnt);
        }
        ppp += QString("--_tempdir=%1 ").arg(tempdir);

        exe_command.replace(QRegExp("\\$\\(arguments\\)"), ppp);
    }

    remove_output_files(MLP, clp);

    qDebug().noquote() << QString("RUNNING %1: " + exe_command).arg(MLP.name);
    info.exe_command = exe_command;
    info.start_time = QDateTime::currentDateTime();
    QProcess qprocess;
    qprocess.setProcessChannelMode(QProcess::MergedChannels);
    qprocess.start(exe_command);
    if (!qprocess.waitForStarted(2000)) {
        info.exit_code = -1;
        info.error = "Problem starting: " + exe_command;
        return;
    }

    QString console_output_fname = clp.value("console_out").toString();
    QFile confile;
    if (!console_output_fname.isEmpty()) {
        confile.setFileName(console_output_fname);
        if (!confile.open(QIODevice::WriteOnly)) {
            info.exit_code = -1;
            info.error = "Unable to open console output file: " + console_output_fname;
            return;
        }
    }

    qint64 pid = qprocess.pid();
    ProcessResourceMonitor PRM;
    PRM.setPid(pid);
    PRM.setProcessor(MLP);
    PRM.setCLP(clp);
    QTime timer0;
    timer0.start();
    bool terminated = false;
    while (1) {
        qprocess.waitForFinished(10);
        if (terminate_requested()) {
            info.error = "Terminate requested";
            info.exit_code = -1;
            qprocess.terminate();
            qprocess.kill();
            terminated = true;
            break;
        }
        QByteArray str = qprocess.readAll();
        if (!str.isEmpty()) {
            qDebug().noquote() << str;
            info.console_output += str;
            if (confile.isOpen()) {
                confile.write(str);
            }
        }
        if (qprocess.state() != QProcess::Running) {
            break;
        }
        if (timer0.elapsed() > 1000) {
            QString errstr;
            if (!PRM.withinLimits(&errstr)) {
                info.error = errstr;
                info.exit_code = -1;
                qprocess.terminate();
                qprocess.kill();
                terminated = true;
                break;
            }
            else if (QFile::exists(monitor_file_name + ".stop")) {
                info.error = "Found .stop file";
                info.exit_code = -1;
                qprocess.terminate();
                qprocess.kill();
                terminated = true;
                break;
            }
            else {
                touch(monitor_file_name);
            }
            timer0.restart();
        }
    }
    if (confile.isOpen()) {
        confile.close();
    }
    info.finish_time = QDateTime::currentDateTime();
    if (!terminated)
        info.exit_code = qprocess.exitCode();
    info.parameters = clp;
    info.processor_name = MLP.name;
    if (!monitor_file_name.isEmpty()) {
        if (QFile::exists(monitor_file_name)) {
            //if (!terminated)
            //    qCWarning(MP).noquote() << "Unexpected: Monitor file still exists! Removing: " + monitor_file_name;
            if (!QFile::remove(monitor_file_name)) {
                qCWarning(MP).noquote() << "Unexpected: Unable to remove monitor file: " + monitor_file_name;
            }
        }
    }
}

bool all_input_and_output_files_exist(MLProcessor P, const QVariantMap& parameters, bool verbose)
{
    QStringList input_file_pnames = P.inputs.keys();
    QStringList output_file_pnames = P.outputs.keys();

    foreach (QString pname, input_file_pnames) {
        QStringList fnames = MLUtil::toStringList(parameters.value(pname));
        foreach (QString fname0, fnames) {
            if (!fname0.isEmpty()) {
                if (!QFile::exists(fname0)) {
                    if (verbose)
                        qCWarning(MP).noquote() << "Input file does not exist: " + fname0;
                    return false;
                }
            }
        }
    }

    foreach (QString pname, output_file_pnames) {
        QStringList fnames = MLUtil::toStringList(parameters.value(pname));
        foreach (QString fname0, fnames) {
            if (!fname0.isEmpty()) {
                if (!QFile::exists(fname0)) {
                    if (verbose)
                        qCWarning(MP).noquote() << "Output file does not exist: " + fname0;
                    return false;
                }
            }
        }
    }

    return true;
}

QJsonObject create_file_object(const QString& fname)
{
    QJsonObject obj;
    if (fname.isEmpty())
        return obj;
    obj["path"] = fname;
    if (!QFile::exists(fname)) {
        obj["size"] = 0;
        return obj;
    }
    else {
        obj["size"] = QFileInfo(fname).size();
        obj["last_modified"] = QFileInfo(fname).lastModified().toString("yyyy-MM-dd-hh-mm-ss-zzz");
    }
    if (QFileInfo(fname).isDir()) {
        QStringList fnames = QDir(fname).entryList(QDir::Files, QDir::Name);
        QJsonArray files_array;
        foreach (QString fname2, fnames) {
            QJsonObject obj0;
            obj0["name"] = fname2;
            obj0["object"] = create_file_object(fname + "/" + fname2);
            files_array.push_back(obj0);
        }
        obj["files"] = files_array;

        QStringList dirnames = QDir(fname).entryList(QDir::Dirs | QDir::NoDotAndDotDot, QDir::Name);
        QJsonArray dirs_array;
        foreach (QString dirname2, dirnames) {
            QJsonObject obj0;
            obj0["name"] = dirname2;
            obj0["object"] = create_file_object(fname + "/" + dirname2);
            dirs_array.push_back(obj0);
        }
        obj["directories"] = dirs_array;
    }
    return obj;
}

QJsonObject compute_unique_process_object(MLProcessor P, const QVariantMap& parameters)
{
    /*
     * Returns an object that depends uniquely on the following:
     *   1. Version of mountainprocess
     *   2. Processor name and version
     *   3. The paths, sizes, and modification times of the input files (together with their parameter names)
     *   4. Same for the output files
     *   5. The parameters converted to strings
     */

    QJsonObject obj;

    obj["mountainprocess_version"] = "0.1";
    obj["processor_name"] = P.name;
    obj["processor_version"] = P.version;
    {
        QJsonObject inputs;
        QStringList input_pnames = P.inputs.keys();
        qSort(input_pnames);
        foreach (QString input_pname, input_pnames) {
            QStringList fnames = MLUtil::toStringList(parameters[input_pname]);
            if (fnames.count() == 1) {
                inputs[input_pname] = create_file_object(fnames[0]);
            }
            else {
                QJsonArray array;
                foreach (QString fname0, fnames) {
                    array.append(create_file_object(fname0));
                }
                inputs[input_pname] = array;
            }
        }
        obj["inputs"] = inputs;
    }
    {
        QJsonObject outputs;
        QStringList output_pnames = P.outputs.keys();
        qSort(output_pnames);
        foreach (QString output_pname, output_pnames) {
            QStringList fnames = MLUtil::toStringList(parameters[output_pname]);
            if (fnames.count() == 1) {
                outputs[output_pname] = create_file_object(fnames[0]);
            }
            else {
                QJsonArray array;
                foreach (QString fname0, fnames) {
                    array.append(create_file_object(fname0));
                }
                outputs[output_pname] = array;
            }
        }
        obj["outputs"] = outputs;
    }
    {
        QJsonObject parameters0;
        QStringList pnames = P.parameters.keys();
        qSort(pnames);
        foreach (QString pname, pnames) {
            parameters0[pname] = parameters[pname].toString();
        }
        obj["parameters"] = parameters0;
    }
    return obj;
}

bool process_already_completed(const MLProcessor& MLP, const QMap<QString, QVariant>& clp)
{
    if (!all_input_and_output_files_exist(MLP, clp, false))
        return false;

    QJsonObject obj = compute_unique_process_object(MLP, clp);

    QString code = compute_unique_object_code(obj);

    QString path0 = MLUtil::tempPath() + "/completed_processes";
    MLUtil::mkdirIfNeeded(path0);

    QString path1 = path0 + "/" + code + ".json";

    return QFile::exists(path1);
}

void record_completed_process(const MLProcessor& MLP, const QMap<QString, QVariant>& clp)
{
    if (!all_input_and_output_files_exist(MLP, clp, true)) {
        qCWarning(MP).noquote() << "Unexpected problem in record_completed_process: not all input and output files exist.";
        return;
    }

    QJsonObject obj = compute_unique_process_object(MLP, clp);

    QString code = compute_unique_object_code(obj);

    QString path0 = MLUtil::tempPath() + "/completed_processes";
    MLUtil::mkdirIfNeeded(path0);

    QString path1 = path0 + "/" + code + ".json";

    if (!QFile::exists(path1)) {
        QString json = QJsonDocument(obj).toJson();
        if (!TextFile::write(path1, json)) {
            qCWarning(MP).noquote() << "Unexpected problem in record_completed_process: unable to write file: " + path1;
        }
    }
}

void remove_stale_monitor_files(QString path)
{
    int timeout0 = 10;
    QStringList list = QDir(path).entryList(QStringList("*.json"), QDir::Files, QDir::Name);
    foreach (QString fname0, list) {
        QString fname = path + "/" + fname0;
        QDateTime ct = QDateTime::currentDateTime();
        int elapsed_sec = QFileInfo(fname).lastModified().secsTo(ct);
        if (elapsed_sec > timeout0) {
            qCWarning(MP).noquote() << QString("Removing stale monitor file after %1 sec: %2").arg(elapsed_sec).arg(fname);
            QFile::remove(fname);
        }
    }
}

QJsonObject read_json_file(QString fname)
{
    QString txt = TextFile::read(fname);
    if (txt.isEmpty()) {
        //don't report warning, i guess, because this happens if the file has disappeared
        //qCWarning(MP).noquote() << "Text is empty in read_json_file: "+fname;
        return QJsonObject();
    }
    QJsonParseError err;
    QJsonObject obj = QJsonDocument::fromJson(txt.toUtf8(), &err).object();
    if (err.error != QJsonParseError::NoError) {
        qDebug().noquote() << txt;
        qCWarning(MP).noquote() << "Error parsing json in file: " + fname;
    }
    return obj;
}

bool okay_to_run(const QList<QJsonObject>& running_process_objects, const MLProcessor& MLP, const QMap<QString, QVariant>& clp)
{
    (void)MLP;
    (void)clp;
    int max_simultaneous_processes = 2;
    if (running_process_objects.count() >= max_simultaneous_processes)
        return false;
    return true;
}

bool okay_to_run(QString path, const MLProcessor& MLP, const QMap<QString, QVariant>& clp, const QStringList& fnames_to_exclude)
{
    QStringList fnames;
    QStringList list = QDir(path).entryList(QStringList("*.json"), QDir::Files, QDir::Name);
    foreach (QString fname0, list) {
        QString fname = path + "/" + fname0;
        if (fnames_to_exclude.indexOf(fname) < 0) {
            fnames << fname;
        }
    }
    QList<QJsonObject> running_process_objects;
    foreach (QString fname, fnames) {
        if (QFile::exists(fname)) {
            QJsonObject obj = read_json_file(fname);
            if (!obj["processor_name"].toString().isEmpty())
                running_process_objects << obj;
        }
    }
    return okay_to_run(running_process_objects, MLP, clp);
}

QString wait_until_ready_to_run(const MLProcessor& MLP, const QMap<QString, QVariant>& clp, bool* already_completed, bool force_run)
{
    (*already_completed) = false;

    //seed the random number generator (see below)
    QDateTime cd = QDateTime::currentDateTime();
    qsrand(cd.toTime_t() + QCoreApplication::applicationPid());

    QString path_running = MLUtil::tempPath() + "/running_processes";
    MLUtil::mkdirIfNeeded(path_running);

    QJsonObject obj;
    obj["processor_name"] = MLP.name;
    obj["clp"] = QJsonObject::fromVariantMap(clp);
    QString obj_json = QJsonDocument(obj).toJson();

    QString monitor_file_name = path_running + "/" + MLP.name + "_" + MLUtil::makeRandomId() + ".json";

    while (1) {
        if (!force_run) {
            if (process_already_completed(MLP, clp)) {
                (*already_completed) = true;
                return "";
            }
        }

        remove_stale_monitor_files(path_running);
        if (okay_to_run(path_running, MLP, clp, QStringList())) {
            if (!TextFile::write(monitor_file_name, obj_json)) {
                qCWarning(MP).noquote() << "Unable to write file: " + monitor_file_name;
                return "";
            }
            //important: check that it is still okay to run

            QStringList exclude = QStringList(monitor_file_name);
            bool okay = okay_to_run(path_running, MLP, clp, exclude);

            if (okay) {
                //it really is okay to run. We are done.
                break;
            }
            else {
                //not okay to run, actually. another process beat us to it. remove the file and try again
                if (!QFile::remove(monitor_file_name)) {
                    qCWarning(MP).noquote() << "Unexpected problem. Unable to remove monitor file: " + monitor_file_name;
                    return "";
                }
            }
        }
        sleep_msec(1000 + (qrand() % 500)); //sleep a variable amount to minimize chance of conflict
    }

    return monitor_file_name;
}

void write_process_output_file(QString fname, const MLProcessInfo& info)
{
    QJsonObject obj; //the output info to be saved
    obj["exe_command"] = info.exe_command;
    obj["exit_code"] = info.exit_code;
    obj["parameters"] = QJsonObject::fromVariantMap(info.parameters);
    obj["processor_name"] = info.processor_name;
    obj["success"] = (info.exit_code == 0);
    obj["error"] = info.error;
    obj["start_time"] = info.start_time.toString("yyyy-MM-dd:hh-mm-ss.zzz");
    obj["finish_time"] = info.finish_time.toString("yyyy-MM-dd:hh-mm-ss.zzz");
    QString json = QJsonDocument(obj).toJson();
    TextFile::write(fname, json);
}
