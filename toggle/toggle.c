#include "toggle.h"
#include "commonfuncs.h"
#include "eplate.h"


int32_t toggle_init(void)
{
#if (2 == DEV_TYPE)
	if(0 != eplate_init()){
		TRACE_LOG_SYSTEM("eplate initialze failed!");
		return -1;
	}
#endif
	return 0;
}
