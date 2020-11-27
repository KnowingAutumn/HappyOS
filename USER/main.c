//#include "stm32f10x.h"
#include "led.h"
#include "os.h"
#include "stdio.h"
#include "lcd.h"
#include "delay.h"	
#define TASK_1_STK_SIZE 1024
#define TASK_2_STK_SIZE 1024
#define TASK_3_STK_SIZE 1024
unsigned int TASK_1_STK[TASK_1_STK_SIZE];
unsigned int TASK_2_STK[TASK_2_STK_SIZE];
unsigned int TASK_3_STK[TASK_3_STK_SIZE];
ECB * s_msg;			//信号量
ECB * m_msg;			//互斥信号量
ECB * q_msg;			//消息队列
void * MsgGrp[255];			//消息队列存储地址,最大支持255个消息

void Task1(void)
{
	static unsigned char i=0;
	OSTimeDly(200);
	while(1)
	{
		i++;
		OS_MutexPend(m_msg);
		LED0=!LED0;
		OS_MutexPost(m_msg);
		if(i==10)
		{
			OS_SemPost(s_msg);
			//OS_SemDel(s_msg);//删除信号量,最好删除操作此信号量的所有任务
			LCD_ShowString(160,100,210,24,24,OS_QPend(q_msg,0,1));
		}
		if(i==150)
			i=100;//防止i溢出
		OSTimeDly(400);
	}
}
void Task2(void)
{
	unsigned char i=0;
	void *p;
	OSTimeDly(250);
	p=my_malloc(8);
	sprintf(p,"By_KITE");
	while(1)
	{
		i++;
		if(i==10)
		{
			OS_QPost(q_msg,p);
			OS_SemPend(s_msg,0);
		}
		if(i==150)
			i=100;//防止i溢出
		LED1=!LED1;	
		OSTimeDly(150);
	}
}
void delay(unsigned int j )//无意义延时  
{  
    unsigned int i = 0;  
    unsigned int k = j;  
    for(i=0;i<50000;i++)  
    {  
        while(--j);  
        j=k;  
    }  
}
void Task3(void)
{
	while(1)
	{
		LED0=0;
		OS_MutexPend(m_msg);
		delay(1000);//一直霸占资源
		OS_MutexPost(m_msg);
		OS_TaskSuspend(OS_PRIO_SELF);//挂起自身
		OSTimeDly(250);
	}
}
int main(void)
{	
	NVIC_PriorityGroupConfig(NVIC_PriorityGroup_2);
	LED_Init();
	Task_Create(Task1,&TASK_1_STK[TASK_1_STK_SIZE-1],0);
	Task_Create(Task2,&TASK_2_STK[TASK_2_STK_SIZE-1],1);
	Task_Create(Task3,&TASK_3_STK[TASK_3_STK_SIZE-1],2);
	malloc_init();//内存初始化
	delay_init();
	LCD_Init();
	POINT_COLOR=RED;
	LCD_Clear(WHITE);
	LCD_ShowString(30,40,210,24,24,(u8*)"This is a system!");
	s_msg=OS_SemCreate(0);
	m_msg =OS_MutexCreate();	//创建互斥信号量
	q_msg=OS_QCreate(MsgGrp,255);//创建消息队列
	
	OS_Start(); 
	return 0;

}




