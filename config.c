#include "config.h"
#include <proto/dos.h>
#include <proto/exec.h>
#include <stdlib.h>

#define BUFFER_SIZE 512

#ifdef DEBUG
extern void KPrintF(char *str, ...);
#endif

struct KCSHDProxyConfig config = { 0, "scsi.device" };

enum FieldPosition
{
    POSITION_OFFSET = 0,
    POSITION_DEVICE = 1
};

void readConfigFile(void)
{
    BPTR file = Open("S:KCSHDProxy-Config", MODE_OLDFILE);

    if(file == DOSFALSE)
    {
#ifdef DEBUG
        LONG error = IoErr();
        KPrintF("Cannot open config file: %ld\n", error);
#endif
    }
    else
    {
        char buffer[BUFFER_SIZE];
        LONG actualLength = Read(file, buffer, BUFFER_SIZE);
        LONG i;
        LONG beginPos = 0;
        enum FieldPosition fieldPosition = 0;

        for(i = 0; i < actualLength; i++)
        {
            if(buffer[i] == '\n')
            {
                char *str = buffer + beginPos;
                LONG length = i - beginPos;

                buffer[i] = '\0';

                switch(fieldPosition)
                {
                    case POSITION_OFFSET:
                        config.kcsPartitionOffset = atoi(str);
                        break;
                    case POSITION_DEVICE:
                        CopyMem(str, config.device, length + 1);
                        break;
                }

                beginPos = i + 1;
                fieldPosition++;
            }
        }

        Close(file);
    }

#ifdef DEBUG
    KPrintF("config KCS partition offset: %ld\n", config.kcsPartitionOffset);
    KPrintF("config device: %s\n\n", config.device);
#endif
}
