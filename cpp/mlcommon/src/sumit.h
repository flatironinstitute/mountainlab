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

#ifndef SUMIT_H
#define SUMIT_H

#include <QString>

/*
Computation of hash checksums. Like sha1sum except applies to folders as well as files and automatically caches computations on the local disk: /tmp/sumit.

In the case of files, outputs the sha1 checksum, equivalent to the output of sha1sum. Local caching is performed (in /tmp/sumit/sha1) so that checksums do not need to be recomputed on subsequent calls with large files. The cache indexing is by device/inode/size/modification_time so there is no problem if files are moved or renamed within the same file system.

In the case of directories, outputs a unique sha1 checksum that depends only on the contents of the directory (not the name or location of the directory). The computation depends on the checksum of each and every file within the directory tree, but again checksums do not need to be recomputed for the files in subsequent calls.
*/

QString sumit(const QString& path, int num_bytes, const QString& temporary_path);
QString sumit_dir(const QString& path, const QString& temporary_path);

#endif // SUMIT_H
