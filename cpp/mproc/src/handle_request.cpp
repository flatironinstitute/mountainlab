#include "handle_request.h"
#include "mlcommon.h"
#include "processormanager.h"

#include <QJsonDocument>
#include <cachemanager.h>
#include <QCoreApplication>
#include <QFile>
#include <QCryptographicHash>
#include <QDir>
#include <unistd.h>

#include <QJsonArray>
#include <QLoggingCategory>
#include <QProcess>
#include <QTime>

Q_LOGGING_CATEGORY(RB, "mp.run_as_bash")
Q_LOGGING_CATEGORY(HR, "mp.handle_request")

QJsonObject handle_request_queue_process(QString processor_name, const QJsonObject& inputs, const QJsonObject& outputs, const QJsonObject& parameters, const QJsonObject& resources, QString prvbucket_path, ProcessorManager* PM);
QJsonObject handle_request_processor_spec(ProcessorManager* PM);

QJsonObject handle_request(const QJsonObject& request, QString prvbucket_path, ProcessorManager* PM)
{
    QString action = request["action"].toString();

    qCInfo(HR) << "Handling request" << action << QJsonDocument(request).toJson(QJsonDocument::Compact) << "prvbucket_path: " + prvbucket_path;

    QJsonObject response;
    response["success"] = false; //assume the worst

    if ((action == "run_process") || (action == "queue_process")) {
        QString processor_name = request["processor_name"].toString();
        if (processor_name.isEmpty()) {
            qCCritical(HR) << "Processor name is empty";
            response["error"] = "Processor name is empty";
            return response;
        }

        qCInfo(HR) << "Starting handle_request_queue_process: " + processor_name;
        QTime timer;
        timer.start();
        response = handle_request_queue_process(processor_name, request["inputs"].toObject(), request["outputs"].toObject(), request["parameters"].toObject(), request["resources"].toObject(), prvbucket_path, PM);
        qCInfo(HR) << "Done running process: " + processor_name << "Elapsed:" << timer.elapsed();
        return response;
    }
    else if (action == "processor_spec") {
        qCInfo(HR) << "Starting handle_request_processor_spec";
        QTime timer;
        timer.start();
        response = handle_request_processor_spec(PM);
        qCInfo(HR) << "Done with handle_request_processor_spec."
                   << "Elapsed:" << timer.elapsed();
        return response;
    }
    else {
        qCCritical(HR) << "Unknown action: " + action;
        response["error"] = "Unknown action: " + action;
        return response;
    }
}

QJsonObject get_processor_spec(ProcessorManager* PM)
{
    QJsonArray processors_array;
    QStringList pnames = PM->processorNames();
    foreach (QString pname, pnames) {
        MLProcessor MLP = PM->processor(pname);
        processors_array.push_back(MLP.spec);
    }
    QJsonObject obj;
    obj["processors"] = processors_array;
    return obj;
}

bool create_prv(QString fname_in, QString prv_fname_out)
{
    QFile::remove(prv_fname_out);
    QString exe = "prv";
    QStringList args;
    args << "create" << fname_in << prv_fname_out;
    QProcess pp;
    pp.execute(exe, args);
    return QFile::exists(prv_fname_out);
}

bool wait_for_file_to_exist(QString fname, int timeout_ms)
{
    QTime timer;
    timer.start();
    while ((!QFile::exists(fname)) && (timer.elapsed() <= timeout_ms))
        ;
    return QFile::exists(fname);
}

QJsonObject handle_request_processor_spec(ProcessorManager* PM)
{
    QJsonObject obj = get_processor_spec(PM);
    QJsonObject response;
    response["success"] = true;
    response["spec"] = obj;
    return response;
}

QJsonObject handle_request_queue_process(QString processor_name, const QJsonObject& inputs, const QJsonObject& outputs, const QJsonObject& parameters, const QJsonObject& resources, QString prvbucket_path, ProcessorManager* PM)
{
    QJsonObject response;

    MLProcessor PP = PM->processor(processor_name);
    if (PP.name != processor_name) { //rather use PP.isNull()
        response["success"] = false;
        response["error"] = "Unable to find processor: " + processor_name;
        return response;
    }

    QStringList args;
    args << processor_name;

    QStringList ikeys = inputs.keys();
    QStringList pkeys = parameters.keys();
    QStringList okeys = outputs.keys();
    foreach (QString key, PP.inputs.keys()) {
        if (!PP.inputs[key].optional) {
            if (!ikeys.contains(key)) {
                response["success"] = false;
                response["error"] = "Missing input: " + key;
                return response;
            }
        }
    }
    foreach (QString key, PP.outputs.keys()) {
        if (!PP.outputs[key].optional) {
            if (!okeys.contains(key)) {
                response["success"] = false;
                response["error"] = "Missing output: " + key;
                return response;
            }
        }
    }
    foreach (QString key, PP.parameters.keys()) {
        if (!PP.parameters[key].optional) {
            if (!pkeys.contains(key)) {
                response["success"] = false;
                response["error"] = "Missing parameter: " + key;
                return response;
            }
        }
    }

    foreach (QString key, ikeys) {
        if (PP.inputs.contains(key)) {
            if (inputs[key].isObject()) {
                QJsonObject prv_object = inputs[key].toObject();
                QString path0 = locate_prv(prv_object);
                if (path0.isEmpty()) {
                    response["success"] = false;
                    response["error"] = QString("Unable to locate prv for key=%1. (original_path=%2,checksum=%3)").arg(key).arg(prv_object["original_path"].toString()).arg(prv_object["original_checksum"].toString());
                    return response;
                }
                args << QString("--%1=%2").arg(key).arg(path0);
            }
            else if (inputs[key].isArray()) {
                QJsonArray prv_object_list = inputs[key].toArray();
                for (int aa = 0; aa < prv_object_list.count(); aa++) {
                    QJsonObject prv_object = prv_object_list[aa].toObject();
                    QString path0 = locate_prv(prv_object);
                    if (path0.isEmpty()) {
                        response["success"] = false;
                        response["error"] = QString("Unable to locate prv for key=%1. (original_path=%2,checksum=%3)").arg(key).arg(prv_object["original_path"].toString()).arg(prv_object["original_checksum"].toString());
                        return response;
                    }
                    args << QString("--%1=%2").arg(key).arg(path0);
                }
            }
            else {
                response["success"] = false;
                response["error"] = "Unexpected: Input is neither object nor array: " + key;
                return response;
            }
        }
        else {
            response["success"] = false;
            response["error"] = "Unexpected input: " + key;
            return response;
        }
    }

    foreach (QString key, pkeys) {
        if (PP.parameters.contains(key)) {
            args << QString("--%1=%2").arg(key).arg(parameters[key].toVariant().toString());
        }
        else {
            response["success"] = false;
            response["error"] = "Unexpected parameter: " + key;
            return response;
        }
    }

    QJsonObject code_object;
    code_object["processor_name"] = processor_name;
    code_object["inputs"] = inputs;
    code_object["parameters"] = parameters;
    QString code = compute_unique_object_code(code_object).mid(0, 10);

    QMap<QString, QString> output_files;
    foreach (QString key, okeys) {
        if (PP.outputs.contains(key)) {
            if (outputs[key].toBool()) {
                QString fname;
                if (!prvbucket_path.isEmpty()) {
                    QDir(prvbucket_path).mkdir("_mountainprocess");
                    fname = QString("%1/_mountainprocess/output_%2_%3").arg(prvbucket_path).arg(key).arg(code);
                }
                else {
                    fname = CacheManager::globalInstance()->makeLocalFile(QString("output_%1_%2").arg(key).arg(code));
                }
                CacheManager::globalInstance()->setTemporaryFileDuration(fname, 60 * 60 * 24 * 7);
                args << QString("--%1=%2").arg(key).arg(fname);
                output_files[key] = fname;
            }
        }
        else {
            response["success"] = false;
            response["error"] = "Unexpected output: " + key;
            return response;
        }
    }

    args << QString("--_max_ram_gb=%1").arg(resources["max_ram_gb"].toDouble());
    args << QString("--_max_etime_sec=%1").arg(resources["max_etime_sec"].toDouble());
    args << QString("--_max_cputime_sec=%1").arg(resources["max_cputime_sec"].toDouble());
    args << QString("--_max_cpu_pct=%1").arg(resources["max_cpu_pct"].toDouble());

    if (resources["force_run"].toBool()) {
        args << "--_force_run";
    }

    QString process_output_fname = CacheManager::globalInstance()->makeLocalFile("process_output_" + processor_name + "_" + MLUtil::makeRandomId() + ".json");
    //CacheManager::globalInstance()->setTemporaryFileExpirePid(process_output_fname,qApp->applicationPid());
    args << "--_process_output=" + process_output_fname;

    // do not do mp-queue-process because the terminate signal does not get transferred (???)
    QString exe_cmd = qApp->applicationDirPath() + "/mproc queue " + args.join(" ");

    qCDebug(HR).noquote() << "Running: " + exe_cmd;

    QTime timer;
    timer.start();

    QProcess qprocess;
    qprocess.setProcessChannelMode(QProcess::MergedChannels);
    //qprocess.start(exe_cmd);
    qprocess.start(exe_cmd);

    if (!qprocess.waitForStarted(2000)) {
        qCWarning(HR).noquote() << "Error starting command: " + exe_cmd;
    }

    bool terminated = false;
    QTime timer0;
    timer0.start();
    while (1) {
        qprocess.waitForFinished(10);
        if (terminate_requested()) {
            qCWarning(HR).noquote() << "Terminating..." << exe_cmd;
            qprocess.terminate();
            terminated = true;
        }
        QString str = qprocess.readAll();
        if (!str.isEmpty()) {
            qDebug().noquote() << str;
        }
        if (qprocess.state() != QProcess::Running) {
            break;
        }
        if (timer0.elapsed() > 1000) {
            timer0.restart();
        }
    }

    qCInfo(HR) << "Finished:" << exe_cmd;

    if (!terminated)
        qCDebug(HR).noquote() << QString("Elapsed time for running processor (%1): %2 sec").arg(processor_name).arg(timer.elapsed() * 1.0 / 1000);

    /*
    int ret = pp.execute(exe, args);
    if (ret != 0) {
        response["success"] = false;
        response["error"] = "Error running processor.";
        return response;
    }
    */

    bool success0 = true;
    QString error0 = "";

    QJsonObject process_output;
    if (terminated) {
        if (success0) {
            success0 = false;
            process_output["error"] = "Terminated.";
        }
    }
    else if (!QFile::exists(process_output_fname)) {
        if (success0) {
            success0 = false;
            process_output["error"] = "Process output file does not exist: " + process_output_fname;
        }
    }
    else {
        QString json = TextFile::read(process_output_fname);
        QJsonParseError error;
        process_output = QJsonDocument::fromJson(json.toUtf8(), &error).object();
        if (error.error != QJsonParseError::NoError) {
            if (success0) {
                success0 = false;
                process_output["error"] = "Error parsing json in process output file";
            }
        }
    }
    if (!process_output["success"].toBool()) {
        if (success0) {
            success0 = false;
            error0 = "Process error: " + process_output["error"].toString();
        }
    }

    QJsonObject outputs0;
    foreach (QString key, okeys) {
        if (outputs[key].toBool()) {
            QString fname = output_files[key];
            if (!wait_for_file_to_exist(fname, 100)) {
                if (success0) {
                    success0 = false;
                    error0 = "Output file does not exist: " + fname;
                }
            }
            else {
                QString tmp_prv = fname + ".prv";
                if (!create_prv(fname, tmp_prv)) {
                    if (success0) {
                        success0 = false;
                        error0 = "Unable to create prv object for: " + fname;
                    }
                }
                else {
                    QString prv_json = TextFile::read(tmp_prv);
                    if (prv_json.isEmpty()) {
                        if (success0) {
                            success0 = false;
                            error0 = "Problem creating prv object for: " + fname;
                        }
                    }
                    else {
                        outputs0[key] = QJsonDocument::fromJson(prv_json.toUtf8()).object();
                    }
                    QFile::remove(tmp_prv);
                }
            }
        }
    }
    response["outputs"] = outputs0;

    response["success"] = success0;
    response["error"] = error0;

    return response;
}

/*
qint64 run_command_as_bash_script(const QString& exe_command, QString monitor_file_name)
{
    QString bash_script_fname = CacheManager::globalInstance()->makeLocalFile();

    int this_pid = QCoreApplication::applicationPid();

    QString cleanup_cmd = "";
    if (!monitor_file_name.isEmpty()) {
        cleanup_cmd = "rm " + monitor_file_name;
    }

    QString script;
    script += QString("#!/bin/bash\n\n");
    script += QString(exe_command + " &\n"); //run the command
    script += QString("cmdpid=$!\n"); //get the pid of the exe_command
    //script += QString("trap \"kill $cmdpid; %1; exit 255;\" SIGINT SIGTERM\n").arg(cleanup_cmd); //capture the terminate signal and pass it on
    script += QString("while kill -0 %1 >/dev/null 2>&1; do\n").arg(this_pid); //while the (parent) pid still exists
    script += QString("    if kill -0 $cmdpid > /dev/null 2>&1; then\n");
    script += QString("        sleep 1;\n"); //we are still going
    if (!monitor_file_name.isEmpty()) {
        script += QString("        touch %1;\n").arg(monitor_file_name); //touch the monitor file
        script += QString("        if [ -e \"%1.stop\" ]; then\n").arg(monitor_file_name);
        script += QString("          kill $cmdpid\n"); //if a stop file exists, then kill the process
        script += QString("        fi\n");
    }
    script += QString("    else\n"); //else the exe process is done
    if (!cleanup_cmd.isEmpty()) {
        script += QString("        %1\n").arg(cleanup_cmd);
    }
    script += QString("        wait $cmdpid\n"); //get the return code for the process that has already completed
    script += QString("        exit $?\n");
    script += QString("    fi\n");
    script += QString("done ;\n");
    script += QString("kill $cmdpid\n"); //the parent pid is gone
    if (!cleanup_cmd.isEmpty()) {
        script += QString("%1\n").arg(cleanup_cmd);
    }
    script += QString("exit 255\n"); //return error exit code

    TextFile::write(bash_script_fname, script);

    qint64 pid;
    QProcess::startDetached("/bin/bash",QStringList(bash_script_fname),"",&pid);
    //qprocess->start("/bin/bash", QStringList(bash_script_fname));
    CacheManager::globalInstance()->setTemporaryFileExpirePid(bash_script_fname, pid);
    return pid;
}
*/

QString compute_unique_object_code(QJsonObject obj)
{
    /// Witold I need a string that depends on the json object. However I am worried about the order of the fields. Is there a way to make this canonical?
    /// Jeremy: You can sort all keys in the dictionary, convert that to string and
    ///         calculate hash of that. However this is going to be CPU consuming
    QByteArray json = QJsonDocument(obj).toJson();
    QCryptographicHash hash(QCryptographicHash::Sha1);
    hash.addData(json);
    return QString(hash.result().toHex());
}

void sleep_msec(int msec)
{
    int microseconds = msec * 1000;
    usleep(microseconds);
}

void touch(const QString& filePath)
{
    QProcess::execute("touch", QStringList(filePath));
}

static bool s_terminate_requested = false;
void set_terminate_requested(bool val)
{
    s_terminate_requested = val;
}

bool terminate_requested()
{
    return s_terminate_requested;
}
