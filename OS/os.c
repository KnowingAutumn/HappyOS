/*---------------------------------------
-* File:    RTOS
-* VERSION：V1.5
-* MODIFY：	重写互斥信号量函数
-*			修改OSTimeDly延时函数
-*			修复消息队列消息满载bug
-* WARNING：此实时操作系统仅供学习参考，请勿用于商业用途，违者必究！
-*---------------------------------------*/
#define OS_SET
#include "os.h"

unsigned int lock=0;

__inline void OSSetPrioRdy(unsigned char prio)
{
	OSRdyTbl|=0x01<<prio;
}

__inline void OSDelPrioRdy(unsigned char prio)
{
	OSRdyTbl&=~(0x01<<prio);
}

__inline void OSGetHighRdy(void)				
{									
	unsigned char	OS_NEXT_Prio;	
	for(OS_NEXT_Prio=0;(OS_NEXT_Prio<OS_MAX_Task)&&(!(OSRdyTbl&(0x01<<OS_NEXT_Prio)));OS_NEXT_Prio++);
	OS_PrioHighRdy=OS_NEXT_Prio;	
}


void OS_TaskSuspend(unsigned char prio)  
{
	unsigned int cpu_sr;
    OS_ENTER_CRITICAL();                                  
	if(prio ==OS_PRIO_SELF)
		prio =OS_PrioCur;
	else if(prio == OS_MAX_Task-1)		
		return ;
    TCB_Task[prio].DLy =0; 
    OSDelPrioRdy(prio);         
    OS_EXIT_CRITICAL();                                   
    if(OS_PrioCur == prio)       
    {  
        OS_Sched(); 
    }  
} 

void OS_TaskResume(u8 prio)  
{  
    unsigned int cpu_sr;
    OS_ENTER_CRITICAL();                                  //进入临界区 
    TCB_Task[prio].DLy =0; 
    OSSetPrioRdy(prio);         
    OS_EXIT_CRITICAL();                                 //退出临界区 
	if(OS_PrioCur == prio)	//唤醒任务为当前运行任务
		return ;
    if(OS_PrioCur > prio)     //当前任务优先级小于恢复的任务优先级 
    {  
        OS_Sched();  
    }  
}


void Task_End(void)
{
	while(1);
}


void Task_Create(void (*task)(void),unsigned int *stk,unsigned char prio)
{
	unsigned int * p_stk;
	p_stk=stk;
	p_stk=(unsigned int *) ((unsigned int)(p_stk)&0xFFFFFFF8u);
	
	*(--p_stk)=(unsigned int)0x01000000uL;//xPSR状态寄存器、第24位THUMB模式必须置位一 
	*(--p_stk)=(unsigned int)task;//entry point//函数入口
	*(--p_stk)=(unsigned int)Task_End ;//R14(LR);
	*(--p_stk)=(unsigned int)0x12121212uL;//R12
	*(--p_stk)=(unsigned int)0x03030303uL;//R3
	*(--p_stk)=(unsigned int)0x02020202uL;//R2
	*(--p_stk)=(unsigned int)0x01010101uL;//R1
	*(--p_stk)=(unsigned int)0x00000000uL;//R0
	//PendSV发生时未自动保存的内核寄存器：R4~R11
	*(--p_stk)=(unsigned int)0x11111111uL;//R11
	*(--p_stk)=(unsigned int)0x10101010uL;//R10
	*(--p_stk)=(unsigned int)0x09090909uL;//R9
	*(--p_stk)=(unsigned int)0x08080808uL;//R8
	*(--p_stk)=(unsigned int)0x07070707uL;//R7
	*(--p_stk)=(unsigned int)0x06060606uL;//R6
	*(--p_stk)=(unsigned int)0x05050505uL;//R5
	*(--p_stk)=(unsigned int)0x04040404uL;//R4
	
	TCB_Task[prio].StkPtr =p_stk;
	TCB_Task[prio].DLy =0;
	
	OSSetPrioRdy(prio);
}


void OS_IDLE_Task(void)
{
	unsigned int IDLE_Count=0;
	while(1)
	{
		IDLE_Count++;
		//__ASM volatile("WFE");
	}
}


void OS_Sched(void)
{
	unsigned int cpu_sr;
	OS_ENTER_CRITICAL();                                 
	if(lock==0)										
	{
		OSGetHighRdy();    							
		if(OS_PrioHighRdy!=OS_PrioCur)             
		{
			p_TCBHightRdy=&TCB_Task[OS_PrioHighRdy];
			//p_TCB_Cur=&TCB_Task[OS_PrioCur];
			OS_PrioCur= OS_PrioHighRdy;//更新OS_PrioCur
			OSCtxSw();//调度任务,在汇编中引用
		}
	}
	OS_EXIT_CRITICAL();                                 //退出临界区
}


void OS_SchedLock(void)
{
	unsigned int cpu_sr;
	OS_ENTER_CRITICAL();                                  //进入临界区
	lock =1;
	OS_EXIT_CRITICAL();                                 //退出临界区
}


void OS_SchedUnlock(void)
{
	unsigned int cpu_sr;
	OS_ENTER_CRITICAL();                                  
	lock =0;
	OS_EXIT_CRITICAL();                                 
	OS_Sched();
}

void System_init(void)
{
	u32 reload;
	SysTick_CLKSourceConfig(SysTick_CLKSource_HCLK_Div8);	//选择外部时钟  HCLK/8 
	reload=SystemCoreClock/8000000;							//每秒钟的计数次数 单位为K	   
	reload*=1000000/System_Ticks;		
											
	SysTick->CTRL|=SysTick_CTRL_TICKINT_Msk;   	//开启SYSTICK中断
	SysTick->LOAD=reload; 		//每1/System_Ticks秒中断一次	
	SysTick->CTRL|=SysTick_CTRL_ENABLE_Msk;   	//开启SYSTICK    

}


void SysTick_Handler(void)
{
	unsigned int cpu_sr;
	unsigned char i=0;
	OS_Tisks++;

	for(;i<OS_MAX_Task;i++)
    {
		OS_ENTER_CRITICAL();
		if(TCB_Task[i].DLy)
		{
			TCB_Task[i].DLy-=1000/System_Ticks;
			if(TCB_Task[i].DLy==0)			//延时时钟到达
			{
				OSSetPrioRdy(i);            //任务重新就绪
			}
		}
		OS_EXIT_CRITICAL();
	}
	
	OS_Sched();//都是由pendsv中断进行调度
}


void OS_Start(void)
{
	System_init();
	CPU_ExceptStkBase=CPU_ExceptStk+OS_EXCEPT_STK_SIZE-1;//Cortex-M3栈向下增长
	Task_Create(OS_IDLE_Task,&IDLE_STK[IDLE_STK_SIZE-1],OS_MAX_Task-1);//空闲任务
	OSGetHighRdy();//获得最高级的就绪任务
	OS_PrioCur= OS_PrioHighRdy;
	//p_TCB_Cur=&TCB_Task[OS_PrioCur];
	p_TCBHightRdy=&TCB_Task[OS_PrioHighRdy];
	OSStartHighRdy();
}


void OSTimeDly(unsigned int ticks)
{
	if(ticks> 0)
	{
		unsigned int cpu_sr;
		OS_ENTER_CRITICAL();                                  
		OSDelPrioRdy(OS_PrioCur);                             
		TCB_Task[OS_PrioCur].DLy= ticks;                      
		OS_EXIT_CRITICAL();                                   
		OS_Sched();                                           
		//return ;
	}
}

#if	OS_USE_EVENT_Sem	> 0

ECB * OS_SemCreate(unsigned char cnt)
{
	ECB * p;
	p=my_malloc(sizeof (ECB));
	p->OSEventTbl =0;
	p->Cnt=cnt;
	return p;
}



void OS_SemPend(ECB  *pevent,unsigned char time)
{
	unsigned int cpu_sr;
	OS_ENTER_CRITICAL();
	if(pevent->Cnt > 0)
	{
		pevent->Cnt--;
		OS_EXIT_CRITICAL(); 
		return ;
	}
	TCB_Task[OS_PrioCur].OSTCBStatPend =OS_STAT_PEND_TO;//信号量状态
	TCB_Task[OS_PrioCur].DLy= time;
	pevent->OSEventTbl|=0x01<<OS_PrioCur;//添加到等待表
	OSDelPrioRdy(OS_PrioCur);//从就绪表中删除
	OS_EXIT_CRITICAL();
	OS_Sched();
	OS_ENTER_CRITICAL();
	if(TCB_Task[OS_PrioCur].OSTCBStatPend==OS_STAT_PEND_TO)//任务是等待超时才获取控制权
	{
		pevent->OSEventTbl&=~(0x01<<OS_PrioCur);//从等待表中删除
		TCB_Task[OS_PrioCur].OSTCBStatPend=OS_STAT_PEND_OK;//标志为正常状态
	}
	OS_EXIT_CRITICAL();
}

void OS_SemPost(ECB  *pevent)
{
	unsigned int cpu_sr;
	unsigned char	OS_ECB_Prio;
	OS_ENTER_CRITICAL();
	if(pevent->OSEventTbl!=0)//有任务在等待
	{
		
		for(OS_ECB_Prio=0;//找出等待表中优先级最高的任务
			(OS_ECB_Prio<OS_MAX_Event)&&(!((pevent->OSEventTbl)&(0x01<<OS_ECB_Prio)));
			OS_ECB_Prio++);
		pevent->OSEventTbl&=~(0x01<<OS_ECB_Prio);//从等待表中删除
		OSSetPrioRdy(OS_ECB_Prio);//添加到就绪表中
		TCB_Task[OS_ECB_Prio].OSTCBStatPend =OS_STAT_PEND_OK;
		OS_EXIT_CRITICAL();
		OS_Sched();
		return ;
	}
	else if(pevent->Cnt<255)
	{
		pevent->Cnt++;
		OS_EXIT_CRITICAL();
		return ;
	}
	else
		pevent->Cnt=0;
	OS_EXIT_CRITICAL();
}


void OS_SemDel(ECB  *pevent)
{
	unsigned int cpu_sr;
	OS_ENTER_CRITICAL();
	if(pevent ->OSEventTbl !=0)//有任务在等待该信号量
		return ;
	my_free(pevent);
	OS_EXIT_CRITICAL();
}
#endif

#if	OS_USE_EVENT_Mutex	> 0

ECB * OS_MutexCreate(void)
{
	ECB * p;
	p=my_malloc(sizeof (ECB));
	p->OSEventTbl =0;
	p->Prio=OS_MAX_Task;//初始化优先级为不存在的优先级数
	return p;
}


void OS_MutexPend(ECB  *pevent)
{
	unsigned int cpu_sr;
	OS_ENTER_CRITICAL();
	if(pevent ->Prio==OS_MAX_Task)
	{
		pevent ->Prio=OS_PrioCur;			
		pevent ->Cnt=OS_PrioCur;			//记录申请资源任务的优先级
		OS_EXIT_CRITICAL();
		return ;
	}
	else if(pevent->Cnt < OS_PrioCur)		//如果上一次申请资源的任务的优先级比此次任务高(优先级数值小)
	{
		pevent->OSEventTbl|=0x01<<OS_PrioCur;//添加到等待表
		OSDelPrioRdy(OS_PrioCur);			//从就绪表中删除
		OS_EXIT_CRITICAL();
		OS_Sched();							
		return ;
	}
	else									//如果上一次申请资源的任务的优先级比此次任务低
	{
		pevent->OSEventTbl|=0x01<<OS_PrioCur;//添加等待资源的任务到等待表
		while(!(OSRdyTbl&(0x01<<(pevent ->Cnt))))//如果上一个申请资源的任务处于休眠状态
		{
			OS_EXIT_CRITICAL();
			OS_Sched();//直接调度，不给于运行,直到占用资源任务处于就绪状态
			OS_ENTER_CRITICAL();
		}	
		OSDelPrioRdy(pevent ->Cnt);		
		pevent ->Cnt=OS_PrioCur;				
		while(pevent ->Prio!=OS_MAX_Task)	
		{
				p_TCBHightRdy=&TCB_Task[pevent ->Prio];
				TCB_Task[OS_PrioCur].OSTCBStatPend=OS_STAT_MUTEX_DLY;
				OS_EXIT_CRITICAL();
				OSCtxSw();					
				OS_ENTER_CRITICAL();
		}
		TCB_Task[OS_PrioCur].OSTCBStatPend=OS_STAT_MUTEX_NO_DLY;
		pevent ->Prio=OS_PrioCur;			
		OS_EXIT_CRITICAL();
		return ;
	}
}


void OS_MutexPost(ECB  *pevent)
{
	unsigned char	OS_ECB_Prio;
	unsigned int cpu_sr;
	OS_ENTER_CRITICAL();
	OS_PrioCur=pevent->Prio;					//OS_PrioCur更新为恢复被提升的优先级
	pevent ->Prio=OS_MAX_Task;				//优先级指向一个不存在的优先级，标记资源释放掉了
	if(pevent->OSEventTbl)
	{
		for(OS_ECB_Prio=0;						//找出等待表中优先级最高的任务
				(OS_ECB_Prio<OS_MAX_Event)&&(!((pevent->OSEventTbl)&(0x01<<OS_ECB_Prio)));
				OS_ECB_Prio++);
		pevent->OSEventTbl&=~(0x01<<OS_ECB_Prio);//从等待表中删除
		OSSetPrioRdy(OS_ECB_Prio);				//添加等待表中优先级最高的任务到就绪表中
		OSSetPrioRdy(OS_PrioCur);				
	}
	OS_EXIT_CRITICAL();
	OS_Sched();
}


void OS_MutexDel(ECB  *pevent)
{
	unsigned int cpu_sr;
	OS_ENTER_CRITICAL();
	if(pevent ->OSEventTbl !=0)//有任务在等待该信号量
		return ;
	my_free(pevent);
	OS_EXIT_CRITICAL();
}
#endif

#if	OS_USE_EVENT_Q		> 0

ECB * OS_QCreate(void	*start[],unsigned char size)
{
	ECB * p;
	p=my_malloc(sizeof (ECB));
	p->OSEventTbl =0;
	p->Cnt=0;
	p->Size=size;
	p->Addr=start;
	return p;
}


void * OS_QPend(ECB  *pevent,unsigned char time,unsigned char opt)
{
	int addr;
	unsigned int cpu_sr;
	OS_ENTER_CRITICAL();
	if((pevent ->Cnt==0)&&(opt==1))
	{
		TCB_Task[OS_PrioCur].OSTCBStatPend =OS_STAT_Q_TO;//信号量状态
		TCB_Task[OS_PrioCur].DLy= time;
		pevent->OSEventTbl|=0x01<<OS_PrioCur;//添加到等待表
		OSDelPrioRdy(OS_PrioCur);			//从就绪表中删除
		OS_EXIT_CRITICAL();
		OS_Sched();
		OS_ENTER_CRITICAL();
	}
	addr=pevent ->Cnt;
	if(pevent ->Cnt >0)//有消息队列
	{
		pevent ->Cnt--;
	}
	if(TCB_Task[OS_PrioCur].OSTCBStatPend==OS_STAT_Q_TO)//任务是等待超时才获取控制权
	{ 
		pevent->OSEventTbl&=~(0x01<<OS_PrioCur);//从等待表中删除
		TCB_Task[OS_PrioCur].OSTCBStatPend=OS_STAT_Q_OK;//标志为正常状态
	}
	OS_EXIT_CRITICAL();
	return (*((pevent->Addr)+addr));	
}


void OS_QPost(ECB  *pevent,void * pmsg)
{
	unsigned char	OS_ECB_Prio;
	unsigned int cpu_sr;
	OS_ENTER_CRITICAL();
	if(pevent ->Cnt==pevent ->Size)
		pevent ->Cnt=pevent ->Size;
	else
		pevent ->Cnt++;
	(pevent ->Addr)[pevent ->Cnt]=pmsg;
	OS_EXIT_CRITICAL();
	if(pevent ->OSEventTbl!=0)//有任务在等待消息
	{
		OS_ENTER_CRITICAL();
		for(OS_ECB_Prio=0;						//找出等待表中优先级最高的任务
			(OS_ECB_Prio<OS_MAX_Event)&&(!((pevent->OSEventTbl)&(0x01<<OS_ECB_Prio)));
			OS_ECB_Prio++);
		pevent->OSEventTbl&=~(0x01<<OS_ECB_Prio);//从等待表中删除
		OSSetPrioRdy(OS_ECB_Prio);				//添加等待表中优先级最高的任务到就绪表中
		TCB_Task[OS_ECB_Prio].OSTCBStatPend =OS_STAT_Q_OK;
		OS_EXIT_CRITICAL();
		OS_Sched();
		return ;
	}
	return ;
}

void OS_QDel(ECB  *pevent)
{
	unsigned int cpu_sr;
	OS_ENTER_CRITICAL();
	if(pevent ->OSEventTbl !=0)//有任务在等待该消息
		return ;
	my_free(pevent);
	OS_EXIT_CRITICAL();
}
#endif



