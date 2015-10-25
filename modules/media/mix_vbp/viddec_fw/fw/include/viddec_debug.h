#ifndef VIDDEC_DEBUG_H
#define VIDDEC_DEBUG_H

#ifndef VBP

#ifdef HOST_ONLY
#include <stdio.h>
#include <osal.h>
#define DEB                        OS_PRINT
#define FWTRACE                    OS_PRINT("trace:%s %d\n", __FUNCTION__, __LINE__ );
//  #define DEB(format, args...)
//  #define FWTRACE
#define DEB_FNAME(format, args...) OS_PRINT("%s:  %s[%d]:: " format, __FILE__, __FUNCTION__ , __LINE__ ,  ## args )
#define CDEB(a, format, args...)   if(a != 0) {DEB(format, ##args);}
#else
#define DEB(format, args...)
#define FWTRACE
#define CDEB(a, format, args...)
#define DEB_FNAME(format, args...)
#endif

#else  // VBP is defined

#define DEB(format, args...)
#define FWTRACE
#define CDEB(a, format, args...)
#define DEB_FNAME(format, args...)

#endif // end of VBP

#endif
