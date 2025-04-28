#include <exec/types.h>
#include <exec/io.h>
#include <exec/errors.h>
#include <devices/trackdisk.h>
#include <devices/scsidisk.h>
#include <proto/exec.h>
#include "config.h"

#ifdef DEBUG
extern void KPrintF(char *str, ...);
#endif

struct MsgPort *msgPort = NULL;
struct IOExtTD *tdRequest = NULL;
BOOL deviceOpened = FALSE;

int  __saveds __asm __UserDevInit(register __d0 long unit,
                                  register __a0 struct IORequest *ior,
                                  register __a6 struct MyLibrary *libbase)
{
    readConfigFile();

    msgPort = CreatePort(NULL, 0);

    if(msgPort == NULL)
        return 1;
    else
    {
        tdRequest = (struct IOExtTD*)CreateExtIO(msgPort, sizeof(struct IOExtTD));

        if(tdRequest == NULL)
            return 1;
        else
        {
            if(OpenDevice(config.device, unit, (struct IORequest*)tdRequest, 0) == 0)
            {
#ifdef DEBUG
                KPrintF("Target device: %s opened successfully at unit: %ld\n", config.device, unit);
#endif
                deviceOpened = TRUE;
                return 0;
            }
            else
                return 1;
        }
    }
}

void __saveds __asm __UserDevCleanup(register __a0 struct IORequest *ior,
                                     register __a6 struct MyLibrary *libbase)
{
#ifdef DEBUG
    KPrintF("Cleanup\n");
#endif

    if(deviceOpened)
        CloseDevice((struct IORequest*)tdRequest);

    if(tdRequest != NULL)
        DeleteExtIO((struct IORequest*)tdRequest);

    if(msgPort != NULL)
        DeletePort(msgPort);
}

static void reverseWordOrder(UBYTE *data, ULONG dataLength)
{
    ULONG i;

    for(i = 1; i < dataLength; i += 2)
    {
        UBYTE remember = data[i - 1];
        data[i - 1] = data[i];
        data[i] = remember;
    }
}

static ULONG translateOffset(ULONG virtualOffset)
{
    return virtualOffset + config.kcsPartitionOffset;
}

void __saveds __asm DevBeginIO(register __a1 struct IORequest *ior)
{
    struct IOExtTD *tdOrigRequest = (struct IOExtTD *)ior;
    struct IOStdReq *origRequest = &tdOrigRequest->iotd_Req;
    struct IOStdReq *ioRequest = &tdRequest->iotd_Req;

    /* Compose translated I/O request */
    ioRequest->io_Command = origRequest->io_Command;
    ioRequest->io_Flags = origRequest->io_Flags;

#ifdef DEBUG
    KPrintF("Command: %ld\n", ior->io_Command);
    KPrintF("Flags: %x\n", ior->io_Flags);
#endif

    switch(ior->io_Command)
    {
        case HD_SCSICMD:
        case TD_ADDCHANGEINT:
        case TD_GETGEOMETRY:
        case TD_REMCHANGEINT:
#ifdef DEBUG
            KPrintF("Length: %ld\n", origRequest->io_Length);
#endif
            ioRequest->io_Length = origRequest->io_Length;
            ioRequest->io_Data = origRequest->io_Data;
            break;
        case CMD_READ:
        case TD_RAWREAD:
#ifdef DEBUG
            KPrintF("Length: %ld\n", origRequest->io_Length);
            KPrintF("Offset: %ld\n", origRequest->io_Offset);
#endif
            ioRequest->io_Length = origRequest->io_Length;
            ioRequest->io_Data = origRequest->io_Data;
            ioRequest->io_Offset = translateOffset(origRequest->io_Offset);
            break;
        case CMD_WRITE:
        case TD_FORMAT:
        case TD_RAWWRITE:
#ifdef DEBUG
            KPrintF("Length: %ld\n", origRequest->io_Length);
            KPrintF("Offset: %ld\n", origRequest->io_Offset);
#endif
            ioRequest->io_Length = origRequest->io_Length;
            ioRequest->io_Data = origRequest->io_Data;
            reverseWordOrder((UBYTE*)ioRequest->io_Data, ioRequest->io_Length);
            ioRequest->io_Offset = translateOffset(origRequest->io_Offset);
            break;
        case TD_SEEK:
            ioRequest->io_Offset = translateOffset(origRequest->io_Offset);
            break;
        case TD_MOTOR:
#ifdef DEBUG
            KPrintF("Length: %ld\n", origRequest->io_Length);
#endif
            ioRequest->io_Length = origRequest->io_Length;
            break;
        case ETD_CLEAR:
        case ETD_UPDATE:
            tdRequest->iotd_Count = tdOrigRequest->iotd_Count;
            break;
        case ETD_READ:
        case ETD_RAWREAD:
#ifdef DEBUG
            KPrintF("Length: %ld\n", origRequest->io_Length);
            KPrintF("Offset: %ld\n", origRequest->io_Offset);
#endif
            ioRequest->io_Length = origRequest->io_Length;
            ioRequest->io_Data = origRequest->io_Data;
            ioRequest->io_Offset = translateOffset(origRequest->io_Offset);
            tdRequest->iotd_Count = tdOrigRequest->iotd_Count;
            break;
        case ETD_WRITE:
        case ETD_RAWWRITE:
#ifdef DEBUG
            KPrintF("Length: %ld\n", origRequest->io_Length);
            KPrintF("Offset: %ld\n", origRequest->io_Offset);
#endif
            ioRequest->io_Length = origRequest->io_Length;
            ioRequest->io_Data = origRequest->io_Data;
            reverseWordOrder((UBYTE*)ioRequest->io_Data, ioRequest->io_Length);
            ioRequest->io_Offset = translateOffset(origRequest->io_Offset);
            tdRequest->iotd_Count = tdOrigRequest->iotd_Count;
            break;
        case ETD_FORMAT:
#ifdef DEBUG
            KPrintF("Length: %ld\n", origRequest->io_Length);
            KPrintF("Offset: %ld\n", origRequest->io_Offset);
#endif
            ioRequest->io_Length = origRequest->io_Length;
            ioRequest->io_Data = origRequest->io_Data;
            reverseWordOrder((UBYTE*)ioRequest->io_Data, ioRequest->io_Length);
            ioRequest->io_Offset = translateOffset(origRequest->io_Offset);
            tdRequest->iotd_Count = tdOrigRequest->iotd_Count;
            tdRequest->iotd_SecLabel = tdOrigRequest->iotd_SecLabel;
            break;
        case ETD_MOTOR:
#ifdef DEBUG
            KPrintF("Length: %ld\n", origRequest->io_Length);
#endif
            ioRequest->io_Length = origRequest->io_Length;
            tdRequest->iotd_Count = tdOrigRequest->iotd_Count;
            break;
        case ETD_SEEK:
#ifdef DEBUG
            KPrintF("Offset: %ld\n", origRequest->io_Offset);
#endif
            ioRequest->io_Offset = translateOffset(origRequest->io_Offset);
            tdRequest->iotd_Count = tdOrigRequest->iotd_Count;
            break;
        default:
            ior->io_Error = IOERR_NOCMD;
#ifdef DEBUG
            KPrintF("Error: %ld\n\n", ior->io_Error);
#endif
            ReplyMsg(&ior->io_Message);
            return;
    }

    /* Do the I/O */

    if(ior->io_Command == TD_ADDCHANGEINT)
    {
        /* Some SCSI drivers seem to crash when doing this operation. Disabling it seems to do no harm */
        /*SendIO((struct IORequest*)ioRequest);*/ /* This operation never sends a notification. Therefore, we must use SendIO() rather than DoIO() */

#ifdef DEBUG
        KPrintF("No return message expected\n\n");
#endif
        return;
    }
    else
        DoIO((struct IORequest*)ioRequest);

    /* Process result */
    origRequest->io_Error = ioRequest->io_Error;

    switch(ior->io_Command)
    {
        case CMD_READ:
        case ETD_READ:
#ifdef DEBUG
            KPrintF("Actual: %ld\n", ioRequest->io_Actual);
#endif
            origRequest->io_Actual = ioRequest->io_Actual;
            reverseWordOrder((UBYTE*)origRequest->io_Data, ioRequest->io_Actual);
            break;
        case CMD_WRITE:
        case ETD_WRITE:
        case TD_FORMAT:
        case TD_RAWREAD:
        case TD_RAWWRITE:
        case ETD_RAWREAD:
        case ETD_RAWWRITE:
            reverseWordOrder((UBYTE*)origRequest->io_Data, ioRequest->io_Length);
            break;
        case TD_PROTSTATUS:
        case TD_CHANGENUM:
        case TD_CHANGESTATE:
        case TD_GETDRIVETYPE:
        case TD_GETNUMTRACKS:
        case TD_MOTOR:
        case ETD_MOTOR:
#ifdef DEBUG
            KPrintF("Actual: %ld\n", ioRequest->io_Actual);
#endif
            origRequest->io_Actual = ioRequest->io_Actual;
            break;
    }

#ifdef DEBUG
    KPrintF("Error: %ld\n\n", ior->io_Error);
#endif

    /* Send notification to the caller */

    ReplyMsg(&ior->io_Message);
}

void __saveds __asm DevAbortIO(register __a1 struct IORequest *ior)
{
}
