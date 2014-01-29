/*
 *  vmstat mib groups
 *
 */
#ifndef _MIBGROUP_ALARMS_H
#define _MIBGROUP_ALARMS_H

#include "mibdefs.h"

FindVarMethod   var_extensible_alarms;
void            init_alarms(void);

#define INPUTALVAL 2
#define MAXALVAL 3
#define MINALVAL 4
#define HYSTALVAL 5
#define ALSTAT 6

#define SWAPIN 7
#define SYSCONTEXT 8
#define CPUUSER 9
#define CPUSYSTEM 10
#define CPUIDLE 11
#define CPUERROR 16
#define CPURAWUSER 50
#define CPURAWNICE 51
#define CPURAWSYSTEM 52
#define CPURAWIDLE 53
#define CPURAWWAIT 54
#define CPURAWKERNEL 55
#define CPURAWINTR 56
#define IORAWSENT 57
#define IORAWRECEIVE 58
#define SYSRAWINTERRUPTS 59
#define SYSRAWCONTEXT 60
#define CPURAWSOFTIRQ 61
#define RAWSWAPIN 62
#define RAWSWAPOUT 63

#endif                          /* _MIBGROUP_VMSTAT_H */
