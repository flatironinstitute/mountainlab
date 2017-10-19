/******************************************************
** See the accompanying README and LICENSE files
** Author(s): Jeremy Magland
** Created: 7/4/2016
*******************************************************/

#include "mlcommon.h"

#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <diskreadmda.h>
#include "diskwritemda.h"

void print_usage();

QString get_default_format(QString path);
QString get_info_string(const DiskReadMda& X);
QString get_json_header_string(const DiskReadMda& X);
bool extract_time_chunk(QString input_fname, QString output_fname, const QMap<QString, QVariant>& params);

void silent_message_output(QtMsgType type, const QMessageLogContext& context, const QString& msg)
{
    Q_UNUSED(type)
    Q_UNUSED(context)
    Q_UNUSED(msg)
    return;
}

QList<int> parse_int_list(QString str);
int parse_data_type(QString str);

int main(int argc, char* argv[])
{
    CLParams params(argc, argv);

    if (params.unnamed_parameters.count() < 1) {
        print_usage();
        return 0;
    }

    QString arg1 = params.unnamed_parameters.value(0);
    QString arg2 = params.unnamed_parameters.value(1);
    QString arg3 = params.unnamed_parameters.value(2);

    if (arg1 == "get_chunk") {
        if (!arg2.endsWith(".mda")) {
            print_usage();
            return -1;
        }
        if (!arg3.endsWith(".mda")) {
            print_usage();
            return -1;
        }
        if (!QFile::exists(arg2)) {
            printf("Input file does not exist.\n");
            return -1;
        }
        QList<int> index = parse_int_list(params.named_parameters["index"].toString());
        QList<int> size = parse_int_list(params.named_parameters["size"].toString());
        if ((index.count() < 2) || (index.count() > 3)) {
            print_usage();
            return -1;
        }
        if (index.count() != size.count()) {
            return -1;
        }

        DiskReadMda X(arg2);
        Mda chunk;
        if (index.count() == 2) {
            if (!X.readChunk(chunk, index[0], index[1], size[0], size[1]))
                return -1;
        }
        else if (index.count() == 3) {
            if (!X.readChunk(chunk, index[0], index[1], index[2], size[0], size[1], size[2]))
                return -1;
        }
        else {
            print_usage();
            return -1;
        }
        int format = X.mdaioHeader().data_type;
        bool ret = false;
        if (format == MDAIO_TYPE_BYTE)
            ret = chunk.write8(arg3);
        else if (format == MDAIO_TYPE_INT16)
            ret = chunk.write16i(arg3);
        else if (format == MDAIO_TYPE_INT32)
            ret = chunk.write32i(arg3);
        else if (format == MDAIO_TYPE_UINT16)
            ret = chunk.write16ui(arg3);
        else if (format == MDAIO_TYPE_UINT32)
            ret = chunk.write32ui(arg3);
        else if (format == MDAIO_TYPE_FLOAT32)
            ret = chunk.write32(arg3);
        else if (format == MDAIO_TYPE_FLOAT64)
            ret = chunk.write64(arg3);
        else {
            print_usage();
            return -1;
        }
        if (!ret)
            return -1;
        return 0;
    }
    else if (arg1 == "create") {
        QList<int> size = parse_int_list(params.named_parameters["size"].toString());
        if (!arg2.endsWith(".mda")) {
            print_usage();
            return -1;
        }
        if (size.count() < 2) {
            printf("Unable to create file. Invalid size.\n");
            return -1;
        }
        int data_type = parse_data_type(params.named_parameters["dtype"].toString());
        if (!data_type) {
            print_usage();
            return -1;
        }
        while (size.count() < 6)
            size << 1;
        DiskWriteMda X;
        X.open(data_type, arg2, size[0], size[1], size[2], size[3], size[4], size[5]);
        X.close();
        ;
    }
    else if (arg1 == "set_chunk") {
        if (!arg2.endsWith(".mda")) {
            print_usage();
            return -1;
        }
        if (!arg3.endsWith(".mda")) {
            print_usage();
            return -1;
        }
        if (!QFile::exists(arg2)) {
            printf("Target file does not exist.\n");
            return -1;
        }
        QList<int> index = parse_int_list(params.named_parameters["index"].toString());

        DiskWriteMda Y;
        Y.open(arg2);
        Mda chunk;
        if (!chunk.read(arg3)) {
            printf("Unable to read chunk.\n");
            Y.close();
            return -1;
        }
        if (index.count() == 2) {
            if (!Y.writeChunk(chunk, index[0], index[1])) {
                printf("Problem writing chunk.\n");
                Y.close();
                return -1;
            }
        }
        else if (index.count() == 3) {
            if (!Y.writeChunk(chunk, index[0], index[1], index[2])) {
                printf("Problem writing chunk.\n");
                Y.close();
                return -1;
            }
        }
        else {
            printf("Unexpected length of index vector: %d.\n", index.count());
            Y.close();
            return -1;
        }
        Y.close();

        return 0;
    }
    else {
        qInstallMessageHandler(silent_message_output);
        DiskReadMda X(arg1);
        QString str = get_json_header_string(X);
        printf("%s\n", str.toUtf8().data());
        return 0;
    }

    /*
    if (arg1 == "extract_time_chunk") {
        if (extract_time_chunk(arg2, arg3, params.named_parameters))
            return 0;
        else
            return -1;
    }

    mdaconvert_opts opts;

    opts.input_path = params.unnamed_parameters.value(0);
    opts.input_dtype = params.named_parameters.value("input_dtype").toString();
    opts.input_format = params.named_parameters.value("input_format").toString();

    opts.output_path = params.unnamed_parameters.value(1);
    opts.output_dtype = params.named_parameters.value("output_dtype").toString();
    opts.output_format = params.named_parameters.value("output_format").toString();

    if (params.named_parameters.contains("allow-subset"))
        opts.check_input_file_size = false;

    if (opts.output_path.isEmpty()) {
        //if (opts.input_path.endsWith(".mda")) {
        DiskReadMda X(opts.input_path);
        if (params.named_parameters.contains("readheader")) {
            QString str = get_json_header_string(X);
            printf("%s\n", str.toUtf8().data());
            return 0;
        }
        else {
            QString str = get_info_string(X);
            printf("%s\n", str.toUtf8().data());
            return 0;
        }
        //}
        //else {
        //    print_usage();
        //    return -1;
        //}
    }

    if (params.named_parameters.contains("dtype")) {
        opts.input_dtype = params.named_parameters.value("dtype").toString();
        opts.output_dtype = params.named_parameters.value("dtype").toString();
    }

    if (params.named_parameters.contains("input_num_header_rows")) {
        opts.input_num_header_rows = params.named_parameters["input_num_header_rows"].toInt();
    }
    if (params.named_parameters.contains("input_num_header_cols")) {
        opts.input_num_header_cols = params.named_parameters["input_num_header_cols"].toInt();
    }

    if (opts.input_format.isEmpty()) {
        opts.input_format = get_default_format(opts.input_path);
    }
    if (opts.output_format.isEmpty()) {
        opts.output_format = get_default_format(opts.output_path);
    }

    QStringList dims_strlist = params.named_parameters.value("dims", "").toString().split("x", QString::SkipEmptyParts);
    foreach (QString str, dims_strlist) {
        opts.dims << str.toLong();
    }
    opts.num_channels = params.named_parameters.value("num_channels", 0).toInt();

    printf("Converting %s --> %s\n", opts.input_path.toLatin1().data(), opts.output_path.toLatin1().data());
    if (!mdaconvert(opts)) {
        return -1;
    }

    return 0;
    */
}

QString get_suffix(QString path)
{
    int index1 = path.lastIndexOf(".");
    if (index1 < 0)
        return "";
    int index2 = path.lastIndexOf("/");
    if ((index2 >= 0) && (index2 > index1))
        return "";
    return path.mid(index1 + 1);
}

QString get_default_format(QString path)
{
    QString suf = get_suffix(path);
    if (suf == "mda")
        return "mda";
    else if (suf == "csv")
        return "csv";
    else if (suf.toLower() == "nrd") {
        return "nrd";
    }
    else if (suf.toLower() == "ncs") {
        return "ncs";
    }
    else
        return "raw";
}

void print_usage()
{
    printf("Example usages:\n");
    printf("mda file.mda\n");
    printf("mda get_chunk file.mda chunk_out.mda --index=0,100 --size=4x50\n");
    printf("mda create file.mda --dtype=int16 --size=4x1000\n");
    printf("mda set_chunk target_file.mda chunk.mda --index=0,100\n");
    /*
    printf("Example usages for converting between raw and mda formats:\n");
    printf("mdaconvert input.mda\n");
    printf("mdaconvert input.dat output.mda --dtype=uint16 --dims=32x100x44\n");
    printf("mdaconvert input.mda output.mda --dtype=int16 --input_format=raw_timeseries --num_channels=32\n");
    printf("mdaconvert input.file output.file --input_format=dat --input_dtype=float64 --output_format=mda --output_dtype=float32\n");
    printf("mdaconvert input.csv output.mda --input_num_header_rows=1 --input_num_header_cols=0\n");
    printf("mdaconvert input.ncs output.mda\n");
    printf("mdaconvert input.nrd output.mda --num_channels=32\n");
    */
}

#define MDAIO_MAX_DIMS 50
#define MDAIO_MAX_SIZE 1e15
#define MDAIO_TYPE_COMPLEX -1
#define MDAIO_TYPE_BYTE -2
#define MDAIO_TYPE_FLOAT32 -3
#define MDAIO_TYPE_INT16 -4
#define MDAIO_TYPE_INT32 -5
#define MDAIO_TYPE_UINT16 -6
#define MDAIO_TYPE_FLOAT64 -7
#define MDAIO_TYPE_UINT32 -8

QString mdaio_type_string(int dtype)
{
    if (dtype == MDAIO_TYPE_COMPLEX)
        return "complex";
    else if (dtype == MDAIO_TYPE_FLOAT32)
        return "float32";
    else if (dtype == MDAIO_TYPE_FLOAT64)
        return "float64";
    else if (dtype == MDAIO_TYPE_BYTE)
        return "byte";
    else if (dtype == MDAIO_TYPE_UINT16)
        return "uint16";
    else if (dtype == MDAIO_TYPE_UINT32)
        return "uint32";
    else if (dtype == MDAIO_TYPE_INT16)
        return "int16";
    else if (dtype == MDAIO_TYPE_INT32)
        return "int32";
    else
        return QString("unknown type %1").arg(dtype);
}

QString get_info_string(const DiskReadMda& X)
{
    QString str = QString("%1x%2").arg(X.N1()).arg(X.N2());
    if (X.N6() > 1) {
        str += QString("x%1x%2x%3x%4").arg(X.N3()).arg(X.N4()).arg(X.N5()).arg(X.N6());
    }
    else if (X.N5() > 1) {
        str += QString("x%1x%2x%3").arg(X.N3()).arg(X.N4()).arg(X.N5());
    }
    else if (X.N4() > 1) {
        str += QString("x%1x%2").arg(X.N3()).arg(X.N4());
    }
    else if (X.N3() > 1) {
        str += QString("x%1").arg(X.N3());
    }
    MDAIO_HEADER header = X.mdaioHeader();
    str += QString(" (%1)").arg(mdaio_type_string(header.data_type));
    return str;
}

QString get_data_type_string(int data_type)
{
    if (data_type == MDAIO_TYPE_BYTE)
        return "byte";
    if (data_type == MDAIO_TYPE_FLOAT32)
        return "float32";
    if (data_type == MDAIO_TYPE_FLOAT64)
        return "float64";
    if (data_type == MDAIO_TYPE_INT16)
        return "int16";
    if (data_type == MDAIO_TYPE_INT32)
        return "int32";
    if (data_type == MDAIO_TYPE_UINT16)
        return "uint16";
    if (data_type == MDAIO_TYPE_UINT32)
        return "uint32";
    return "";
}

QString get_json_header_string(const DiskReadMda& X)
{
    MDAIO_HEADER H = X.mdaioHeader();
    QJsonObject ret;
    ret["num_dims"] = H.num_dims;
    QJsonArray dims0;
    for (int i = 0; i < H.num_dims; i++) {
        dims0.push_back((long long)H.dims[i]);
    }
    ret["dims"] = dims0;
    ret["num_bytes_per_entry"] = H.num_bytes_per_entry;
    ret["header_size"] = (int)H.header_size;
    ret["data_type"] = H.data_type;
    ret["data_type_string"] = get_data_type_string(H.data_type);
    return QJsonDocument(ret).toJson(QJsonDocument::Indented);
}

bool extract_time_chunk(QString input_fname, QString output_fname, const QMap<QString, QVariant>& params)
{
    DiskReadMda X(input_fname);
    Mda Y;

    bigint t1 = params["t1"].toLongLong();
    bigint t2 = params["t2"].toLongLong();

    if (!X.readChunk(Y, 0, t1, X.N1(), t2 - t1 + 1)) {
        qWarning() << "Problem reading chunk.";
        return false;
    }
    MDAIO_HEADER H = X.mdaioHeader();
    if (H.data_type == MDAIO_TYPE_FLOAT32)
        return Y.write32(output_fname);
    else if (H.data_type == MDAIO_TYPE_FLOAT64)
        return Y.write64(output_fname);
    else if (H.data_type == MDAIO_TYPE_INT16)
        return Y.write16i(output_fname);
    else if (H.data_type == MDAIO_TYPE_UINT16)
        return Y.write16ui(output_fname);
    else if (H.data_type == MDAIO_TYPE_INT32)
        return Y.write16i(output_fname);
    else if (H.data_type == MDAIO_TYPE_UINT32)
        return Y.write16ui(output_fname);
    else if (H.data_type == MDAIO_TYPE_BYTE)
        return Y.write8(output_fname);
    else {
        qWarning() << "Unexpected data type: " << H.data_type;
        return false;
    }
}

QList<int> parse_int_list(QString str)
{
    QList<int> ret;
    if (str.isEmpty())
        return ret;
    str = str.split("x").join(",");
    QStringList vals = str.split(",");
    foreach (QString val, vals) {
        ret << val.toLong();
    }
    return ret;
}

int parse_data_type(QString format)
{
    if (format == "byte")
        return MDAIO_TYPE_BYTE;
    if (format == "int8")
        return MDAIO_TYPE_BYTE;
    if (format == "int16")
        return MDAIO_TYPE_INT16;
    if (format == "int32")
        return MDAIO_TYPE_INT32;
    if (format == "uint16")
        return MDAIO_TYPE_UINT16;
    if (format == "float32")
        return MDAIO_TYPE_FLOAT32;
    if (format == "float64")
        return MDAIO_TYPE_FLOAT64;
    if (format == "int32")
        return MDAIO_TYPE_INT32;
    return 0;
}
