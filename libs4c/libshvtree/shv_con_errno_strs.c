#include <shv/tree/shv_com.h>

const char *shv_con_errno_strs[SHV_ERRNOS_COUNT] = {
    [SHV_NO_ERROR] = "",
    [SHV_PROC_THRD] = "Process thread creation fail",
    [SHV_TLAYER_INIT] = "Tlayer init fail",
    [SHV_TLAYER_READ] = "Read from tlayer fail",
    [SHV_RECONNECTS] = "Too many reconnects",
    [SHV_LOGIN] = "Login to broker fail",
    [SHV_CCPCP_PACK] =  "Error in chainpack packing",
    [SHV_CCPCP_UNPACK] = "Error in chainpack unpacking"
};
