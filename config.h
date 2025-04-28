#ifndef __CONFIG_H
#define __CONFIG_H

#include <exec/types.h>

#define MAX_FILENAME_LENGTH 31

struct KCSHDProxyConfig
{
    ULONG kcsPartitionOffset;
    char device[MAX_FILENAME_LENGTH];
};

extern struct KCSHDProxyConfig config;

void readConfigFile(void);

#endif
