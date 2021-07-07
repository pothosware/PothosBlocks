// Copyright (c) 2020 Nicholas Corgan
// SPDX-License-Identifier: BSL-1.0

#pragma once

#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#ifdef _MSC_VER
#include <io.h>
#else
#include <unistd.h>
#endif //_MSC_VER
#include <stdio.h>
#include <cerrno>

#ifndef O_BINARY
#define O_BINARY 0
#endif

#include <sys/stat.h> //O_CREAT mode defines
#ifdef _MSC_VER
#define MY_S_IREADWRITE (_S_IREAD | _S_IWRITE)
#else
#define MY_S_IREADWRITE (S_IRUSR | S_IWUSR)
#endif

inline int openFileForRead(const char* filepath)
{
    return open(filepath, O_RDONLY | O_BINARY);
}

inline int openFileForWrite(const char* filepath)
{
    return open(filepath, O_WRONLY | O_CREAT | O_TRUNC | O_BINARY, MY_S_IREADWRITE);
}
