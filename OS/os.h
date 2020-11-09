#ifndef __OS_H
#define __OS_H	 
#include "sys.h"
#include <malloc.h>


#ifdef   OS_SET
#define  OS_EXT
#else
#define  OS_EXT  extern
#endif

#define OS_EXCEPT_STK_SIZE 		128u					//��ջ��С
#define OS_MAX_Task 			32u					//���������(���ȼ���)
#define IDLE_STK_SIZE 			64u					//���������ջ��С
#define OS_MAX_Event 			32u					//����¼���
#define OS_PRIO_SELF			0xFFu				//��������
#define System_Ticks  			100u					//ÿ1000/System_Ticks ms����һ���ж�

/*---------------------------------------
-* ��������
-*---------------------------------------*/
#define OS_USE_EVENT		1u	//ʹ���¼�
#define OS_USE_EVENT_Sem	1u	//ʹ���ź���
#define OS_USE_EVENT_Mutex	1u	//ʹ�û����ź���
#define OS_USE_EVENT_Q		1u	//ʹ����Ϣ����

/*---------------------------------------
-* �¼�״̬
-*---------------------------------------*/
#define  OS_STAT_PEND_OK                0u  
#define  OS_STAT_PEND_TO                1u
#define  OS_STAT_MUTEX_DLY              2u
#define  OS_STAT_MUTEX_NO_DLY           3u
#define  OS_STAT_Q_OK                	4u 
#define  OS_STAT_Q_TO                	5u

/*---------------------------------------
-* Description�����ñ�������չ���ܻ�ó���״̬�֣������ھֲ�����cpu_sr
-*---------------------------------------*/
#define  OS_ENTER_CRITICAL()  {cpu_sr = OS_CPU_SR_Save();}
#define  OS_EXIT_CRITICAL()   {OS_CPU_SR_Restore(cpu_sr);}

typedef struct 	OS_Tcb		   
{
	unsigned int *StkPtr;//����ջ��
	unsigned int DLy;//������ʱʱ��
	unsigned char OSTCBStatPend;//����״̬
   
}TCB; //������ƿ�
//typedef struct OS_Tcb  TCB; //������ƿ�

typedef struct 	OS_Ecb		   
{
	unsigned int Cnt;//������
	unsigned char OSEventTbl;//�¼��ȴ���
#if	OS_USE_EVENT_Mutex	> 0
	unsigned int Prio;//���ȼ�
#endif
#if	OS_USE_EVENT_Q		> 0
	void **Addr;//��ַ
	unsigned int Size;//��Ϣ���д�С
#endif
   
}ECB; //�¼����ƿ�

OS_EXT unsigned int  CPU_ExceptStk[OS_EXCEPT_STK_SIZE];//�������ջ
OS_EXT unsigned int * CPU_ExceptStkBase;//ָ������������һ��Ԫ��

OS_EXT TCB TCB_Task[OS_MAX_Task];//������ƿ�
OS_EXT unsigned int IDLE_STK[IDLE_STK_SIZE];
OS_EXT TCB *p_TCB_Cur;//ָ��ǰ�����tcb
OS_EXT TCB *p_TCBHightRdy;//ָ����߼������tcb
OS_EXT volatile unsigned char OS_PrioCur;//��¼��ǰ���е��������ȼ�
OS_EXT volatile unsigned char OS_PrioHighRdy;
OS_EXT volatile unsigned int OSRdyTbl;//���������
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

