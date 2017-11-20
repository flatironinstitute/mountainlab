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

#ifndef MDACONVERT_H
#define MDACONVERT_H

#include <QDebug>
#include "mlcommon.h"

struct mdaconvert_opts {
    QString input_path;
    QString input_dtype; // uint16, float32, ...
    QString input_format; // mda, raw, ...
    int input_num_header_rows = 0; //for csv
    int input_num_header_cols = 0; //for csv
    int num_channels = 0; //for nrd

    QString output_path;
    QString output_dtype; // uint16, float32, ...
    QString output_format; // mda, raw, ...

    QList<bigint> dims;

    bool check_input_file_size = true;
};
bool mdaconvert(const mdaconvert_opts& opts);

#endif // MDACONVERT_H
