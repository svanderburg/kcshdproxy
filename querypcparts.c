#include <exec/io.h>
#include <proto/exec.h>
#include <stdio.h>
#include <stdlib.h>

#define BUFFER_SIZE 512

#define OFFSET_PARTITION_1_ENTRY 0x1be
#define OFFSET_PARTITION_2_ENTRY 0x1ce
#define OFFSET_PARTITION_3_ENTRY 0x1de
#define OFFSET_PARTITION_4_ENTRY 0x1ee
#define OFFSET_SIGNATURE_1       0x1fe
#define OFFSET_SIGNATURE_2       0x1ff

#define OFFSET_TYPE           0x4
#define OFFSET_LBA_FIELD      0x8
#define OFFSET_NUM_OF_SECTORS 0xc

static ULONG byteSwapULong(UBYTE *value)
{
    return ((ULONG)value[0]) | ((ULONG)value[1] << 8) | ((ULONG)value[2] << 16) | ((ULONG)value[3] << 24);
}

static int checkFATPartition(UBYTE type)
{
    return type == 0x1 || type == 0x4 || type == 0x6 || type == 0xb || type == 0xc || type == 0xe;
}

static void printOffset(unsigned int partitionNumber, UBYTE *partitionEntry)
{
    UBYTE type = partitionEntry[OFFSET_TYPE];
    ULONG lbaOfFirstAbsoluteSector = byteSwapULong(partitionEntry + OFFSET_LBA_FIELD);
    ULONG numOfSectors = byteSwapULong(partitionEntry + OFFSET_NUM_OF_SECTORS);

    if(checkFATPartition(type))
        printf("Partition: %u, first sector offset: %u, number of sectors: %u\n", partitionNumber, lbaOfFirstAbsoluteSector, numOfSectors);
}

static int checkMBRSignature(UBYTE signature1, UBYTE signature2)
{
    return signature1 == 0x55 && signature2 == 0xaa;
}

int main(int argc, char *argv[])
{
    ULONG unitNumber;
    int exit_status;
    struct MsgPort *msgPort = CreatePort(NULL, 0);

    if(argc >= 2)
        unitNumber = atoi(argv[1]);
    else
        unitNumber = 0;

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
            if(OpenDevice("kcshdproxy.device", unitNumber, (struct IORequest*)ioRequest, 0) == 0)
            {
                UBYTE readBuffer[BUFFER_SIZE];

                ioRequest->io_Command = CMD_READ;
                ioRequest->io_Length = BUFFER_SIZE;
                ioRequest->io_Data = (APTR)readBuffer;
                ioRequest->io_Offset = 0;

                if(DoIO((struct IORequest*)ioRequest) == 0)
                {
                    if(ioRequest->io_Actual < BUFFER_SIZE)
                    {
                        fprintf(stderr, "We require a block of %u bytes for the MBR! Instead, we have read: %u bytes!\n", BUFFER_SIZE, ioRequest->io_Actual);
                        exit_status = 1;
                    }
                    else
                    {
                        UBYTE signature1 = readBuffer[OFFSET_SIGNATURE_1];
                        UBYTE signature2 = readBuffer[OFFSET_SIGNATURE_2];

                        /* Check if we have read a valid MBR partition table */

                        if(checkMBRSignature(signature1, signature2))
                        {
                            /* Print the offset of the partitions (max 4) */
                            printOffset(1, readBuffer + OFFSET_PARTITION_1_ENTRY);
                            printOffset(2, readBuffer + OFFSET_PARTITION_2_ENTRY);
                            printOffset(3, readBuffer + OFFSET_PARTITION_3_ENTRY);
                            printOffset(4, readBuffer + OFFSET_PARTITION_4_ENTRY);

                            exit_status = 0;
                        }
                        else
                        {
                            fprintf(stderr, "Failed to read valid MBR! Last two bytes are: 0x%x, 0x%x\n", signature1, signature2);
                            exit_status = 1;
                        }
                    }
                }
                else
                {
                    fprintf(stderr, "Failed to do I/O! Error is: %d\n", ioRequest->io_Error);
                    exit_status = 1;
                }

                CloseDevice((struct IORequest*)ioRequest);
            }

            DeleteExtIO((struct IORequest*)ioRequest);
        }

        DeletePort(msgPort);
    }

    return exit_status;
}
