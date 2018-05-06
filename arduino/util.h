#ifndef __UTIL_H__
#define __UTIL_H__

#define htons(x) ( ((x)<< 8 & 0xFF00) | \
                   ((x)>> 8 & 0x00FF) )
#define ntohs(x) htons(x)

#endif
