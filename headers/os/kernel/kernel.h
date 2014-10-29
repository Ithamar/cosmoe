#ifndef _COSMOE_OS_SUPPORT_H_
#define _COSMOE_OS_SUPPORT_H_

#include <Debug.h>
#include <OS.h>

#ifdef __cplusplus
extern "C" {
#endif


typedef	struct
{
    thread_id	hThread;
} DR_ThreadDied_s;

#ifdef __cplusplus
}
#endif

#endif /* ndef _COSMOE_OS_SUPPORT_H_ */
