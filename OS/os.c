/*---------------------------------------
-* File:    RTOS
-* VERSION��V1.5
-* MODIFY��	��д�����ź�������
-*			�޸�OSTimeDly��ʱ����
-*			�޸���Ϣ������Ϣ����bug
-* WARNING����ʵʱ����ϵͳ����ѧϰ�ο�������������ҵ��;��Υ�߱ؾ���
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
    OS_ENTER_CRITICAL();                                  //�����ٽ��� 
    TCB_Task[prio].DLy =0; 
    OSSetPrioRdy(prio);         
    OS_EXIT_CRITICAL();                                 //�˳��ٽ��� 
	if(OS_PrioCur == prio)	//��������Ϊ��ǰ��������
		return ;
    if(OS_PrioCur > prio)     //��ǰ�������ȼ�С�ڻָ����������ȼ� 
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
	
	*(--p_stk)=(unsigned int)0x01000000uL;//xPSR״̬�Ĵ�������24λTHUMBģʽ������λһ 
	*(--p_stk)=(unsigned int)task;//entry point//�������
	*(--p_stk)=(unsigned int)Task_End ;//R14(LR);
	*(--p_stk)=(unsigned int)0x12121212uL;//R12
	*(--p_stk)=(unsigned int)0x03030303uL;//R3
	*(--p_stk)=(unsigned int)0x02020202uL;//R2
	*(--p_stk)=(unsigned int)0x01010101uL;//R1
	*(--p_stk)=(unsigned int)0x00000000uL;//R0
	//PendSV����ʱδ�Զ�������ں˼Ĵ�����R4~R11
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
			OS_PrioCur= OS_PrioHighRdy;//����OS_PrioCur
			OSCtxSw();//��������,�ڻ��������
		}
	}
	OS_EXIT_CRITICAL();                                 //�˳��ٽ���
}


void OS_SchedLock(void)
{
	unsigned int cpu_sr;
	OS_ENTER_CRITICAL();                                  //�����ٽ���
	lock =1;
	OS_EXIT_CRITICAL();                                 //�˳��ٽ���
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
	SysTick_CLKSourceConfig(SysTick_CLKSource_HCLK_Div8);	//ѡ���ⲿʱ��  HCLK/8 
	reload=SystemCoreClock/8000000;							//ÿ���ӵļ������� ��λΪK	   
	reload*=1000000/System_Ticks;		
											
	SysTick->CTRL|=SysTick_CTRL_TICKINT_Msk;   	//����SYSTICK�ж�
	SysTick->LOAD=reload; 		//ÿ1/System_Ticks���ж�һ��	
	SysTick->CTRL|=SysTick_CTRL_ENABLE_Msk;   	//����SYSTICK    

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
			if(TCB_Task[i].DLy==0)			//��ʱʱ�ӵ���
			{
				OSSetPrioRdy(i);            //�������¾���
			}
		}
		OS_EXIT_CRITICAL();
	}
	
	OS_Sched();//������pendsv�жϽ��е���
}


void OS_Start(void)
{
	System_init();
	CPU_ExceptStkBase=CPU_ExceptStk+OS_EXCEPT_STK_SIZE-1;//Cortex-M3ջ��������
	Task_Create(OS_IDLE_Task,&IDLE_STK[IDLE_STK_SIZE-1],OS_MAX_Task-1);//��������
	OSGetHighRdy();//�����߼��ľ�������
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
	TCB_Task[OS_PrioCur].OSTCBStatPend =OS_STAT_PEND_TO;//�ź���״̬
	TCB_Task[OS_PrioCur].DLy= time;
	pevent->OSEventTbl|=0x01<<OS_PrioCur;//��ӵ��ȴ���
	OSDelPrioRdy(OS_PrioCur);//�Ӿ�������ɾ��
	OS_EXIT_CRITICAL();
	OS_Sched();
	OS_ENTER_CRITICAL();
	if(TCB_Task[OS_PrioCur].OSTCBStatPend==OS_STAT_PEND_TO)//�����ǵȴ���ʱ�Ż�ȡ����Ȩ
	{
		pevent->OSEventTbl&=~(0x01<<OS_PrioCur);//�ӵȴ�����ɾ��
		TCB_Task[OS_PrioCur].OSTCBStatPend=OS_STAT_PEND_OK;//��־Ϊ����״̬
	}
	OS_EXIT_CRITICAL();
}

void OS_SemPost(ECB  *pevent)
{
	unsigned int cpu_sr;
	unsigned char	OS_ECB_Prio;
	OS_ENTER_CRITICAL();
	if(pevent->OSEventTbl!=0)//�������ڵȴ�
	{
		
		for(OS_ECB_Prio=0;//�ҳ��ȴ��������ȼ���ߵ�����
			(OS_ECB_Prio<OS_MAX_Event)&&(!((pevent->OSEventTbl)&(0x01<<OS_ECB_Prio)));
			OS_ECB_Prio++);
		pevent->OSEventTbl&=~(0x01<<OS_ECB_Prio);//�ӵȴ�����ɾ��
		OSSetPrioRdy(OS_ECB_Prio);//��ӵ���������
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
	if(pevent ->OSEventTbl !=0)//�������ڵȴ����ź���
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
	p->Prio=OS_MAX_Task;//��ʼ�����ȼ�Ϊ�����ڵ����ȼ���
	return p;
}


void OS_MutexPend(ECB  *pevent)
{
	unsigned int cpu_sr;
	OS_ENTER_CRITICAL();
	if(pevent ->Prio==OS_MAX_Task)
	{
		pevent ->Prio=OS_PrioCur;			
		pevent ->Cnt=OS_PrioCur;			//��¼������Դ��������ȼ�
		OS_EXIT_CRITICAL();
		return ;
	}
	else if(pevent->Cnt < OS_PrioCur)		//�����һ��������Դ����������ȼ��ȴ˴������(���ȼ���ֵС)
	{
		pevent->OSEventTbl|=0x01<<OS_PrioCur;//��ӵ��ȴ���
		OSDelPrioRdy(OS_PrioCur);			//�Ӿ�������ɾ��
		OS_EXIT_CRITICAL();
		OS_Sched();							
		return ;
	}
	else									//�����һ��������Դ����������ȼ��ȴ˴������
	{
		pevent->OSEventTbl|=0x01<<OS_PrioCur;//��ӵȴ���Դ�����񵽵ȴ���
		while(!(OSRdyTbl&(0x01<<(pevent ->Cnt))))//�����һ��������Դ������������״̬
		{
			OS_EXIT_CRITICAL();
			OS_Sched();//ֱ�ӵ��ȣ�����������,ֱ��ռ����Դ�����ھ���״̬
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
	OS_PrioCur=pevent->Prio;					//OS_PrioCur����Ϊ�ָ������������ȼ�
	pevent ->Prio=OS_MAX_Task;				//���ȼ�ָ��һ�������ڵ����ȼ��������Դ�ͷŵ���
	if(pevent->OSEventTbl)
	{
		for(OS_ECB_Prio=0;						//�ҳ��ȴ��������ȼ���ߵ�����
				(OS_ECB_Prio<OS_MAX_Event)&&(!((pevent->OSEventTbl)&(0x01<<OS_ECB_Prio)));
				OS_ECB_Prio++);
		pevent->OSEventTbl&=~(0x01<<OS_ECB_Prio);//�ӵȴ�����ɾ��
		OSSetPrioRdy(OS_ECB_Prio);				//��ӵȴ��������ȼ���ߵ����񵽾�������
		OSSetPrioRdy(OS_PrioCur);				
	}
	OS_EXIT_CRITICAL();
	OS_Sched();
}


void OS_MutexDel(ECB  *pevent)
{
	unsigned int cpu_sr;
	OS_ENTER_CRITICAL();
	if(pevent ->OSEventTbl !=0)//�������ڵȴ����ź���
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
		TCB_Task[OS_PrioCur].OSTCBStatPend =OS_STAT_Q_TO;//�ź���״̬
		TCB_Task[OS_PrioCur].DLy= time;
		pevent->OSEventTbl|=0x01<<OS_PrioCur;//��ӵ��ȴ���
		OSDelPrioRdy(OS_PrioCur);			//�Ӿ�������ɾ��
		OS_EXIT_CRITICAL();
		OS_Sched();
		OS_ENTER_CRITICAL();
	}
	addr=pevent ->Cnt;
	if(pevent ->Cnt >0)//����Ϣ����
	{
		pevent ->Cnt--;
	}
	if(TCB_Task[OS_PrioCur].OSTCBStatPend==OS_STAT_Q_TO)//�����ǵȴ���ʱ�Ż�ȡ����Ȩ
	{ 
		pevent->OSEventTbl&=~(0x01<<OS_PrioCur);//�ӵȴ�����ɾ��
		TCB_Task[OS_PrioCur].OSTCBStatPend=OS_STAT_Q_OK;//��־Ϊ����״̬
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
	if(pevent ->OSEventTbl!=0)//�������ڵȴ���Ϣ
	{
		OS_ENTER_CRITICAL();
		for(OS_ECB_Prio=0;						//�ҳ��ȴ��������ȼ���ߵ�����
			(OS_ECB_Prio<OS_MAX_Event)&&(!((pevent->OSEventTbl)&(0x01<<OS_ECB_Prio)));
			OS_ECB_Prio++);
		pevent->OSEventTbl&=~(0x01<<OS_ECB_Prio);//�ӵȴ�����ɾ��
		OSSetPrioRdy(OS_ECB_Prio);				//��ӵȴ��������ȼ���ߵ����񵽾�������
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
	if(pevent ->OSEventTbl !=0)//�������ڵȴ�����Ϣ
		return ;
	my_free(pevent);
	OS_EXIT_CRITICAL();
}
#endif



