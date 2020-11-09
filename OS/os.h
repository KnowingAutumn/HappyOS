#ifndef __OS_H
#define __OS_H	 
#include "sys.h"
#include <malloc.h>


#ifdef   OS_SET
#define  OS_EXT
#else
#define  OS_EXT  extern
#endif

#define OS_EXCEPT_STK_SIZE 		128u					//堆栈大小
#define OS_MAX_Task 			32u					//最大任务数(优先级数)
#define IDLE_STK_SIZE 			64u					//空闲任务堆栈大小
#define OS_MAX_Event 			32u					//最大事件数
#define OS_PRIO_SELF			0xFFu				//任务自身
#define System_Ticks  			100u					//每1000/System_Ticks ms进入一次中断

/*---------------------------------------
-* 条件编译
-*---------------------------------------*/
#define OS_USE_EVENT		1u	//使用事件
#define OS_USE_EVENT_Sem	1u	//使用信号量
#define OS_USE_EVENT_Mutex	1u	//使用互斥信号量
#define OS_USE_EVENT_Q		1u	//使用消息队列

/*---------------------------------------
-* 事件状态
-*---------------------------------------*/
#define  OS_STAT_PEND_OK                0u  
#define  OS_STAT_PEND_TO                1u
#define  OS_STAT_MUTEX_DLY              2u
#define  OS_STAT_MUTEX_NO_DLY           3u
#define  OS_STAT_Q_OK                	4u 
#define  OS_STAT_Q_TO                	5u

/*---------------------------------------
-* Description：利用编译器扩展功能获得程序状态字，保存在局部变量cpu_sr
-*---------------------------------------*/
#define  OS_ENTER_CRITICAL()  {cpu_sr = OS_CPU_SR_Save();}
#define  OS_EXIT_CRITICAL()   {OS_CPU_SR_Restore(cpu_sr);}

typedef struct 	OS_Tcb		   
{
	unsigned int *StkPtr;//任务栈顶
	unsigned int DLy;//任务延时时钟
	unsigned char OSTCBStatPend;//任务状态
   
}TCB; //任务控制块
//typedef struct OS_Tcb  TCB; //任务控制块

typedef struct 	OS_Ecb		   
{
	unsigned int Cnt;//计数器
	unsigned char OSEventTbl;//事件等待表
#if	OS_USE_EVENT_Mutex	> 0
	unsigned int Prio;//优先级
#endif
#if	OS_USE_EVENT_Q		> 0
	void **Addr;//地址
	unsigned int Size;//消息队列大小
#endif
   
}ECB; //事件控制块

OS_EXT unsigned int  CPU_ExceptStk[OS_EXCEPT_STK_SIZE];//主任务堆栈
OS_EXT unsigned int * CPU_ExceptStkBase;//指向的是数组最后一个元素

OS_EXT TCB TCB_Task[OS_MAX_Task];//任务控制块
OS_EXT unsigned int IDLE_STK[IDLE_STK_SIZE];
OS_EXT TCB *p_TCB_Cur;//指向当前任务的tcb
OS_EXT TCB *p_TCBHightRdy;//指向最高级任务的tcb
OS_EXT volatile unsigned char OS_PrioCur;//记录当前运行的任务优先级
OS_EXT volatile unsigned char OS_PrioHighRdy;
OS_EXT volatile unsigned int OSRdyTbl;//任务就绪表
OS_EXT unsigned int OS_Tisks;


extern void OSCtxSw(void);
extern void OSStartHighRdy(void);
extern unsigned int OS_CPU_SR_Save(void);
extern void OS_CPU_SR_Restore(unsigned int cpu_sr);

void OS_TaskSuspend(unsigned char prio) ;
void OS_TaskResume(u8 prio);
void Task_Create(void (*task)(void),unsigned int *stk,unsigned char prio);
void OSSetPrioRdy(unsigned char prio);
void OSDelPrioRdy(unsigned char prio);
void OS_Start(void);
void OSTimeDly(unsigned int ticks);
void OS_Sched(void);
void OS_SchedLock(void);
void OS_SchedUnlock(void);

#if	OS_USE_EVENT_Sem	> 0
ECB * OS_SemCreate(unsigned char cnt);
void OS_SemPend(ECB  *pevent,unsigned char time);
void OS_SemPost(ECB  *pevent);
void OS_SemDel(ECB  *pevent);
#endif

#if	OS_USE_EVENT_Mutex	> 0
ECB * OS_MutexCreate(void);
void OS_MutexPend(ECB  *pevent);
void OS_MutexPost(ECB  *pevent);
void OS_MutexDel(ECB  *pevent);
#endif

#if	OS_USE_EVENT_Q		> 0
ECB * OS_QCreate(void	*start[],unsigned char size);
void * OS_QPend(ECB  *pevent,unsigned char time,unsigned char opt);
void OS_QPost(ECB  *pevent,void * pmsg);
void OS_QDel(ECB  *pevent);
#endif

#endif


