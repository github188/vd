#ifndef __VE_H
#define __VE_H

#include "types.h"
#include "svn_version.h"


C_CODE_BEGIN

#define VE_MAJOR_VERSION	0
#define VE_MINOR_VERSION		5
#define VE_REVISION_VERSION		1
#define VE_BUILD_VERSION		SVN_VERSION

int32_t ve_init(void);

C_CODE_END


#endif
