// Copyright (c) 2020 Nicholas Corgan
// SPDX-License-Identifier: BSL-1.0

#pragma once

#include <errno.h>
#include <fcntl.h>
#include <stdio.h>

#include <sys/types.h>
#include <sys/stat.h> // O_CREAT mode defines

#ifdef _MSC_VER
#include <io.h>
#else
#include <unistd.h>
#endif //_MSC_VER

#ifndef O_BINARY
#define O_BINARY 0
#endif

#ifdef _MSC_VER
#define MY_S_IREADWRITE (_S_IREAD | _S_IWRITE)
#else
#define MY_S_IREADWRITE (S_IRUSR | S_IWUSR)
#endif

#define FDSINK_OPENFLAGS   (O_WRONLY | O_CREAT | O_TRUNC | O_BINARY)
#define FDSOURCE_OPENFLAGS (O_RDONLY | O_BINARY)

static inline int openSourceFD(const char* path)
{
    return open(path, FDSOURCE_OPENFLAGS);
}

static inline int openSinkFD(const char* path)
{
    return open(path, FDSINK_OPENFLAGS, MY_S_IREADWRITE);
}
