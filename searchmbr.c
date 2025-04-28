#include <exec/io.h>
#include <proto/exec.h>
#include <stdio.h>
#include <stdlib.h>

#define OFFSET_SIGNATURE_1       0x1fe
#define OFFSET_SIGNATURE_2       0x1ff

#define BUFFER_SIZE 512

static int checkMBRSignature(UBYTE *buffer)
{
    return buffer[OFFSET_SIGNATURE_1] == 0xaa && buffer[OFFSET_SIGNATURE_2] == 0x55;
}

int main(int argc, char *argv[])
{
    char *device;
    ULONG unitNumber;
    ULONG offset;

    int exit_status = 1;
    struct MsgPort *msgPort = CreatePort(NULL, 0);

    if(argc >= 2)
        device = argv[1];
    else
        device = "scsi.device";

    if(argc >= 3)
        unitNumber = atoi(argv[2]);
    else
        unitNumber = 0;

    if(argc >= 4)
        offset = atoi(argv[3]);
    else
        offset = 0;

    if(msgPort == NULL)
    {
        fprintf(stderr, "Cannot create message port!\n");
        exit_status = 1;
    }
    else
    {
        struct IOStdReq *ioRequest = (struct IOStdReq*)CreateExtIO(msgPort, sizeof(struct IOStdReq));

        if(ioRequest == NULL)
        {
            fprintf(stderr, "Cannot create IORequest!\n");
            exit_status = 1;
        }
        else
        {
            if(OpenDevice(device, unitNumber, (struct IORequest*)ioRequest, 0) == 0)
            {
                while(TRUE)
                {
                    UBYTE readBuffer[BUFFER_SIZE];

                    fprintf(stderr, "Checking block at offset: %u\n", offset);

                    ioRequest->io_Command = CMD_READ;
                    ioRequest->io_Length = BUFFER_SIZE;
                    ioRequest->io_Data = (APTR)readBuffer;
                    ioRequest->io_Offset = offset;

                    if(DoIO((struct IORequest*)ioRequest) == 0)
                    {
                        if(ioRequest->io_Actual < BUFFER_SIZE)
                        {
                            fprintf(stderr, "We require a block of %u bytes for the MBR! Instead, we have read: %u bytes!\n", BUFFER_SIZE, ioRequest->io_Actual);
                            exit_status = 1;
                            break;
                        }
                        else
                        {
                            if(checkMBRSignature(readBuffer))
                            {
                                printf("MBR found at offset: %u\n", offset);
                                exit_status = 0;
                                break;
                            }
                        }
                    }
                    else
                    {
                        fprintf(stderr, "Failed to do I/O! Error is: %d\n", ioRequest->io_Error);
                        exit_status = 1;
                    }

                    offset += BUFFER_SIZE;
                }

                CloseDevice((struct IORequest*)ioRequest);
            }

            DeleteExtIO((struct IORequest*)ioRequest);
        }

        DeletePort(msgPort);
    }

    return exit_status;
}
