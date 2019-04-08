/*
 * File      : CPLD.c
 * This file is part of RT-Thread RTOS
 * COPYRIGHT (C) 2006, RT-Thread Development Team
 *
 * The license and distribution terms for this file may be
 * found in the file LICENSE in this distribution or at
 * http://www.rt-thread.org/license/LICENSE
 *
 * Change Logs:
 * Date           Author       Notes
 * 2011-09-21     JoyChen      First version, support I2C1
 */

#include <rtthread.h>
#include "CPLD.h"
#include "stm32f4xx_rcc.h"
#include "manage.h"
#include "sysapp.h"


#define CPLD_DBuf1    *(__IO uint8_t *) (0x64000000)    	//���ݻ���Ĵ���1
#define CPLD_DBuf2    *(__IO uint8_t *) (0x64000004)	 	//���ݻ���Ĵ���2
#define CPLD_DBuf3    *(__IO uint8_t *) (0x64000008)    	//���ݻ���Ĵ���3
#define CPLD_DBuf4    *(__IO uint8_t *) (0x6400000c)	 	//���ݻ���Ĵ���4
#define CPLD_DCmd     *(__IO uint8_t *) (0x64000010)	 	//����Ĵ���

///////////////////////////////////////////////////////////////////
#define DO_Config  0x00			//DO����������üĴ�����8λ�Ĵ�������ƫ�Ƶ�ַ��0x00��         ��д
#define DO_reg  0x01			//DO����Ĵ�����11λ�Ĵ�������ƫ�Ƶ�ַ��0x01��                  ��д
#define IQR_Rst  0x02			//�жϱ�־��λ�Ĵ�����8λ�Ĵ�������ƫ�Ƶ�ַ��0x02��             ֻд
#define Filter_reg  0x03			//�˲����ƼĴ�����32λ�Ĵ�������ƫ�Ƶ�ַ��0x03��            ֻд

#define PTO_Ctrl1  0x04  	//��1·PTO/PWM���������������8λ�Ĵ�������ƫ�Ƶ�ַ��0x04�� ��д
#define PTO_Freq1  0x05      	//��1·PTO����Ƶ�ʼĴ�����20λ�Ĵ�������ƫ�Ƶ�ַ��0x05��    ֻд
#define PlusSum1  0x06			//��1·PTO����������Ĵ�����32λ�Ĵ�������ƫ�Ƶ�ַ��0x06��    ֻд

#define NA240X_HC1_Ctrl  0x0a			//��1�����������ƼĴ�����8λ�Ĵ�������ƫ�Ƶ�ַ��0x0A��        ��д
#define NA240X_HC1_Init_Value  0x0b	//��1�����������ڼĴ�����16λ�Ĵ�������ƫ�Ƶ�ַ��0x0B��     ֻд
#define NA240X_HC1_Pre_Value  0x0c	//��1�����������ڼĴ�����16λ�Ĵ�������ƫ�Ƶ�ַ��0x0C��     ֻд

#define NA2412_HC1_Ctrl  0x10			//��1�����������ƼĴ�����8λ�Ĵ�������ƫ�Ƶ�ַ��0x10��        ��д
#define NA2412_HC1_Init_Value  0x11	//��1�����������ڼĴ�����16λ�Ĵ�������ƫ�Ƶ�ַ��0x11��     ֻд
#define NA2412_HC1_Pre_Value  0x12	//��1�����������ڼĴ�����16λ�Ĵ�������ƫ�Ƶ�ַ��0x12��     ֻд

#define IQR_State  0x02				//�ж�״̬�Ĵ�����8λ�Ĵ�������ƫ�Ƶ�ַ��0x02��
#define DI_reg  0x03				//DI�������ݼĴ�����14λ����ƫ�Ƶ�ַ��0x03��                  ֻ��
#define PTO1_PlusSumCur  0x05  		//��1·PTO�����ǰ�������Ĵ�����31λ����ƫ�Ƶ�ַ��0x05��ֻ��
#define NA240X_HC1_Cur_Value  0x0b	//��1·��ǰ�����Ĵ�����16λ����ƫ�Ƶ�ַ��0x0B��
#define NA2412_HC1_Cur_Value  0x11	//��1·��ǰ�����Ĵ�����16λ����ƫ�Ƶ�ַ��0x11��

////////PTO_Control����λ/////////////////////////////
#define PTO_Rst 0x00	 //RST/START:���������ʼ/��λ��==0��λ��==1���������ʼ��
#define PTO_Start 0x80
#define PTO_Mode 0x00	//TYPE:������ͣ�==0 PTO�����==1 PWM�����
#define PWM_Mode 0x20
#define DIR_Forward  0x00  //DIR:PTOʱ�����巽��
#define DIR_Reverse  0x10
#define PTO_INT_EN 0x04  //�ж�ʹ��

////////Tim_Ctrl����λ/////////////////////////////
#define HC_Rst	0x00  //��������λ��==0��λ��==1������ʹ�ܡ�
#define HC_EN	0x80
#define HC_Pause 0x40 //==1���Ƽ�������ͣ������==0���Ƽ�������������
#define HC_Mode1 0x00	//ģʽ1����ͨ�����������ģʽ��
#define HC_Mode2 0x01	//ģʽ2������+�������ģʽ
#define HC_Mode3 0x02	//ģʽ3��AB�����
#define HC_1X 0x00    //1��Ƶ
#define HC_4x 0x04    //4��Ƶ
#define HC_INT_EN 0x20  //�ж�ʹ��

#define Bank1_SRAM2_ADDR    ((u32)(0x64000000))	
#define NA240X_Axis_MAX    2
#define NA2411_Axis_MAX    4
#define Axis_MAX    4
#define HC_MAX    2

static unsigned int TIM3_State=0; //0-û�����ã�1-�Ѿ�����
static unsigned int TIM4_State=0; //0-û�����ã�1-�Ѿ�����
static unsigned char Count_10ms_flag=0;
static unsigned int DO_date=0;
static unsigned short CPLD_IO_Config_reg=0;
static unsigned char PTO_Ctrl_date[Axis_MAX]={0};
static unsigned char HC_Ctrl_date[HC_MAX]={0};

#define PTO_PWM  1
#define PTO_PLSY 2
#define PTO_PLSR 3
#define PTO_MPTO 4
#define PTO_ZRN  5
#define PTO_PLSV 6
#define PTO_PSTOP 7
#define PTO_PLIN 8


#define HC_Ctrl  10
#define IO_INT 20

unsigned char pwm_en_flag=0;
unsigned char plsy_en_flag=0;
unsigned char plsr_en_flag=0;
unsigned char zrn_en_flag=0;
unsigned char plsv_en_flag=0;
unsigned char plsv_en=0;
unsigned int plsv_freq=500;
unsigned char mpto_en_flag=0;
unsigned char pstop_en_flag=0;
unsigned char ch_flag=0;
unsigned char HC_flag=0;
unsigned char P_ABS_INC_Flag=0;

volatile uint8_t PTO1_INT_Flag=0;
volatile uint8_t PTO2_INT_Flag=0;
volatile uint8_t PTO3_INT_Flag=0;
volatile uint8_t PTO4_INT_Flag=0;
rt_sem_t	sem_lineprg=RT_NULL;	
 
struct rt_PTO_pagram_set
{
	unsigned char Dir_polar;   //������
	unsigned char P_Stop_type; //�������ֹͣ��ʽ��0-��ͣ��1-����ͣ
	unsigned char Limit_Protection; //���ޱ�����0-�رգ�1-����
	unsigned char Int_En;    //�ж�ʹ��
	unsigned short Time_min; //��С����ʱ��
	unsigned short Time_max; //������ʱ��
	unsigned int Freq_min;   //��С����Ƶ��
	unsigned int Freq_max;   //�������Ƶ��
};
struct rt_PTO_pagram_set PTO_pagram_set[Axis_MAX];

struct rt_PTO_pagram_run
{
  unsigned char P_plus_EN;  //���˶�ʹ��
	unsigned char P_state;    //��æ����
	unsigned char P_INC;      //0-����λ�ã�1-���λ��
	unsigned char P_cur_FB_mode; //��ǰִ�й��ܿ�����
	unsigned char P_MPTO_index;  //���MPTOִ�ж�����
	unsigned char P_MPTO_N;     //���MPTOִ�жζ���
	unsigned char P_Stop_Req;   //����ֹͣ��־
	unsigned char DI_X1_Flag;   //�����Կ���״̬
	unsigned char DI_X2_Flag;   //�����Կ���״̬
	unsigned char DI_X3_Flag;   //ԭ�㿪��״̬
	unsigned char P_Dir;
	unsigned char P_PStop_flag;
	unsigned int P_cur_FB_NO; //��ǰִ�й��ܿ�������
	unsigned int Exception_Code; //�쳣���� 0-������1-������
	unsigned int Freq_target; //Ŀ��Ƶ��
	unsigned int PlusSum_target; //Ŀ�귢�����塢Ŀ�����λ��
	unsigned int Freq_cur;    //��ǰ����Ƶ��
	unsigned int Duty_cur;    //��ǰռ�ձ�
	int POS_cur;     //��ǰ����λ��������
	unsigned int PON_cur;     //��ǰ�ѷ���������
	unsigned int PON_cur_reg;     //ɨ���������ݴ�
	unsigned int ADT_cur;   //�Ӽ��ٹ�����ÿ5ms��Ƶ�ʼӼ���
	unsigned int Plus_AD;   //�Ӽ��ٹ����У���ʼ���ٵ�������
	int *P_MPTO_TBL;   //��β�����ָ��
	unsigned int P_MPTO_Time;  
	unsigned int P_MPTO_PlusSum;  
	unsigned int P_Dec_flag;  
};

typedef struct
{
	unsigned char 	status;
	unsigned char 	abs_inc;
	unsigned char 	direction;
	unsigned char 	pto_index;
	unsigned short 	err_code;
	unsigned short 	rst_err_code;
	unsigned int 		freq;
	int 						abs_pos;
	unsigned int 		inc_pos;
}PTO_DATA;

struct rt_PTO_pagram_run PTO_pagram_run[Axis_MAX];
PTO_DATA pto_data[Axis_MAX];

#define MAX_INTS			(INT_PTO4 - INT_UP_DI13+1)
static rt_mailbox_t int_prg_mb=RT_NULL;
static rt_uint8_t 	irqnos[MAX_INTS] = {0,};

void set_pto_status()
{
	char i=0;
	short Axis=0;
	if(cputype==CPU2001_2411) Axis=NA2411_Axis_MAX;
	else Axis=NA240X_Axis_MAX;
	
	if (pConfigData->PtoVType==PARAMTYPE_R)
	{
		memcpy(pto_data,&pDynamicData->pR.Value[pConfigData->PtoVOccno-1],sizeof(PTO_DATA)*Axis);
	}
	else if (pConfigData->PtoVType==PARAMTYPE_NR)
	{
		memcpy(pto_data,&pNvramData->pNR.Value[pConfigData->PtoVOccno-1],sizeof(PTO_DATA)*Axis);
	}
	else if (pConfigData->PtoVType==PARAMTYPE_V)
	{
		memcpy(pto_data,&pDynamicData->pVAR[pConfigData->PtoVOccno-1],sizeof(PTO_DATA)*Axis);
	}
	
	for(i=0;i<Axis;i++)
	{
		if(pto_data[i].rst_err_code==1)
		{
			pto_data[i].rst_err_code=0;
			PTO_pagram_run[i].Exception_Code=0;
		}
		
		pto_data[i].status = PTO_pagram_run[i].P_state;
		pto_data[i].abs_inc = PTO_pagram_run[i].P_INC;
		pto_data[i].direction = PTO_pagram_run[i].P_Dir;
		pto_data[i].pto_index = PTO_pagram_run[i].P_MPTO_index;
		pto_data[i].err_code = PTO_pagram_run[i].Exception_Code;
		pto_data[i].freq = PTO_pagram_run[i].Freq_cur;
		pto_data[i].abs_pos = PTO_pagram_run[i].POS_cur;
		pto_data[i].inc_pos = PTO_pagram_run[i].PON_cur;
	}

	if (pConfigData->PtoVType==PARAMTYPE_R)
	{
		memcpy(&pDynamicData->pR.Value[pConfigData->PtoVOccno-1],pto_data,sizeof(PTO_DATA)*Axis);
	}
	else if (pConfigData->PtoVType==PARAMTYPE_NR)
	{
		memcpy(&pNvramData->pNR.Value[pConfigData->PtoVOccno-1],pto_data,sizeof(PTO_DATA)*Axis);
	}
	else if (pConfigData->PtoVType==PARAMTYPE_V)
	{
		memcpy(&pDynamicData->pVAR[pConfigData->PtoVOccno-1],pto_data,sizeof(PTO_DATA)*Axis);
	}
}

struct rt_HC_pagram_set
{
	unsigned char En;  		 //���ټ���ʹ��
	unsigned char Mode;    //ģʽ��0-��Ƶ��1-���������2-����+����3-AB�����
	unsigned char X1_X4;   //����
	unsigned char Int_En;  //�ж�ʹ��
};
struct rt_HC_pagram_set HC_pagram_set[HC_MAX];

struct rt_HC_pagram_run
{
	unsigned char Rst;    //��λ,1-��λ ������
	unsigned char Start;  //1-���� ������
	unsigned char Pause;  //1-��ͣ ������
	unsigned char State;  //״̬ 0-��λ״̬��1-����״̬��2-��ͣ״̬
	int T_Init_Value;     //��ʼֵ
	int T_Pre_Value;      //Ԥ��ֵ
	int T_Freq_Period;    //��Ƶ���ڣ���λ10ms�����ֵ200����2��
	int T_Cur_Value;      //��ǰֵ
	int T_Freq_Value;     //Ƶ��ֵ
	int T_Init_Value_reg;     //��ʼֵ  ���������ֵ�仯
	int T_Pre_Value_reg;      //Ԥ��ֵ
	int ptr;
	unsigned int T_Freq_buf[250];  //��Ƶ��������ֵ
};
struct rt_HC_pagram_run HC_pagram_run[HC_MAX];

typedef struct
{
	unsigned char 	start;
	unsigned char 	pause;
	unsigned char 	reset;
	unsigned char 	rsvd;
	int 						ini_value;
	int 						pre_value;
	unsigned char 	rsvd1;
	unsigned char 	rsvd2;
	unsigned char 	rsvd3;
	unsigned char 	state;
	int 						cur_value;
}HSC_DATA;

HSC_DATA hsc_data[HC_MAX];

////���ټ�����ʼ��////////////////////////////////
void HC_Init_Set(unsigned char ch,unsigned char mode,unsigned char X1_X4,unsigned char Int_En)
{
	if(ch<=HC_MAX) 
	{
		HC_Ctrl_date[ch]=0;
		if(mode==0) 
		{//��Ƶģʽ
			HC_Ctrl_date[ch]=HC_Ctrl_date[ch]|HC_Mode1;
		}
		else if(mode==1)
		{//�������
			HC_Ctrl_date[ch]=HC_Ctrl_date[ch]|HC_Mode1;
			if(Int_En==1) HC_Ctrl_date[ch]=HC_Ctrl_date[ch]|HC_INT_EN;
		}
		else if(mode==2)
		{//����+����
			HC_Ctrl_date[ch]=HC_Ctrl_date[ch]|HC_Mode2;
			if(Int_En==1) HC_Ctrl_date[ch]=HC_Ctrl_date[ch]|HC_INT_EN;
		}
		else if(mode==3)
		{//AB�����
			HC_Ctrl_date[ch]=HC_Ctrl_date[ch]|HC_Mode3;
			if(X1_X4==1) HC_Ctrl_date[ch]=HC_Ctrl_date[ch]|HC_4x;
			if(Int_En==1) HC_Ctrl_date[ch]=HC_Ctrl_date[ch]|HC_INT_EN;
		}
		if(cputype==CPU2001_2411)		CPLD_Write(NA2412_HC1_Ctrl+3*ch,HC_Ctrl_date[ch]);
		else CPLD_Write(NA240X_HC1_Ctrl+3*ch,HC_Ctrl_date[ch]);
	}
}

////���ټ�������////////////////////////////////
void HC_Start_set(unsigned char ch)
{
	if(ch<HC_MAX) 
	{
		HC_Ctrl_date[ch]=HC_Ctrl_date[ch]&0x3f;
		HC_Ctrl_date[ch]=HC_Ctrl_date[ch] | 0x80;
		if(cputype==CPU2001_2411)		CPLD_Write(NA2412_HC1_Ctrl+3*ch,HC_Ctrl_date[ch]);
		else CPLD_Write(NA240X_HC1_Ctrl+3*ch,HC_Ctrl_date[ch]);
		HC_pagram_run[ch].Start=0;
		HC_pagram_run[ch].State=1;
	}
}
	
////���ټ�����λ////////////////////////////////
void HC_Rst_set(unsigned char ch)
{
	if(ch<HC_MAX) 
	{
		HC_Ctrl_date[ch]=HC_Ctrl_date[ch]&0x3f;
		if(cputype==CPU2001_2411)		CPLD_Write(NA2412_HC1_Ctrl+3*ch,HC_Ctrl_date[ch]);
		else CPLD_Write(NA240X_HC1_Ctrl+3*ch,HC_Ctrl_date[ch]);
		HC_pagram_run[ch].Rst=0;
		HC_pagram_run[ch].State=0;
	}
}

////���ټ�����ͣ////////////////////////////////
void HC_Pause_set(unsigned char ch)
{
	if(ch<HC_MAX) 
	{
		HC_Ctrl_date[ch]=HC_Ctrl_date[ch]|HC_Pause;
		if(cputype==CPU2001_2411)		CPLD_Write(NA2412_HC1_Ctrl+3*ch,HC_Ctrl_date[ch]);
		else CPLD_Write(NA240X_HC1_Ctrl+3*ch,HC_Ctrl_date[ch]);
		HC_pagram_run[ch].Pause=0;
		HC_pagram_run[ch].State=2;
	}
}

////���ټ�����ʼֵ����////////////////////////////////
void HC_Init_Value_set(unsigned char ch,int Value)
{
	if(ch<HC_MAX) 
	{
		if(cputype==CPU2001_2411) CPLD_Write(NA2412_HC1_Init_Value+3*ch,Value);
		else CPLD_Write(NA240X_HC1_Init_Value+3*ch,Value);
	}
}
////���ټ���Ԥ��ֵ����////////////////////////////////
void HC_Pre_Value_set(unsigned char ch,int Value)
{
	if(ch<HC_MAX) 
	{
		if(cputype==CPU2001_2411) CPLD_Write(NA2412_HC1_Pre_Value+3*ch,Value);
		else CPLD_Write(NA240X_HC1_Pre_Value+3*ch,Value);
	}
}
////���ټ�����ǰ����ֵ��ȡ////////////////////////////////
unsigned int HC_Cur_Value_Read(unsigned char ch)
{
	unsigned int tmp=0;
	if(ch<HC_MAX) 
	{
		if(cputype==CPU2001_2411) tmp=CPLD_Read(NA2412_HC1_Cur_Value+3*ch);
		else tmp=CPLD_Read(NA240X_HC1_Cur_Value+3*ch);
	}
	return tmp;
}
////////////////////////////////////////////////////
void set_hsc_param()
{

	int i;
	if (pConfigData->HscVType==PARAMTYPE_R)
	{
		memcpy(hsc_data,&pDynamicData->pR.Value[pConfigData->HscVOccno-1],sizeof(HSC_DATA)*2);
	}
	else if (pConfigData->HscVType==PARAMTYPE_NR)
	{
		memcpy(hsc_data,&pNvramData->pNR.Value[pConfigData->HscVOccno-1],sizeof(HSC_DATA)*2);
	}
	else if (pConfigData->HscVType==PARAMTYPE_V)
	{
		memcpy(hsc_data,&pDynamicData->pVAR[pConfigData->HscVOccno-1],sizeof(HSC_DATA)*2);
	}
			
	for (i=0;i<2;i++)
	{
		if (hsc_data[i].reset==1)
		{
			HC_pagram_run[i].Rst=1;
			HC_Init_Value_set(i,hsc_data[i].ini_value);
			HC_Rst_set(i);
		}					
		else if (hsc_data[i].pause==1)
		{
			HC_pagram_run[i].Pause=1;
			HC_Pause_set(i);
		}		
		else if (hsc_data[i].start==1)
		{
			HC_pagram_run[i].Start=1;
			HC_Init_Value_set(i,hsc_data[i].ini_value);
			HC_Pre_Value_set(i,hsc_data[i].pre_value);
			HC_Start_set(i);
			HC_pagram_run[i].T_Init_Value=hsc_data[i].ini_value;
			HC_pagram_run[i].T_Pre_Value=hsc_data[i].pre_value;
			HC_pagram_run[i].T_Freq_Period=hsc_data[i].ini_value;
		}
		hsc_data[i].reset=0;
		hsc_data[i].pause=0;
		hsc_data[i].start=0;
		
		if (HC_pagram_run[i].T_Init_Value!=hsc_data[i].ini_value)
		{
			HC_pagram_run[i].T_Init_Value=hsc_data[i].ini_value;
			HC_Init_Value_set(i,HC_pagram_run[i].T_Init_Value);
			HC_pagram_run[i].T_Freq_Period=hsc_data[i].ini_value;
		}
		if (HC_pagram_run[i].T_Pre_Value!=hsc_data[i].pre_value)
		{
			HC_pagram_run[i].T_Pre_Value=hsc_data[i].pre_value;
			HC_Pre_Value_set(i,HC_pagram_run[i].T_Pre_Value);
		}
	}
	
	if (pConfigData->HscVType==PARAMTYPE_R)
	{
		memcpy(&pDynamicData->pR.Value[pConfigData->HscVOccno-1],hsc_data,sizeof(HSC_DATA)*2);
	}
	else if (pConfigData->HscVType==PARAMTYPE_NR)
	{
		memcpy(&pNvramData->pNR.Value[pConfigData->HscVOccno-1],hsc_data,sizeof(HSC_DATA)*2);
	}
	else if (pConfigData->HscVType==PARAMTYPE_V)
	{
		memcpy(&pDynamicData->pVAR[pConfigData->HscVOccno-1],hsc_data,sizeof(HSC_DATA)*2);
	}
}

void get_hsc_status(void)
{
	char i=0;
	
	if (pConfigData->HscVType==PARAMTYPE_R)
	{
		memcpy(hsc_data,&pDynamicData->pR.Value[pConfigData->HscVOccno-1],sizeof(HSC_DATA)*2);
	}
	else if (pConfigData->HscVType==PARAMTYPE_NR)
	{
		memcpy(hsc_data,&pNvramData->pNR.Value[pConfigData->HscVOccno-1],sizeof(HSC_DATA)*2);
	}
	else if (pConfigData->HscVType==PARAMTYPE_V)
	{
		memcpy(hsc_data,&pDynamicData->pVAR[pConfigData->HscVOccno-1],sizeof(HSC_DATA)*2);
	}
	
	for(i=0;i<2;i++)
	{
		hsc_data[i].state = HC_pagram_run[i].State;
		HC_pagram_run[i].T_Cur_Value = HC_Cur_Value_Read(i);

		if(HC_pagram_set[i].Mode==0)
			hsc_data[i].cur_value = HC_pagram_run[i].T_Freq_Value;
		else
			hsc_data[i].cur_value = HC_pagram_run[i].T_Cur_Value;
	}
	
	if (pConfigData->HscVType==PARAMTYPE_R)
	{
		memcpy(&pDynamicData->pR.Value[pConfigData->HscVOccno-1],hsc_data,sizeof(HSC_DATA)*2);
	}
	else if (pConfigData->HscVType==PARAMTYPE_NR)
	{
		memcpy(&pNvramData->pNR.Value[pConfigData->HscVOccno-1],hsc_data,sizeof(HSC_DATA)*2);
	}
	else if (pConfigData->HscVType==PARAMTYPE_V)
	{
		memcpy(&pDynamicData->pVAR[pConfigData->HscVOccno-1],hsc_data,sizeof(HSC_DATA)*2);
	}
}

//��ʼ���ⲿSRAM
void FSMC_SRAM_Init(void)
{	
	GPIO_InitTypeDef  GPIO_InitStructure;
	FSMC_NORSRAMInitTypeDef  FSMC_NORSRAMInitStructure;
  FSMC_NORSRAMTimingInitTypeDef  readWriteTiming; 
	
	RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOD|RCC_AHB1Periph_GPIOE|RCC_AHB1Periph_GPIOF|RCC_AHB1Periph_GPIOG, ENABLE);//ʹ��PD,PE,PF,PGʱ��  
  RCC_AHB3PeriphClockCmd(RCC_AHB3Periph_FSMC,ENABLE);//ʹ��FSMCʱ��  
   
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF;//�������
  GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;//�������
  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;//100MHz
  GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_UP;//����
	
	GPIO_PinAFConfig(GPIOD,GPIO_PinSource0,GPIO_AF_FSMC);//PD0,AF12
  GPIO_PinAFConfig(GPIOD,GPIO_PinSource1,GPIO_AF_FSMC);//PD1,AF12
  GPIO_PinAFConfig(GPIOD,GPIO_PinSource4,GPIO_AF_FSMC);
  GPIO_PinAFConfig(GPIOD,GPIO_PinSource5,GPIO_AF_FSMC); 
  GPIO_PinAFConfig(GPIOD,GPIO_PinSource7,GPIO_AF_FSMC); 
  GPIO_PinAFConfig(GPIOD,GPIO_PinSource14,GPIO_AF_FSMC);
  GPIO_PinAFConfig(GPIOD,GPIO_PinSource15,GPIO_AF_FSMC);//PD15,AF12
	GPIO_InitStructure.GPIO_Pin = (3<<0)|(3<<4)|(1<<7)|(3<<14);//PD0,1,4,5,14~15 AF OUT
  GPIO_Init(GPIOD, &GPIO_InitStructure);//��ʼ��  
	
	GPIO_PinAFConfig(GPIOE,GPIO_PinSource7,GPIO_AF_FSMC);//PE7,AF12
  GPIO_PinAFConfig(GPIOE,GPIO_PinSource8,GPIO_AF_FSMC);
  GPIO_PinAFConfig(GPIOE,GPIO_PinSource9,GPIO_AF_FSMC);
  GPIO_PinAFConfig(GPIOE,GPIO_PinSource10,GPIO_AF_FSMC);
  GPIO_InitStructure.GPIO_Pin = (0X0F<<7);//PE7~10,AF OUT
  GPIO_Init(GPIOE, &GPIO_InitStructure);//��ʼ��  
	
	GPIO_PinAFConfig(GPIOF,GPIO_PinSource2,GPIO_AF_FSMC);//PF2,AF12
  GPIO_PinAFConfig(GPIOF,GPIO_PinSource3,GPIO_AF_FSMC);//PF3,AF12
  GPIO_PinAFConfig(GPIOF,GPIO_PinSource4,GPIO_AF_FSMC);//PF4,AF12
  GPIO_PinAFConfig(GPIOF,GPIO_PinSource5,GPIO_AF_FSMC);//PF5,AF12
  GPIO_PinAFConfig(GPIOF,GPIO_PinSource12,GPIO_AF_FSMC);//PF12,AF12
  GPIO_PinAFConfig(GPIOF,GPIO_PinSource13,GPIO_AF_FSMC);//PF13,AF12
  GPIO_PinAFConfig(GPIOF,GPIO_PinSource14,GPIO_AF_FSMC);//PF14,AF12
  GPIO_PinAFConfig(GPIOF,GPIO_PinSource15,GPIO_AF_FSMC);//PF15,AF12
 	GPIO_InitStructure.GPIO_Pin = (0X0F<<2)|(0XF<<12); 	//PF2~5,12~15
  GPIO_Init(GPIOF, &GPIO_InitStructure);//��ʼ��  

	GPIO_PinAFConfig(GPIOG,GPIO_PinSource9,GPIO_AF_FSMC);
	GPIO_InitStructure.GPIO_Pin =GPIO_Pin_9;//PG0~5,10
  GPIO_Init(GPIOG, &GPIO_InitStructure);//��ʼ�� 
 
 	readWriteTiming.FSMC_AddressSetupTime = 15;	 //��ַ����ʱ�䣨ADDSET��Ϊ1��HCLK 1/36M=27ns
  readWriteTiming.FSMC_AddressHoldTime = 5;	 //��ַ����ʱ�䣨ADDHLD��ģʽAδ�õ�	
  readWriteTiming.FSMC_DataSetupTime = 15;		 ////���ݱ���ʱ�䣨DATAST��Ϊ9��HCLK 6*9=54ns	 	 
  readWriteTiming.FSMC_BusTurnAroundDuration = 1;
  readWriteTiming.FSMC_CLKDivision = 0x00;
  readWriteTiming.FSMC_DataLatency = 0x00;
  readWriteTiming.FSMC_AccessMode = FSMC_AccessMode_A;	 //ģʽA 
     
  FSMC_NORSRAMInitStructure.FSMC_Bank = FSMC_Bank1_NORSRAM2;//  ��������ʹ��NE3 ��Ҳ�Ͷ�ӦBTCR[4],[5]��
  FSMC_NORSRAMInitStructure.FSMC_DataAddressMux = FSMC_DataAddressMux_Disable; 
  FSMC_NORSRAMInitStructure.FSMC_MemoryType =FSMC_MemoryType_SRAM;// FSMC_MemoryType_SRAM;  //SRAM   
  FSMC_NORSRAMInitStructure.FSMC_MemoryDataWidth = FSMC_MemoryDataWidth_8b;//�洢�����ݿ��Ϊ16bit  
  FSMC_NORSRAMInitStructure.FSMC_BurstAccessMode =FSMC_BurstAccessMode_Disable;// FSMC_BurstAccessMode_Disable; 
  FSMC_NORSRAMInitStructure.FSMC_WaitSignalPolarity = FSMC_WaitSignalPolarity_Low;
	FSMC_NORSRAMInitStructure.FSMC_AsynchronousWait=FSMC_AsynchronousWait_Disable;
  FSMC_NORSRAMInitStructure.FSMC_WrapMode = FSMC_WrapMode_Disable;   
  FSMC_NORSRAMInitStructure.FSMC_WaitSignalActive = FSMC_WaitSignalActive_BeforeWaitState;  
  FSMC_NORSRAMInitStructure.FSMC_WriteOperation = FSMC_WriteOperation_Enable;	//�洢��дʹ�� 
  FSMC_NORSRAMInitStructure.FSMC_WaitSignal = FSMC_WaitSignal_Disable;  
  FSMC_NORSRAMInitStructure.FSMC_ExtendedMode = FSMC_ExtendedMode_Disable; // ��дʹ����ͬ��ʱ��
  FSMC_NORSRAMInitStructure.FSMC_WriteBurst = FSMC_WriteBurst_Disable;  
  FSMC_NORSRAMInitStructure.FSMC_ReadWriteTimingStruct = &readWriteTiming;
  FSMC_NORSRAMInitStructure.FSMC_WriteTimingStruct = &readWriteTiming; //��дͬ��ʱ��

  FSMC_NORSRAMInit(&FSMC_NORSRAMInitStructure);  //��ʼ��FSMC����

 	FSMC_NORSRAMCmd(FSMC_Bank1_NORSRAM2, ENABLE);  // ʹ��BANK1����3										  
											
}

/////////дCPLDָ���Ĵ���/////////////////////////////
void CPLD_Write(unsigned char addr,unsigned int date)
{
	rt_base_t level;
	level = rt_hw_interrupt_disable();
	CPLD_DBuf1=	date;	   
	CPLD_DBuf2=	date>>8;
	CPLD_DBuf3=	date>>16;
	CPLD_DBuf4=	date>>24;
	CPLD_DCmd=0x80 | addr;
	CPLD_DCmd=0;
	rt_hw_interrupt_enable(level);
}

/////////��CPLDָ���Ĵ���/////////////////////////////
unsigned int CPLD_Read(unsigned char addr)
{
	unsigned int tmp=0;
	rt_base_t level;
	level = rt_hw_interrupt_disable();
	CPLD_DCmd=0x40 | addr;
	CPLD_DCmd=0;
	tmp=CPLD_DBuf4;
	tmp=tmp<<8;
	tmp=tmp+CPLD_DBuf3;
	tmp=tmp<<8;
	tmp=tmp+CPLD_DBuf2;
	tmp=tmp<<8;
	tmp=tmp+CPLD_DBuf1;
	rt_hw_interrupt_enable(level);
	return tmp;
}

unsigned char has_sub_modules()
{
	 unsigned int di_states = 0;
	 unsigned char flag = 0;
	 di_states = CPLD_Read(DI_reg);

	 flag = (di_states>>16) & 0x01;

	 return !flag;
}

unsigned short DO_Read()
{
	return CPLD_Read(DO_reg);
}

////////////////////////////////////////////////////
//DO_Write���DOλ  ch DOͨ���� ��Χ0-7   date 0����1
void DO_Write(unsigned char ch,unsigned char date)
{
	if(date>0)	DO_date=DO_date|(1<<ch);
	else  DO_date=DO_date&((~(1<<ch))&0xffff);
	CPLD_Write(DO_reg,DO_date);
}

////////////////////////////////////////////////////
//DO_8_Write���DOλ  date 8λDOͬʱ���
void DO_8_Write(unsigned int date)
{
	DO_date &= 0xFFFFFF00;
	DO_date |= (date&0xff);
	CPLD_Write(DO_reg,date);
}

void Light_LED_RUN(unsigned char date)
{
	unsigned short value = DO_Read();
	if(date==0)
		DO_date &= (~(1<<10));
	else
		DO_date |= (1<<10);
	CPLD_Write(DO_reg,DO_date);	
}

void Light_LED_Alarm(unsigned char date)
{
	unsigned short value = DO_Read();
	if(date==0)
		DO_date &= (~(1<<9));
	else
		DO_date |= (1<<9);
	CPLD_Write(DO_reg,DO_date);	
}

void Light_LED_ERROR(unsigned char date)
{
	unsigned short value = DO_Read();
	if(date==0)
		DO_date &= (~(1<<8));
	else
		DO_date |= (1<<8);
	CPLD_Write(DO_reg,DO_date);
}

////////////////////////////////////////////////////
//DI_Read����DIλ  ch DIͨ���� ��Χ0-15  ���� λ״̬ 0��1
unsigned char DI_Read(unsigned char ch)
{
	unsigned int tmp=0;
	tmp=CPLD_Read(DI_reg);
	tmp=tmp&0xffff;
	if((tmp&(1<<ch))!=0) return 1;
	else return 0;
}

////////////////////////////////////////////////////
//DI_16_Readͬʱ����16λ����DIλ
unsigned int DI_16_Read(void)
{
	unsigned int tmp=0;
	tmp=CPLD_Read(DI_reg);
	tmp=tmp&0xffff;
	return tmp;
}

///////////////////////////////////////////////////
//PTO_Freq_Set ����PTO��ǰ���Ƶ�� ch ͨ��0��1 Freq �趨Ƶ��ֵ ��λHZ
void PTO_Freq_Set(unsigned char ch,unsigned int Freq)
{
	float f=1.048575;
	Freq=f*Freq;
	CPLD_Write(PTO_Freq1+3*ch,Freq);
}
void PWM_Freq_Set(unsigned char ch,unsigned int Freq)
{
	if((Freq>0)&(Freq<100000000))
	{
		Freq=100000000/Freq;
		CPLD_Write(PTO_Freq1+3*ch,Freq);
	}
}
//PWMģʽ��ռ�ձ�
void PWM_Duty_Set(unsigned char ch,unsigned int Duty)
{
	float f_tmp=0;
	int i_tmp;
	if((PTO_pagram_run[ch].Freq_cur>0)&(PTO_pagram_run[ch].Freq_cur<100000000))
	{
		i_tmp=100000000/PTO_pagram_run[ch].Freq_cur;
		f_tmp=Duty;
		f_tmp=(f_tmp/100)*i_tmp;
		CPLD_Write(PlusSum1+3*ch,(unsigned int)f_tmp);
	}
}
///////////////////////////////////////////////////
//PTO_PlusSum_Set ����PTO��Ҫ��������� ch ͨ��0��1  PlusSum �趨���������ֵ
void PTO_PlusSum_Set(unsigned char ch,unsigned int PlusSum)
{
	CPLD_Write(PlusSum1+3*ch,PlusSum);
}

////////////////////////////////////////////////////
//PTO_PlusCurNum_Read PTO��ǰ��������� ch ͨ��0��1   ����32λ��ǰ������
unsigned int PTO_PlusCurNum_Read(unsigned char ch)
{
	unsigned int tmp=0;
	tmp=CPLD_Read(PTO1_PlusSumCur+3*ch);
	return tmp;
}

void PTO_INT_Enable(uint8_t ch)
{
	PTO_Ctrl_date[ch]=PTO_Ctrl_date[ch]|PTO_INT_EN;
	CPLD_Write(PTO_Ctrl1+3*ch,PTO_Ctrl_date[ch]);
}
void PTO_INT_Disable(uint8_t ch)
{
	PTO_Ctrl_date[ch]=PTO_Ctrl_date[ch]&(~PTO_INT_EN);
	CPLD_Write(PTO_Ctrl1+3*ch,PTO_Ctrl_date[ch]);
}


///////////////////////////////////////////////////
//PTO_Mode_Set ����PTOģʽ mode==0:PTO,==1PWM
void PTO_Mode_Set(unsigned char ch,unsigned char mode)
{
	if(mode==0) 
	{
		PTO_Ctrl_date[ch]=PTO_Ctrl_date[ch]&0xdf;
		PTO_INT_Enable(ch);
	}
	else
	{
		PTO_Ctrl_date[ch]=PTO_Ctrl_date[ch]|PWM_Mode;
		PTO_INT_Disable(ch);
	}
	CPLD_Write(PTO_Ctrl1+3*ch,PTO_Ctrl_date[ch]);
}



///////////////////////////////////////////////////
//PTO_NO_Stop_Set ��������������
//mode:0���� 1:����
void PTO_NO_Stop_Set(unsigned char ch,unsigned char mode)
{
	if(mode==0) PTO_Ctrl_date[ch]=PTO_Ctrl_date[ch]&0xf7;
	else PTO_Ctrl_date[ch]=PTO_Ctrl_date[ch]|0x08;
	CPLD_Write(PTO_Ctrl1+3*ch,PTO_Ctrl_date[ch]);
}
//dir��PTO�������  0-������1-������
void PTO_Dir_Set(unsigned char ch,unsigned char dir)
{
	if(PTO_pagram_set[ch].Dir_polar==0)
	{
		if(dir==0) PTO_Ctrl_date[ch]=PTO_Ctrl_date[ch] & 0x0ef;
		else PTO_Ctrl_date[ch]=PTO_Ctrl_date[ch] | DIR_Reverse;
	}
	else
	{
		if(dir==0) PTO_Ctrl_date[ch]=PTO_Ctrl_date[ch] | DIR_Reverse;
		else PTO_Ctrl_date[ch]=PTO_Ctrl_date[ch] & 0x0ef;
	}
	CPLD_Write(PTO_Ctrl1+3*ch,PTO_Ctrl_date[ch]);
}


void PTO_Start_set(unsigned char ch)
{
	PTO_Ctrl_date[ch]=PTO_Ctrl_date[ch]|PTO_Start;
	CPLD_Write(PTO_Ctrl1+3*ch,PTO_Ctrl_date[ch]);
}

void PTO_Rst_set(unsigned char ch)
{
	PTO_Ctrl_date[ch]=PTO_Ctrl_date[ch]&0x7f;
	CPLD_Write(PTO_Ctrl1+3*ch,PTO_Ctrl_date[ch]);
}

//CPLD_IO_Config cpld��IO���ã�DO1-DO4,DI12-DI15�ж����´���������
void CPLD_IO_Config(unsigned char type,unsigned char ch,unsigned char param)
{
	switch(type)
	{
		case 0:CPLD_IO_Config_reg=0;
						break;
		case PTO_PWM:
								if(cputype==CPU2001_2411)
								{
									if(ch==0)
									{
										if(param==0) CPLD_IO_Config_reg=CPLD_IO_Config_reg&0xffee;
										else CPLD_IO_Config_reg=CPLD_IO_Config_reg|0x01;
									}
									else if(ch==1)
									{
										if(param==0) CPLD_IO_Config_reg=CPLD_IO_Config_reg&0xffdd;
										else CPLD_IO_Config_reg=CPLD_IO_Config_reg|0x02;
									}
									else if(ch==2)
									{
										if(param==0) CPLD_IO_Config_reg=CPLD_IO_Config_reg&0xffbb;
										else CPLD_IO_Config_reg=CPLD_IO_Config_reg|0x04;
									}
									else 
									{
										if(param==0) CPLD_IO_Config_reg=CPLD_IO_Config_reg&0xff77;
										else CPLD_IO_Config_reg=CPLD_IO_Config_reg|0x08;
									}
								}
								else
								{
									if(ch==0)
									{
										if(param==0) CPLD_IO_Config_reg=CPLD_IO_Config_reg&0xfffa;
										else CPLD_IO_Config_reg=CPLD_IO_Config_reg|0x01;
									}
									else
									{
										if(param==0) CPLD_IO_Config_reg=CPLD_IO_Config_reg&0xfff5;
										else CPLD_IO_Config_reg=CPLD_IO_Config_reg|0x02;
									}
								}
						break;
		case PTO_PLSY:
		case PTO_PLSR:
		case PTO_ZRN:
		case PTO_MPTO:
		case PTO_PSTOP:
		case PTO_PLSV:
		case PTO_PLIN:
								if(cputype==CPU2001_2411)
								{
									if(ch==0)
									{
										if(param==0) CPLD_IO_Config_reg=CPLD_IO_Config_reg&0xffee;
										else CPLD_IO_Config_reg=CPLD_IO_Config_reg|0x11;
									}
									else if(ch==1)
									{
										if(param==0) CPLD_IO_Config_reg=CPLD_IO_Config_reg&0xffdd;
										else CPLD_IO_Config_reg=CPLD_IO_Config_reg|0x22;
									}
									else if(ch==2)
									{
										if(param==0) CPLD_IO_Config_reg=CPLD_IO_Config_reg&0xffbb;
										else CPLD_IO_Config_reg=CPLD_IO_Config_reg|0x44;
									}
									else 
									{
										if(param==0) CPLD_IO_Config_reg=CPLD_IO_Config_reg&0xff77;
										else CPLD_IO_Config_reg=CPLD_IO_Config_reg|0x88;
									}
								}
								else
								{
									if(ch==0)
									{
										if(param==0) CPLD_IO_Config_reg=CPLD_IO_Config_reg&0xfffa;
										else CPLD_IO_Config_reg=CPLD_IO_Config_reg|0x05;
									}
									else
									{
										if(param==0) CPLD_IO_Config_reg=CPLD_IO_Config_reg&0xfff5;
										else CPLD_IO_Config_reg=CPLD_IO_Config_reg|0x0a;
									}
								}
						break;
		case IO_INT: //����DI12-DI15�ж�ʹ���Լ����ش���
								if(ch==0)//DI12
								{
									CPLD_IO_Config_reg=CPLD_IO_Config_reg&0xfeff;
									CPLD_IO_Config_reg=CPLD_IO_Config_reg|0x100;
								}
								if(ch==1)//DI13
								{
									CPLD_IO_Config_reg=CPLD_IO_Config_reg&0xfdff;
									CPLD_IO_Config_reg=CPLD_IO_Config_reg|0x200;
								}
								if(ch==2)//DI14
								{
									CPLD_IO_Config_reg=CPLD_IO_Config_reg&0xfbff;
									CPLD_IO_Config_reg=CPLD_IO_Config_reg|0x400;
								}
								if(ch==3)//DI15
								{
									CPLD_IO_Config_reg=CPLD_IO_Config_reg&0xf7ff;
									CPLD_IO_Config_reg=CPLD_IO_Config_reg|0x800;
								}
								break;
	}
	CPLD_Write(DO_Config,CPLD_IO_Config_reg);
}




/////////////////////////////////////////////////////
void Tim_Init(void)
{ 
    TIM_TimeBaseInitTypeDef  TIM_TimeBaseStructure;
		NVIC_InitTypeDef NVIC_InitStructure;
    
    /* Enable the TIM3 gloabal Interrupt */
    NVIC_InitStructure.NVIC_IRQChannel = TIM3_IRQn;
    NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0;
    NVIC_InitStructure.NVIC_IRQChannelSubPriority = 1;
    NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
    NVIC_Init(&NVIC_InitStructure);

    RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM3, ENABLE);      //    TIM3 clock enable 
    
    /* Time base configuration */
    TIM_TimeBaseStructure.TIM_Period=9;//�Զ����ؼĴ���ֵΪ1
		TIM_TimeBaseStructure.TIM_Prescaler=(42000-1);//ʱ��Ԥ��Ƶϵ��1
    TIM_TimeBaseStructure.TIM_ClockDivision = 0;
    TIM_TimeBaseStructure.TIM_CounterMode = TIM_CounterMode_Up;
    
    TIM_TimeBaseInit(TIM3, &TIM_TimeBaseStructure);

    TIM_ClearITPendingBit(TIM3, TIM_IT_Update);

	/* TIM IT enable */ //������ж�
    TIM_ITConfig(TIM3, TIM_IT_Update, ENABLE);      


	 /* Enable the TIM4 gloabal Interrupt */
    NVIC_InitStructure.NVIC_IRQChannel = TIM4_IRQn;
    NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0;
    NVIC_InitStructure.NVIC_IRQChannelSubPriority =0;
    NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
    NVIC_Init(&NVIC_InitStructure);

    RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM4, ENABLE);      //    TIM4 clock enable 
    
    /* Time base configuration */
    TIM_TimeBaseStructure.TIM_Period=9;
    TIM_TimeBaseStructure.TIM_Prescaler=(42000*2/5-1);   //  2ms
    TIM_TimeBaseStructure.TIM_ClockDivision = 0;
    TIM_TimeBaseStructure.TIM_CounterMode = TIM_CounterMode_Up;
    
    TIM_TimeBaseInit(TIM4, &TIM_TimeBaseStructure);

    TIM_ClearITPendingBit(TIM4, TIM_IT_Update);

	/* TIM IT enable */ //������ж�
    TIM_ITConfig(TIM4, TIM_IT_Update, ENABLE); 
}

void TIM3_Enable(void)
{
	/* TIM3 enable counter */
  TIM_Cmd(TIM3, ENABLE); 
	TIM3_State=1;
}

void TIM3_Disable(void)
{
	/* TIM3 enable counter */
  TIM_Cmd(TIM3, DISABLE); 
	TIM3_State=0;
}

void TIM4_Enable(void)
{
	/* TIM4 enable counter */
  TIM_Cmd(TIM4, ENABLE); 
	TIM4_State=1;
}

void TIM4_Disable(void)
{
	/* TIM4 enable counter */
  TIM_Cmd(TIM4, DISABLE); 
	TIM4_State=0;
}

void TIM3_IRQHandler(void)
{//5ms�ж�
	int i=0;
	float f_value=0;
	unsigned int tmp=0;
	int DIreg=0;
	int Itmp=0;
	short Axis=0;
	if(cputype==CPU2001_2411) Axis=NA2411_Axis_MAX;
	else Axis=NA240X_Axis_MAX;
	
	if (TIM_GetITStatus(TIM3, TIM_IT_Update) != RESET)
	{
		TIM_ClearITPendingBit(TIM3, TIM_IT_Update);
		for(i=0;i<Axis;i++)
		{
			if((PTO_pagram_run[i].P_plus_EN==1)&&(PTO_pagram_run[i].P_state==1))
			{
				if(PTO_pagram_run[i].P_cur_FB_mode==PTO_PLSY)
				{
					if(PTO_pagram_set[i].Limit_Protection==1)
					{
						DIreg=DI_16_Read();
						DIreg=DIreg&0x0fff0;
						if(i==0) DIreg=DIreg>>4;//��1��
						else if(i==1) DIreg=DIreg>>7; //��2��
						else if(i==2) DIreg=DIreg>>10; //��3��
						else DIreg=DIreg>>13; //��4��
						if((DIreg&0x03)!=0) PTO_pagram_run[i].Exception_Code=1;
					}
				}
				else if(PTO_pagram_run[i].P_cur_FB_mode==PTO_PLSR)
				{
					if(PTO_pagram_set[i].Limit_Protection==1)
					{
						DIreg=DI_16_Read();
						DIreg=DIreg&0x0fff0;
						if(i==0) DIreg=DIreg>>4;//��1��
						else if(i==1) DIreg=DIreg>>7; //��2��
						else if(i==2) DIreg=DIreg>>10; //��3��
						else DIreg=DIreg>>13; //��4��
						if((DIreg&0x03)!=0) PTO_pagram_run[i].Exception_Code=1;
					}
					PTO_pagram_run[i].PON_cur=PTO_PlusCurNum_Read(i);
					if((PTO_pagram_run[i].PON_cur<PTO_pagram_run[i].Plus_AD)&&(PTO_pagram_run[i].P_Stop_Req==0))
					{
						if(PTO_pagram_run[i].Freq_cur<PTO_pagram_run[i].Freq_target)
						{
							PTO_pagram_run[i].Freq_cur=PTO_pagram_run[i].Freq_cur+PTO_pagram_run[i].ADT_cur;
						}
						else PTO_pagram_run[i].Freq_cur=PTO_pagram_run[i].Freq_target;
						PTO_pagram_run[i].P_Dec_flag=1;
					}
					else if(PTO_pagram_run[i].P_Dec_flag==1) PTO_pagram_run[i].P_Dec_flag=0;
					else
					{
						if(PTO_pagram_run[i].Freq_cur>(PTO_pagram_set[i].Freq_min+PTO_pagram_run[i].ADT_cur))
						{
							PTO_pagram_run[i].Freq_cur=PTO_pagram_run[i].Freq_cur-PTO_pagram_run[i].ADT_cur;
						}
						else PTO_pagram_run[i].Freq_cur=PTO_pagram_set[i].Freq_min;
					}
					PTO_Freq_Set(i,PTO_pagram_run[i].Freq_cur);
				}
				else if(PTO_pagram_run[i].P_cur_FB_mode==PTO_ZRN)
				{
					DIreg=DI_16_Read();
					DIreg=DIreg&0x0fff0;
					if(i==0) DIreg=DIreg>>4;//��1��
					else if(i==1) DIreg=DIreg>>7; //��2��
					else if(i==2) DIreg=DIreg>>10; //��3��
					else DIreg=DIreg>>13; //��4��
					if(DIreg&0x01)
					{//��⵽�����Կ���
						if(PTO_pagram_run[i].DI_X1_Flag==0) PTO_pagram_run[i].P_Stop_Req=1;	//����������ֹͣ�������˶�
						PTO_pagram_run[i].DI_X1_Flag=1;
					}
					else PTO_pagram_run[i].DI_X1_Flag=0;
					if(DIreg&0x02)
					{//��⵽�����Կ���
						if(PTO_pagram_run[i].DI_X2_Flag==0) PTO_pagram_run[i].P_Stop_Req=2;	//����������ֹͣ�������˶�
						PTO_pagram_run[i].DI_X2_Flag=1;
					}
					else PTO_pagram_run[i].DI_X2_Flag=0;
					if(DIreg&0x04)
					{//�������ԭ�㿪��
						if(PTO_pagram_run[i].DI_X3_Flag==0) 
						{
							if(PTO_pagram_run[i].P_Dir==0) PTO_pagram_run[i].P_Stop_Req=3;	 //�Ӹ�������ԭ���������˶�����⵽ԭ�㿪�غ��
							else PTO_pagram_run[i].P_Stop_Req=4;	 //����������ԭ�㷴�����˶�����⵽ԭ�㿪��ǰ��
						}
						PTO_pagram_run[i].DI_X3_Flag=1;
					}
					else 
					{
						if(PTO_pagram_run[i].DI_X3_Flag==1) 
						{//����뿪ԭ�㿪��
							if(PTO_pagram_run[i].P_Dir==1) PTO_pagram_run[i].P_Stop_Req=5;	//��⵽ԭ��λ��
						}
						PTO_pagram_run[i].DI_X3_Flag=0;
					}
					
					if(PTO_pagram_run[i].P_Stop_Req==0)
					{//�����˶�
						if(PTO_pagram_run[i].Freq_cur<PTO_pagram_run[i].Freq_target)
						{
							PTO_pagram_run[i].Freq_cur=PTO_pagram_run[i].Freq_cur+PTO_pagram_run[i].ADT_cur;
						}
						else PTO_pagram_run[i].Freq_cur=PTO_pagram_run[i].Freq_target;
						PTO_Freq_Set(i,PTO_pagram_run[i].Freq_cur);
					}
					else if(PTO_pagram_run[i].P_Stop_Req<=3)
					{
						if(PTO_pagram_run[i].Freq_cur>(PTO_pagram_set[i].Freq_min+PTO_pagram_run[i].ADT_cur))
						{
							PTO_pagram_run[i].Freq_cur=PTO_pagram_run[i].Freq_cur-PTO_pagram_run[i].ADT_cur;
							PTO_Freq_Set(i,PTO_pagram_run[i].Freq_cur);
						}
						else 
						{
							PTO_pagram_run[i].Freq_cur=PTO_pagram_set[i].Freq_min;
							PTO_Rst_set(i);
							if(PTO_pagram_run[i].P_Dir==0)
							{
								PTO_Dir_Set(i,1);
								PTO_pagram_run[i].P_Dir=1;
								if(PTO_pagram_run[i].P_Stop_Req==3) PTO_pagram_run[i].P_Stop_Req=4;
								else PTO_pagram_run[i].P_Stop_Req=0;
							}
							else
							{
								PTO_Dir_Set(i,0);
								PTO_pagram_run[i].P_Dir=0;
								PTO_pagram_run[i].P_Stop_Req=0;
							}
							PTO_Freq_Set(i,PTO_pagram_run[i].Freq_cur);
							PTO_Start_set(i);
						}
					}
					else if(PTO_pagram_run[i].P_Stop_Req==4)
					{
						if(PTO_pagram_run[i].Freq_cur>(PTO_pagram_set[i].Freq_min+PTO_pagram_run[i].ADT_cur))
						{
							PTO_pagram_run[i].Freq_cur=PTO_pagram_run[i].Freq_cur-PTO_pagram_run[i].ADT_cur;
						}
						else PTO_pagram_run[i].Freq_cur=PTO_pagram_set[i].Freq_min;
						PTO_Freq_Set(i,PTO_pagram_run[i].Freq_cur);
					}
					else
					{
						PTO_Rst_set(i);
						PTO_pagram_run[i].P_state=0;
					}
				}
				else if(PTO_pagram_run[i].P_cur_FB_mode==PTO_PLSV)
				{
					if(PTO_pagram_run[i].P_Stop_Req==0)
					{
						if(PTO_pagram_run[i].Freq_cur<PTO_pagram_run[i].Freq_target)
						{
							if(PTO_pagram_run[i].Freq_cur<PTO_pagram_run[i].Freq_target-PTO_pagram_run[i].ADT_cur)
							{
								PTO_pagram_run[i].Freq_cur=PTO_pagram_run[i].Freq_cur+PTO_pagram_run[i].ADT_cur;
							}
							else PTO_pagram_run[i].Freq_cur=PTO_pagram_run[i].Freq_target;
						}
						else
						{
							if(PTO_pagram_run[i].Freq_cur>(PTO_pagram_run[i].Freq_target+PTO_pagram_run[i].ADT_cur))
							{
								PTO_pagram_run[i].Freq_cur=PTO_pagram_run[i].Freq_cur-PTO_pagram_run[i].ADT_cur;
							}
							else PTO_pagram_run[i].Freq_cur=PTO_pagram_run[i].Freq_target;
						}
					}
					else
					{
						if(PTO_pagram_run[i].Freq_cur>(PTO_pagram_set[i].Freq_min+PTO_pagram_run[i].ADT_cur))
						{
							PTO_pagram_run[i].Freq_cur=PTO_pagram_run[i].Freq_cur-PTO_pagram_run[i].ADT_cur;
						}
						else PTO_pagram_run[i].Freq_cur=PTO_pagram_set[i].Freq_min;
					}
					PTO_Freq_Set(i,PTO_pagram_run[i].Freq_cur);
				}
				else if(PTO_pagram_run[i].P_cur_FB_mode==PTO_MPTO)
				{
					if(PTO_pagram_set[i].Limit_Protection==1)
					{
						DIreg=DI_16_Read();
						DIreg=DIreg&0x0fff0;
						if(i==0) DIreg=DIreg>>4;//��1��
						else if(i==1) DIreg=DIreg>>7; //��2��
						else if(i==2) DIreg=DIreg>>10; //��3��
						else DIreg=DIreg>>13; //��4��
						if((DIreg&0x03)!=0) PTO_pagram_run[i].Exception_Code=1;
					}
					PTO_pagram_run[i].PON_cur=PTO_PlusCurNum_Read(i);
					if(PTO_pagram_run[i].PON_cur>=PTO_pagram_run[i].P_MPTO_PlusSum)
					{
						if(PTO_pagram_run[i].PON_cur<PTO_pagram_run[i].PlusSum_target)
						{
							PTO_pagram_run[i].P_MPTO_index=PTO_pagram_run[i].P_MPTO_index+1;
							if(PTO_pagram_run[i].P_INC==0) 
							{
								Itmp=(PTO_pagram_run[i].P_MPTO_TBL[2*PTO_pagram_run[i].P_MPTO_index+1]-PTO_pagram_run[i].P_MPTO_TBL[2*PTO_pagram_run[i].P_MPTO_index-1]);
								if(Itmp>0) PTO_pagram_run[i].P_MPTO_PlusSum = PTO_pagram_run[i].P_MPTO_PlusSum+Itmp;
								else PTO_pagram_run[i].P_MPTO_PlusSum =PTO_pagram_run[i].P_MPTO_PlusSum-Itmp;
							}
							else 
							{
								if(PTO_pagram_run[i].P_Dir==0) PTO_pagram_run[i].P_MPTO_PlusSum +=PTO_pagram_run[i].P_MPTO_TBL[2*PTO_pagram_run[i].P_MPTO_index+1];
								else PTO_pagram_run[i].P_MPTO_PlusSum -=PTO_pagram_run[i].P_MPTO_TBL[2*PTO_pagram_run[i].P_MPTO_index+1];
							}
							PTO_pagram_run[i].Freq_target=PTO_pagram_run[i].P_MPTO_TBL[2*PTO_pagram_run[i].P_MPTO_index];
							if(PTO_pagram_run[i].P_MPTO_Time==0) PTO_pagram_run[i].Freq_cur=PTO_pagram_run[i].Freq_target;
							else
							{
								if(PTO_pagram_run[i].Freq_cur<PTO_pagram_run[i].Freq_target){
								PTO_pagram_run[i].ADT_cur=(PTO_pagram_run[i].Freq_target-PTO_pagram_run[i].Freq_cur)*5/PTO_pagram_run[i].P_MPTO_Time;
								}
								else PTO_pagram_run[i].ADT_cur=(PTO_pagram_run[i].Freq_cur-PTO_pagram_run[i].Freq_target)*5/PTO_pagram_run[i].P_MPTO_Time;
							}
						}
					}
					if((PTO_pagram_run[i].PON_cur<PTO_pagram_run[i].Plus_AD)&&(PTO_pagram_run[i].P_Stop_Req==0))
					{
						if(PTO_pagram_run[i].Freq_cur<=PTO_pagram_run[i].Freq_target)
						{
							if(PTO_pagram_run[i].Freq_cur<(PTO_pagram_run[i].Freq_target-PTO_pagram_run[i].ADT_cur))
							{
								PTO_pagram_run[i].Freq_cur=PTO_pagram_run[i].Freq_cur+PTO_pagram_run[i].ADT_cur;
							}
							else PTO_pagram_run[i].Freq_cur=PTO_pagram_run[i].Freq_target;
							PTO_pagram_run[i].P_Dec_flag=1;
						}
						else if(PTO_pagram_run[i].P_Dec_flag==1) PTO_pagram_run[i].P_Dec_flag=0;
						else 
						{
							if(PTO_pagram_run[i].Freq_cur>(PTO_pagram_run[i].Freq_target+PTO_pagram_run[i].ADT_cur))
							{
								PTO_pagram_run[i].Freq_cur=PTO_pagram_run[i].Freq_cur-PTO_pagram_run[i].ADT_cur;
							}
							else PTO_pagram_run[i].Freq_cur=PTO_pagram_run[i].Freq_target;
						}
					}
					else if(PTO_pagram_run[i].P_Dec_flag==1) 
					{
						PTO_pagram_run[i].P_Dec_flag=0;
						PTO_pagram_run[i].ADT_cur=(PTO_pagram_run[i].Freq_cur-PTO_pagram_set[i].Freq_min)*5/PTO_pagram_run[i].P_MPTO_Time;
					}
					else
					{
						if(PTO_pagram_run[i].Freq_cur>(PTO_pagram_set[i].Freq_min+PTO_pagram_run[i].ADT_cur))
						{
							PTO_pagram_run[i].Freq_cur=PTO_pagram_run[i].Freq_cur-PTO_pagram_run[i].ADT_cur;
						}
						else PTO_pagram_run[i].Freq_cur=PTO_pagram_set[i].Freq_min;
					}
					
					PTO_Freq_Set(i,PTO_pagram_run[i].Freq_cur);
				}
				else if(PTO_pagram_run[i].P_cur_FB_mode==PTO_PSTOP)
				{
					if(PTO_pagram_run[i].P_Stop_Req==1)
					{
						if(PTO_pagram_run[i].Freq_cur>(PTO_pagram_set[i].Freq_min+PTO_pagram_run[i].ADT_cur))
						{
							PTO_pagram_run[i].Freq_cur=PTO_pagram_run[i].Freq_cur-PTO_pagram_run[i].ADT_cur;
						}
						else PTO_pagram_run[i].Freq_cur=PTO_pagram_set[i].Freq_min;
					}
					PTO_Freq_Set(i,PTO_pagram_run[i].Freq_cur);
				}
			}
		}
		Count_10ms_flag++;
		if(Count_10ms_flag>=2)
		{
			Count_10ms_flag=0;
			for(i=0;i<2;i++)
			{
				if((HC_pagram_set[i].En==1)&&(HC_pagram_set[i].Mode==0)&&(HC_pagram_run[i].T_Freq_Period>0)&&(HC_pagram_run[i].T_Freq_Period<=200))
				{
					if(HC_pagram_run[i].ptr>=250) HC_pagram_run[i].ptr=0;
					HC_pagram_run[i].T_Freq_buf[HC_pagram_run[i].ptr]=HC_Cur_Value_Read(i);
					if(HC_pagram_run[i].ptr<HC_pagram_run[i].T_Freq_Period) tmp=HC_pagram_run[i].ptr+250-HC_pagram_run[i].T_Freq_Period;
					else tmp=HC_pagram_run[i].ptr-HC_pagram_run[i].T_Freq_Period;
					if(HC_pagram_run[i].T_Freq_buf[HC_pagram_run[i].ptr]>=HC_pagram_run[i].T_Freq_buf[tmp])
					{
						tmp=HC_pagram_run[i].T_Freq_buf[HC_pagram_run[i].ptr]-HC_pagram_run[i].T_Freq_buf[tmp];
					}
					else
					{
						tmp=0xffffffff-HC_pagram_run[i].T_Freq_buf[tmp]+1+HC_pagram_run[i].T_Freq_buf[HC_pagram_run[i].ptr];
					}
					f_value=100/((float)(HC_pagram_run[i].T_Freq_Period));
					HC_pagram_run[i].T_Freq_Value=tmp*f_value;
					HC_pagram_run[i].ptr++;
				}
				else HC_pagram_run[i].T_Freq_Value=0;
			}
		}
	}
}


void TIM4_IRQHandler(void)
{// 2ms�ж�
	int i=0;
	float f_value=0;
	unsigned int tmp=0;
	int DIreg=0;
	int Itmp=0;
	short Axis=0;
	if(cputype==CPU2001_2411) Axis=NA2411_Axis_MAX;
	else Axis=NA240X_Axis_MAX;
	
	if (TIM_GetITStatus(TIM4, TIM_IT_Update) != RESET)
	{
		TIM_ClearITPendingBit(TIM4, TIM_IT_Update);
		for(i=0;i<Axis;i++)
		{
			if((PTO_pagram_run[i].P_plus_EN==1)&&(PTO_pagram_run[i].P_state==1))
			{
				if(PTO_pagram_run[i].P_cur_FB_mode==PTO_PLIN)
				{
					if(PTO_pagram_set[i].Limit_Protection==1)
					{
						DIreg=DI_16_Read();
						DIreg=DIreg&0x0fff0;
						if(i==0) DIreg=DIreg>>4;//��1��
						else if(i==1) DIreg=DIreg>>7; //��2��
						else if(i==2) DIreg=DIreg>>10; //��3��
						else DIreg=DIreg>>13; //��4��
						if((DIreg&0x03)!=0) PTO_pagram_run[i].Exception_Code=1;
					}
				}
			}
		}
	}
}
////////////////////////////////////////////////////
static void GPIO_Config()
{
	GPIO_InitTypeDef GPIO_InitStructure;  
	/* Enable GPIOA clock */  
	RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOA |RCC_AHB1Periph_GPIOE| RCC_AHB1Periph_GPIOG,ENABLE);
	
	/* Enable SYSCFG clock */
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_SYSCFG, ENABLE);
	
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_3 | GPIO_Pin_4;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
	GPIO_InitStructure.GPIO_PuPd  = GPIO_PuPd_NOPULL;
	GPIO_Init(GPIOA, &GPIO_InitStructure);
	
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_1;
	GPIO_Init(GPIOE, &GPIO_InitStructure); 
	
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_10 | GPIO_Pin_11;
	GPIO_Init(GPIOG, &GPIO_InitStructure); 
}

//configure sd hotplug gpio pin:sd_cd use gpiob7
static void  EXTIX_Init()
{
	EXTI_InitTypeDef EXTI_InitStructure; 
	NVIC_InitTypeDef NVIC_InitStructure;  	
  
	GPIO_Config();
	
	SYSCFG_EXTILineConfig(EXTI_PortSourceGPIOA,GPIO_PinSource3);
	SYSCFG_EXTILineConfig(EXTI_PortSourceGPIOA,GPIO_PinSource4);
	SYSCFG_EXTILineConfig(EXTI_PortSourceGPIOE,GPIO_PinSource1);
	SYSCFG_EXTILineConfig(EXTI_PortSourceGPIOG,GPIO_PinSource10);
	SYSCFG_EXTILineConfig(EXTI_PortSourceGPIOG,GPIO_PinSource11);

	EXTI_InitStructure.EXTI_Line = EXTI_Line1;  //LINE1
	EXTI_InitStructure.EXTI_Mode = EXTI_Mode_Interrupt;  //�ж��¼�
	EXTI_InitStructure.EXTI_Trigger = EXTI_Trigger_Rising;  //�����ش��� 
	EXTI_InitStructure.EXTI_LineCmd = ENABLE;  //ʹ��LINE1
	EXTI_Init(&EXTI_InitStructure);  //����
	 
	EXTI_InitStructure.EXTI_Line = EXTI_Line3;  //LINE3
	EXTI_InitStructure.EXTI_Mode = EXTI_Mode_Interrupt;  //�ж��¼�
	EXTI_InitStructure.EXTI_Trigger = EXTI_Trigger_Rising;  //�����ش��� 
	EXTI_InitStructure.EXTI_LineCmd = ENABLE;  //ʹ��LINE3
	EXTI_Init(&EXTI_InitStructure);  //����
	
	EXTI_InitStructure.EXTI_Line = EXTI_Line4;  //LINE4
	EXTI_InitStructure.EXTI_Mode = EXTI_Mode_Interrupt;  //�ж��¼�
	EXTI_InitStructure.EXTI_Trigger = EXTI_Trigger_Falling;  //�����ش��� 
	EXTI_InitStructure.EXTI_LineCmd = ENABLE;  //ʹ��LINE3
	EXTI_Init(&EXTI_InitStructure);  //����
	 
	EXTI_InitStructure.EXTI_Line = EXTI_Line10;  //LINE10
	EXTI_InitStructure.EXTI_Mode = EXTI_Mode_Interrupt;  //�ж��¼�
	EXTI_InitStructure.EXTI_Trigger = EXTI_Trigger_Rising;  //�����ش��� 
	EXTI_InitStructure.EXTI_LineCmd = ENABLE;  //ʹ��LINE10
	EXTI_Init(&EXTI_InitStructure);  //����

	NVIC_InitStructure.NVIC_IRQChannel = EXTI1_IRQn; //�ⲿ�ж�1
	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0; //�����ȼ�2
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0; //��ռ���ȼ�0
	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
	NVIC_Init(&NVIC_InitStructure);
	
	NVIC_InitStructure.NVIC_IRQChannel = EXTI3_IRQn; //�ⲿ�ж�3
	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0; //�����ȼ�2
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0; //��ռ���ȼ�0
	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
	NVIC_Init(&NVIC_InitStructure);
	
	NVIC_InitStructure.NVIC_IRQChannel = EXTI4_IRQn; //�ⲿ�ж�4
	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0; //�����ȼ�2
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0; //��ռ���ȼ�0
	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
	NVIC_Init(&NVIC_InitStructure);
	 
	NVIC_InitStructure.NVIC_IRQChannel = EXTI15_10_IRQn; //�ⲿ�ж�10,11
	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0; //�����ȼ�2
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0; //��ռ���ȼ�0
	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
	NVIC_Init(&NVIC_InitStructure);
}	

void active_int_prg(unsigned char irqno)
{
	irqnos[irqno - INT_UP_DI13] = irqno;
	rt_mb_send(int_prg_mb,(rt_uint32_t)&irqnos[irqno - INT_UP_DI13]);
}

void call_int_prg(unsigned char irqno)
{
	int rc;
	if(pStaticData->int_enable_flag && !Get_S(MST_STOP1))
	{
		if(pStaticData->IntPrgNo[irqno-1] <= pPrgData->PrgNum && pStaticData->IntPrgNo[irqno - 1] >0)
		{
			if(rt_sem_take(sem_prg,RT_WAITING_FOREVER) == RT_EOK )
			{
				rt_sem_release(sem_prg);

				rc=PrgEntry[pStaticData->IntPrgNo[irqno-1]-1]();
				pPrgData->CurrentStep[pStaticData->IntPrgNo[irqno-1]-1]=0;
				if (rc)
				{
					pDynamicData->pSW.Value[EXECERR_INFO-1] = pStaticData->IntPrgNo[irqno-1];
					pDynamicData->pSW.Value[EXECERR_INFO] = rc;
					pDynamicData->pSW.Value[EXECERR_INFO+1] = 0;
					Output_S(LD_EXECERR, 1);
				}
				set_hsc_param();
			}
		}
	}
}

void store_backup_data()
{
	int i=0;
	int start_index = 0;
	unsigned short *pStartBackup = (unsigned short *)(BKPSRAM_BASE + 2048-900);
	if(pConfigData->RetDef.TOccno > 0 && pConfigData->RetDef.TNum > 0)
	{
		for(i=0;i<pConfigData->RetDef.TNum; i++)
		{
			start_index = pConfigData->RetDef.TOccno + i - 1;
			*(unsigned short *)(pStartBackup) = pDynamicData->pT.CtrlWord[start_index];
			*(unsigned short *)(pStartBackup + 1) =  pDynamicData->pT.PreValue[start_index] & 0xFFFF;
			*(unsigned short *)(pStartBackup + 2) =  (pDynamicData->pT.PreValue[start_index] >> 16) & 0xFFFF;
			*(unsigned short *)(pStartBackup + 3) =  pDynamicData->pT.CurValue[start_index] & 0xFFFF;
			*(unsigned short *)(pStartBackup + 4) =  (pDynamicData->pT.CurValue[start_index] >> 16) & 0xFFFF;
			pStartBackup += 5;
		}
	}
	
	if(pConfigData->RetDef.COccno > 0 && pConfigData->RetDef.CNum > 0)
	{
		for(i=0;i<pConfigData->RetDef.CNum; i++)
		{
			start_index = pConfigData->RetDef.COccno + i - 1;
			*(unsigned short *)(pStartBackup) = pDynamicData->pC.CtrlWord[start_index];
			*(unsigned short *)(pStartBackup + 1) =  pDynamicData->pC.PreValue[start_index];
			*(unsigned short *)(pStartBackup + 2) =  pDynamicData->pC.CurValue[start_index];
			pStartBackup += 3;
		}
	}
	
	if(pConfigData->RetDef.HscOccno > 0 && pConfigData->RetDef.HscNum > 0)
	{
		for(i=0;i<pConfigData->RetDef.HscNum; i++)
		{
			start_index = pConfigData->RetDef.HscOccno + i - 1;
			memcpy(pStartBackup,&hsc_data[start_index],sizeof(HSC_DATA));
			pStartBackup += sizeof(HSC_DATA)/2;
		}
	}
	
	if(pConfigData->RetDef.PtoOccno > 0 && pConfigData->RetDef.PtoNum > 0)
	{
		for(i=0;i<pConfigData->RetDef.PtoNum; i++)
		{
			start_index = pConfigData->RetDef.PtoOccno + i - 1;
			memcpy(pStartBackup,&pto_data[start_index],sizeof(PTO_DATA));
			pStartBackup += sizeof(PTO_DATA)/2;
		}
	}
}

void restore_backup_data()
{
	int i=0;
	int start_index = 0;
	unsigned short *pStartBackup = (unsigned short *)(BKPSRAM_BASE + 2048-900);
	if(pConfigData->RetDef.TOccno > 0 && pConfigData->RetDef.TNum > 0)
	{
		for(i=0;i<pConfigData->RetDef.TNum; i++)
		{
			start_index = pConfigData->RetDef.TOccno + i - 1;
			pDynamicData->pT.CtrlWord[start_index] = *(unsigned short *)(pStartBackup);
			pDynamicData->pT.PreValue[start_index] = *(unsigned short *)(pStartBackup + 1);
			pDynamicData->pT.PreValue[start_index] +=(*(unsigned short *)(pStartBackup + 2) << 16);
			pDynamicData->pT.CurValue[start_index] = *(unsigned short *)(pStartBackup + 3);
			pDynamicData->pT.CurValue[start_index] +=(*(unsigned short *)(pStartBackup + 4) << 16);
			pStartBackup += 5;
		}
	}
	
	if(pConfigData->RetDef.COccno > 0 && pConfigData->RetDef.CNum > 0)
	{
		for(i=0;i<pConfigData->RetDef.CNum; i++)
		{
			start_index = pConfigData->RetDef.COccno + i - 1;
			pDynamicData->pC.CtrlWord[start_index] = *(unsigned short *)(pStartBackup);
			pDynamicData->pC.PreValue[start_index] = *(unsigned short *)(pStartBackup + 1);
			pDynamicData->pC.CurValue[start_index] = *(unsigned short *)(pStartBackup + 2);
			pStartBackup += 3;
		}
	}
	
	if(pConfigData->RetDef.HscOccno > 0 && pConfigData->RetDef.HscNum > 0)
	{
		for(i=0;i<pConfigData->RetDef.HscNum; i++)
		{
			start_index = pConfigData->RetDef.HscOccno + i - 1;
			memcpy(&hsc_data[start_index],pStartBackup,sizeof(HSC_DATA));
			pStartBackup += sizeof(HSC_DATA)/2;

			HC_pagram_run[start_index].T_Init_Value=hsc_data[start_index].ini_value;
			HC_pagram_run[start_index].T_Pre_Value=hsc_data[start_index].pre_value;
			HC_pagram_run[start_index].T_Freq_Period=hsc_data[start_index].ini_value;
			HC_pagram_run[start_index].T_Cur_Value = hsc_data[start_index].cur_value;

			HC_Init_Value_set(start_index,hsc_data[start_index].cur_value);
			HC_Pre_Value_set(start_index,hsc_data[start_index].pre_value);

			if (pConfigData->HscVType==PARAMTYPE_R)
			{
				memcpy(&pDynamicData->pR.Value[start_index],hsc_data+start_index,sizeof(HSC_DATA));
			}
			else if (pConfigData->HscVType==PARAMTYPE_NR)
			{
				memcpy(&pNvramData->pNR.Value[start_index],hsc_data+start_index,sizeof(HSC_DATA));
			}
			else if (pConfigData->HscVType==PARAMTYPE_V)
			{
				memcpy(&pDynamicData->pVAR[start_index],hsc_data+start_index,sizeof(HSC_DATA));
			}
		}

	}
	
	if(pConfigData->RetDef.PtoOccno > 0 && pConfigData->RetDef.PtoNum > 0)
	{
		for(i=0;i<pConfigData->RetDef.PtoNum; i++)
		{
			start_index = pConfigData->RetDef.PtoOccno + i - 1;
			memcpy(&pto_data[start_index],pStartBackup,sizeof(PTO_DATA));
			pStartBackup += sizeof(PTO_DATA)/2;

			PTO_pagram_run[start_index].POS_cur = pto_data[start_index].abs_pos;
			PTO_pagram_run[start_index].PON_cur = pto_data[start_index].inc_pos;
		}
	}
}

void EXTI4_IRQHandler(void)  
{  
	//unsigned int  IQR_State_tmp;
	//unsigned int  tmp=0;
	//int i=0;
	if(EXTI_GetITStatus(EXTI_Line4) != RESET)  
	{
		/* Clear the EXTI line 4 pending bit */  
		EXTI_ClearITPendingBit(EXTI_Line4);
		store_backup_data();
		rt_kprintf("power down\r\n");
//		while(1)
//		{
//			for(i=0;i<1000;i++) IQR_State_tmp=CPLD_Read(IQR_State);
//			pNvramData->pNR.Value[20] = tmp;
//			tmp++;
//		}
	}
}
void EXTI3_IRQHandler(void)  
{  
	unsigned int DI_tmp=0;
	unsigned int IQR_State_tmp=0;
	if(EXTI_GetITStatus(EXTI_Line3) != RESET)  
	{
		/* Clear the EXTI line 3 pending bit */  
		EXTI_ClearITPendingBit(EXTI_Line3); 
		//rt_kprintf("Int3!\n");
		
		IQR_State_tmp=CPLD_Read(IQR_State);
		while(IQR_State_tmp != 0)
		{
			DI_tmp=DI_16_Read();
			DI_tmp=DI_tmp&0xf000;
			if(IQR_State_tmp&0x1000)
			{//DI12�ж�
				if(DI_tmp&0x1000)
				{//DI12������
					active_int_prg(INT_UP_DI13);
				}
				else
				{//DI12�½���
					active_int_prg(INT_DOWN_DI13);
				}
				CPLD_Write(IQR_Rst,0x1000);
				CPLD_Read(IQR_State);
			}
			if(IQR_State_tmp&0x2000)
			{//DI13�ж�
				if(DI_tmp&0x2000)
				{//DI13������
					active_int_prg(INT_UP_DI14);
				}
				else
				{//DI13�½���
					active_int_prg(INT_DOWN_DI14);
				}
				CPLD_Write(IQR_Rst,0x2000);
				CPLD_Read(IQR_State);
			}
			if(IQR_State_tmp&0x4000)
			{//DI14�ж�
				if(DI_tmp&0x4000)
				{//DI14������
					active_int_prg(INT_UP_DI15);
				}
				else
				{//DI14�½���
					active_int_prg(INT_DOWN_DI15);
				}
				CPLD_Write(IQR_Rst,0x4000);
				CPLD_Read(IQR_State);
			}
			if(IQR_State_tmp&0x8000)
			{//DI15�ж�
				if(DI_tmp&0x8000)
				{//DI15������
					active_int_prg(INT_UP_DI16);
				}
				else
				{//DI15�½���
					active_int_prg(INT_DOWN_DI16);
				}
				CPLD_Write(IQR_Rst,0x8000);
				CPLD_Read(IQR_State);
			}
			DI_tmp=DI_16_Read();
			DI_tmp=DI_tmp&0xf000;
			IQR_State_tmp=CPLD_Read(IQR_State);
			IQR_State_tmp=IQR_State_tmp&0xf000;
		}
	}
}
void EXTI1_IRQHandler(void)  
{  
	unsigned int  IQR_State_tmp;
	if(EXTI_GetITStatus(EXTI_Line1) != RESET)  
	{
		/* Clear the EXTI line 1 pending bit */ 
		EXTI_ClearITPendingBit(EXTI_Line1); 
		IQR_State_tmp=CPLD_Read(IQR_State);
		while(IQR_State_tmp!=0)
		{
			if(IQR_State_tmp&0x10)
			{	
				//active_int_prg(INT_PTO1);
				PTO1_INT_Flag=1;
				CPLD_Write(IQR_Rst,0x10);	
				CPLD_Read(IQR_State);			
			}
			if(IQR_State_tmp&0x20)
			{
				//active_int_prg(INT_PTO2);
				PTO2_INT_Flag=1;
				CPLD_Write(IQR_Rst,0x20);
				CPLD_Read(IQR_State);	
			}
			if(IQR_State_tmp&0x40)
			{
				//active_int_prg(INT_PTO3);
				PTO3_INT_Flag=1;
				CPLD_Write(IQR_Rst,0x40);
				CPLD_Read(IQR_State);	
			}
			if(IQR_State_tmp&0x80)
			{
				//active_int_prg(INT_PTO4);
				PTO4_INT_Flag=1;
				CPLD_Write(IQR_Rst,0x80);
				CPLD_Read(IQR_State);	
			}
			IQR_State_tmp=CPLD_Read(IQR_State);	
			IQR_State_tmp=IQR_State_tmp&0xf0;
		}
		switch(gbiax)
		{
			case Q1Q2 :
				if(PTO1_INT_Flag==1&&PTO2_INT_Flag==1)
				rt_sem_release(sem_lineprg);
				break;
			case Q1Q3 :
				if(PTO1_INT_Flag==1&&PTO3_INT_Flag==1)
				rt_sem_release(sem_lineprg);
				break;
			case Q1Q4 :
				if(PTO1_INT_Flag==1&&PTO4_INT_Flag==1)
				rt_sem_release(sem_lineprg);
				break;
			case Q2Q3 :
				if(PTO2_INT_Flag==1&&PTO3_INT_Flag==1)
				rt_sem_release(sem_lineprg);
				break;
			case Q2Q4 :
				if(PTO2_INT_Flag==1&&PTO4_INT_Flag==1)
				rt_sem_release(sem_lineprg);
				break;
			case Q3Q4 :
				if(PTO3_INT_Flag==1&&PTO4_INT_Flag==1)
				rt_sem_release(sem_lineprg);
				break;
			default :
				break;

		}

	}
}

void EXTI15_10_IRQHandler(void)  
{  
	unsigned int  IQR_State_tmp;
	if(EXTI_GetITStatus(EXTI_Line10) != RESET)  
	{
		/* Clear the EXTI line 10 pending bit */  
		EXTI_ClearITPendingBit(EXTI_Line10); 
		get_hsc_status();
		IQR_State_tmp=CPLD_Read(IQR_State);
		while(IQR_State_tmp!=0)
		{
			if(IQR_State_tmp&0x01)
			{			
				active_int_prg(INT_HSC1);		
				
				CPLD_Write(IQR_Rst,0x01);	
				CPLD_Read(IQR_State);			
			}
			if(IQR_State_tmp&0x02)
			{
				active_int_prg(INT_HSC2);
				CPLD_Write(IQR_Rst,0x02);
				CPLD_Read(IQR_State);	
			}
			IQR_State_tmp=CPLD_Read(IQR_State);	
			IQR_State_tmp=IQR_State_tmp&0x03;
		}
	}
}

void int_prg_srv(void *param)
{
		rt_uint8_t* pirqno;
		while(1)
		{
			rt_mb_recv(int_prg_mb,(rt_uint32_t*)&pirqno,RT_WAITING_FOREVER);
			call_int_prg(*pirqno);
		}
}

//CPLD��ʼ��
void Init_CPLD(void)
{
	unsigned int filter=0;
	short i=0;
	short Axis=0;
	if(cputype==CPU2001_2411) Axis=NA2411_Axis_MAX;
	else Axis=NA240X_Axis_MAX;
	
	for(i=0;i<Axis;i++)
	{
		PTO_pagram_set[i].Limit_Protection=pConfigData->PtoDef[i].Prt;
		PTO_pagram_set[i].Dir_polar=pConfigData->PtoDef[i].Polar;
		PTO_pagram_set[i].P_Stop_type=pConfigData->PtoDef[i].Stop;
		PTO_pagram_set[i].Time_max=pConfigData->PtoDef[i].MaxTime;
		PTO_pagram_set[i].Time_min=pConfigData->PtoDef[i].MinTime;
		PTO_pagram_set[i].Freq_max=pConfigData->PtoDef[i].MaxFrq;
		PTO_pagram_set[i].Freq_min=pConfigData->PtoDef[i].MinFrq;
		
		PTO_pagram_run[i].P_cur_FB_NO=0;
		PTO_pagram_run[i].POS_cur=0;
		PTO_pagram_run[i].PON_cur_reg=0;
	}
		
	CPLD_IO_Config(0,0,0);
	CPLD_IO_Config(IO_INT,0,0);	
	CPLD_IO_Config(IO_INT,1,0);	
	CPLD_IO_Config(IO_INT,2,0);	
	CPLD_IO_Config(IO_INT,3,0);	
	
	for(i=0;i<16;i++)
	{
		filter = filter << 2;
		filter |= pStaticData->pI[pStaticData->pMDU[mon_module-1].IRefAddr+i-1].FiltTime & 0x03;
	}	
	CPLD_Write(Filter_reg,filter);
	
	for(i=0;i<HC_MAX;i++)
	{
		if(pConfigData->HscDef[i].Mode>0) 
		{
			HC_pagram_set[i].En=1;
			HC_pagram_set[i].Int_En=1;
			HC_pagram_set[i].Mode=pConfigData->HscDef[i].Mode-1;
			HC_pagram_set[i].X1_X4=pConfigData->HscDef[i].X1X4;
		}
		else HC_pagram_set[i].En=0;
		HC_Init_Set(i,HC_pagram_set[i].Mode,HC_pagram_set[i].X1_X4,HC_pagram_set[i].Int_En);
	}
	restore_backup_data();
	
//	//////////////////////////////////////////////////////
//	//��ʼ��������ʱ��
	Tim_Init();
	EXTIX_Init();
	
	int_prg_mb = rt_mb_create("int_prg",MAX_INTS,RT_IPC_FLAG_FIFO);
	if(TIM3_State==0) TIM3_Enable();
	
	if(TIM4_State==0) TIM4_Enable();
	
	
		rt_thread_t tid = rt_thread_create("int_prg",int_prg_srv,RT_NULL,1024,15,20);
		if(tid)
			rt_thread_startup(tid);
	
}
uint8_t biax_check(uint8_t Ch0,uint8_t Ch1 )
{
	uint8_t biax=0;
	if((Ch0==PQ1 && Ch1==PQ2)||(Ch0==PQ2 && Ch1==PQ1))
		biax=Q1Q2;
	else if ((Ch0==PQ1 && Ch1==PQ3)||(Ch0==PQ3 && Ch1==PQ1))
		biax=Q1Q3;
	else if ((Ch0==PQ1 && Ch1==PQ4)||(Ch0==PQ4 && Ch1==PQ1))
		biax=Q1Q4;
	else if ((Ch0==PQ2 && Ch1==PQ3)||(Ch0==PQ3 && Ch1==PQ2))
		biax=Q2Q3;
	else if ((Ch0==PQ2 && Ch1==PQ4)||(Ch0==PQ4 && Ch1==PQ2))
		biax=Q2Q4;
	else if ((Ch0==PQ2 && Ch1==PQ4)||(Ch0==PQ4 && Ch1==PQ2))
		biax=Q3Q4;
	else biax=10;
	return biax;
}
int myresh(unsigned char pointtype, unsigned short occno, unsigned char num)
{
	if(pointtype == PARAMTYPE_I)
	{
		extern void di_process();
		di_process();
		get_hsc_status();
	}
	if(pointtype == PARAMTYPE_Q)
	{
		unsigned short no,bias;
		unsigned char dovalue;

		no=(pStaticData->pMDU[mon_module-1].QRefAddr-1)/8;
		bias=(pStaticData->pMDU[mon_module-1].QRefAddr-1)%8;

		if (bias==0)
			dovalue=pDynamicData->pQ.Output[no];
		else
			dovalue=(pDynamicData->pQ.Output[no]>>bias)|(pDynamicData->pQ.Output[no+1]<<(8-bias));

		DO_8_Write(dovalue);
	}
	
	return 0;
}
/////////////////mypwm���/////////////////////////////////////////
//en:���ܿ�ʹ��  Ch:PTOͨ����  freq:���Ƶ��  Duty:ռ�ձ�
int mypwm(int index, unsigned char Ch, unsigned char En, int Freq, int Duty)
{
	static int prttimes=0;
	if(prttimes<50&&En==1){

		printf("index=%d,Ch=%d, En=%d,Freq=%d,Duty=%d\n",index,Ch,En,Freq,Duty);
		prttimes++;
	}
	short Axis=0;
	if(cputype==CPU2001_2411) Axis=NA2411_Axis_MAX;
	else if(cputype==CPU2001_2401 || cputype==CPU2001_2403) Axis=NA240X_Axis_MAX;
	else return 1;
	if(Ch>=Axis) return 1;
	if((En==1)&&(PTO_pagram_run[Ch].P_PStop_flag==0))
	{			
		if((PTO_pagram_run[Ch].P_plus_EN!=1)&&(PTO_pagram_run[Ch].P_state==0)&&(PTO_pagram_run[Ch].P_cur_FB_NO!=index))
		{
			PTO_pagram_run[Ch].P_plus_EN=1;
			PTO_pagram_run[Ch].P_state=1;
			PTO_pagram_run[Ch].P_cur_FB_NO=index;
			PTO_pagram_run[Ch].P_cur_FB_mode=PTO_PWM;
			PTO_Rst_set(Ch);
			CPLD_IO_Config(PTO_PWM,Ch,1);
			PTO_Mode_Set(Ch,PWM_Mode);
			PWM_Freq_Set(Ch,Freq);
			PTO_pagram_run[Ch].Freq_cur=Freq;
			PWM_Duty_Set(Ch,Duty);
			PTO_pagram_run[Ch].Duty_cur=Duty;
			PTO_Start_set(Ch);
		}
		else if(PTO_pagram_run[Ch].P_cur_FB_NO==index)
		{
			if(PTO_pagram_run[Ch].Freq_cur!=Freq) 
			{
				PWM_Freq_Set(Ch,Freq);
				PTO_pagram_run[Ch].Freq_cur=Freq;
				PWM_Duty_Set(Ch,Duty);
				PTO_pagram_run[Ch].Duty_cur=Duty;
			}
			if(PTO_pagram_run[Ch].Duty_cur!=Duty) 
			{
				PWM_Duty_Set(Ch,Duty);
				PTO_pagram_run[Ch].Duty_cur=Duty;
			}
		}
	}
	else
	{
		if(PTO_pagram_run[Ch].P_cur_FB_NO==index)
		{
			if(PTO_pagram_run[Ch].P_plus_EN==1) 
			{
				PTO_Rst_set(Ch);
				CPLD_IO_Config(PTO_PWM,Ch,0);
				PTO_pagram_run[Ch].P_plus_EN=0;
				PTO_pagram_run[Ch].P_state=0;
				PTO_pagram_run[Ch].P_cur_FB_NO=0;
				PTO_pagram_run[Ch].P_cur_FB_mode=0;
				PTO_pagram_run[Ch].Freq_cur=0;
			}
		}
	}
	return 0;
}

/////////////////myplsy�����������/////////////////////////////////////////
int myplsy(int index, unsigned char Ch, unsigned char En, int Freq, int PulsNum, unsigned char *State, int *Pon, int *Pos)
{
	short Axis=0;
	if(cputype==CPU2001_2411) Axis=NA2411_Axis_MAX;
	else Axis=NA240X_Axis_MAX;
	if(Ch>=Axis) return 1;
	if((En==1)&&(PTO_pagram_run[Ch].P_PStop_flag==0))
	{	
		if((PTO_pagram_run[Ch].P_plus_EN!=1)&&(PTO_pagram_run[Ch].P_state==0)&&(PTO_pagram_run[Ch].P_cur_FB_NO!=index)&&(PTO_pagram_run[Ch].Exception_Code==0))
		{
			PTO_pagram_run[Ch].P_plus_EN=1;
			PTO_pagram_run[Ch].P_state=1;
			* State=0;
			PTO_pagram_run[Ch].P_cur_FB_NO=index;
			PTO_pagram_run[Ch].P_cur_FB_mode=PTO_PLSY;
			PTO_Rst_set(Ch);
			CPLD_IO_Config(PTO_PLSY,Ch,1);
			PTO_Mode_Set(Ch,PTO_Mode);
			PTO_NO_Stop_Set(Ch,0);//��������
			if(Freq>PTO_pagram_set[Ch].Freq_max) Freq=PTO_pagram_set[Ch].Freq_max;
			if(Freq<PTO_pagram_set[Ch].Freq_min) Freq=PTO_pagram_set[Ch].Freq_min;
			PTO_Freq_Set(Ch,Freq);
			PTO_pagram_run[Ch].Freq_cur=Freq;
			
			if(P_ABS_INC_Flag==0)	PTO_pagram_run[Ch].P_INC=0;
			else PTO_pagram_run[Ch].P_INC=1;
			
			if(PTO_pagram_run[Ch].P_INC==0) PulsNum=PulsNum-PTO_pagram_run[Ch].POS_cur;
			if(PulsNum>=0) 
			{
				PTO_Dir_Set(Ch,0);
				PTO_pagram_run[Ch].P_Dir=0; //������
				PTO_pagram_run[Ch].PlusSum_target=PulsNum;
			}
			else
			{
				PTO_Dir_Set(Ch,1);
				PTO_pagram_run[Ch].P_Dir=1; //������
				PTO_pagram_run[Ch].PlusSum_target=0-PulsNum;
			}
			PTO_PlusSum_Set(Ch,PTO_pagram_run[Ch].PlusSum_target);
			PTO_Start_set(Ch);
			PTO_pagram_run[Ch].PON_cur=0;
			PTO_pagram_run[Ch].PON_cur_reg=0;
			*Pos=PTO_pagram_run[Ch].POS_cur;
		}
		else if(PTO_pagram_run[Ch].P_cur_FB_NO==index)
		{
			PTO_pagram_run[Ch].PON_cur=PTO_PlusCurNum_Read(Ch);
			if(PTO_pagram_run[Ch].PON_cur>=PTO_pagram_run[Ch].PlusSum_target) 
			{
				PTO_pagram_run[Ch].P_state=0;
				* State=1;
				PTO_pagram_run[Ch].Freq_cur=0;
			}
			else * State=0;
			*Pon=PTO_pagram_run[Ch].PON_cur;
			if(PTO_pagram_run[Ch].P_Dir==0) 
			{
				PTO_pagram_run[Ch].POS_cur=PTO_pagram_run[Ch].POS_cur+(PTO_pagram_run[Ch].PON_cur-PTO_pagram_run[Ch].PON_cur_reg);
			}
			else 
			{
				PTO_pagram_run[Ch].POS_cur=PTO_pagram_run[Ch].POS_cur-(PTO_pagram_run[Ch].PON_cur-PTO_pagram_run[Ch].PON_cur_reg);
			}
			PTO_pagram_run[Ch].PON_cur_reg=PTO_pagram_run[Ch].PON_cur;
			*Pos=PTO_pagram_run[Ch].POS_cur;
			//if(PTO_pagram_run[Ch].P_state==1) rt_kprintf("PON_cur=%d,POS_cur=%d\n",PTO_pagram_run[Ch].PON_cur,PTO_pagram_run[Ch].POS_cur);
			
			if(PTO_pagram_run[Ch].Exception_Code==1)
			{
				PTO_Rst_set(Ch);
				CPLD_IO_Config(PTO_PLSY,Ch,0);
				PTO_pagram_run[Ch].P_plus_EN=0;
				PTO_pagram_run[Ch].P_state=0;
				* State=0;
				PTO_pagram_run[Ch].P_cur_FB_NO=0;
				PTO_pagram_run[Ch].P_cur_FB_mode=0;
				PTO_pagram_run[Ch].Freq_cur=0;
			}
		}
		else * State=0;
	}
	else if(PTO_pagram_run[Ch].P_cur_FB_NO==index)
	{
		* State=0;
		if(PTO_pagram_run[Ch].P_state==1) 
		{
			PTO_pagram_run[Ch].PON_cur=PTO_PlusCurNum_Read(Ch);
			*Pon=PTO_pagram_run[Ch].PON_cur;
			if(PTO_pagram_run[Ch].P_Dir==0) 
			{
				PTO_pagram_run[Ch].POS_cur=PTO_pagram_run[Ch].POS_cur+(PTO_pagram_run[Ch].PON_cur-PTO_pagram_run[Ch].PON_cur_reg);
			}
			else 
			{
				PTO_pagram_run[Ch].POS_cur=PTO_pagram_run[Ch].POS_cur-(PTO_pagram_run[Ch].PON_cur-PTO_pagram_run[Ch].PON_cur_reg);
			}
			PTO_pagram_run[Ch].PON_cur_reg=PTO_pagram_run[Ch].PON_cur;
			*Pos=PTO_pagram_run[Ch].POS_cur;
		}
		//rt_kprintf("PON_cur=%d,POS_cur=%d\n",PTO_pagram_run[Ch].PON_cur,PTO_pagram_run[Ch].POS_cur);
		if(PTO_pagram_run[Ch].P_plus_EN==1) 
		{
			PTO_Rst_set(Ch);
			CPLD_IO_Config(PTO_PLSY,Ch,0);
			PTO_pagram_run[Ch].P_plus_EN=0;
			PTO_pagram_run[Ch].P_state=0;
			PTO_pagram_run[Ch].P_cur_FB_NO=0;
			PTO_pagram_run[Ch].P_cur_FB_mode=0;
			PTO_pagram_run[Ch].Freq_cur=0;
		}
	}
	else * State=0;
	return 0;
}


/////////////////myplsr�Ӽ����������/////////////////////////////////////////
int myplsr(int index, unsigned char Ch, unsigned char En, int Freq, int PulsNum, int Time, unsigned char *State, int *Pon, int *Pos)
{
	float f_vlaue=0; 
	int itmp=0;
  short Axis=0;
	if(cputype==CPU2001_2411) Axis=NA2411_Axis_MAX;
	else Axis=NA240X_Axis_MAX;
	if(Ch>=Axis) return 1;
	
	if((En==1)&&(PTO_pagram_run[Ch].P_PStop_flag==0))
	{	
		if(Time<=0) return 1;
		if((PTO_pagram_run[Ch].P_plus_EN!=1)&&(PTO_pagram_run[Ch].P_state==0)&&(PTO_pagram_run[Ch].P_cur_FB_NO!=index)&&(PTO_pagram_run[Ch].Exception_Code==0))
		{
			PTO_pagram_run[Ch].P_plus_EN=1;
			PTO_pagram_run[Ch].P_state=1;
			* State=0;
			PTO_pagram_run[Ch].P_cur_FB_NO=index;
			PTO_pagram_run[Ch].P_cur_FB_mode=PTO_PLSR;
			if(P_ABS_INC_Flag==0)	PTO_pagram_run[Ch].P_INC=0;
			else PTO_pagram_run[Ch].P_INC=1;
			PTO_Rst_set(Ch);
			PTO_Mode_Set(Ch,PTO_Mode);
			PTO_NO_Stop_Set(Ch,0);//��������
			CPLD_IO_Config(PTO_PLSR,Ch,1);
			if(Freq>PTO_pagram_set[Ch].Freq_max) Freq=PTO_pagram_set[Ch].Freq_max;
			if(Freq<PTO_pagram_set[Ch].Freq_min) Freq=PTO_pagram_set[Ch].Freq_min;	
			if(Time>PTO_pagram_set[Ch].Time_max) Time=PTO_pagram_set[Ch].Time_max;
			if(Time<PTO_pagram_set[Ch].Time_min) Time=PTO_pagram_set[Ch].Time_min;
			if(PTO_pagram_run[Ch].P_INC==0) PulsNum=PulsNum-PTO_pagram_run[Ch].POS_cur;
			if(PulsNum>=0) 
			{
				PTO_Dir_Set(Ch,0);
				PTO_pagram_run[Ch].P_Dir=0; //������
				PTO_pagram_run[Ch].PlusSum_target=PulsNum;
			}
			else
			{
				PTO_Dir_Set(Ch,1);
				PTO_pagram_run[Ch].P_Dir=1; //������
				PTO_pagram_run[Ch].PlusSum_target=0-PulsNum;
			}
			
			f_vlaue=((float)Time)/1000;
			itmp=(Freq+PTO_pagram_set[Ch].Freq_min)/2*f_vlaue;
			if((itmp*2)>=PTO_pagram_run[Ch].PlusSum_target) PTO_pagram_run[Ch].Plus_AD=PTO_pagram_run[Ch].PlusSum_target/2;
			else PTO_pagram_run[Ch].Plus_AD=PTO_pagram_run[Ch].PlusSum_target-itmp;
			
			PTO_pagram_run[Ch].ADT_cur=(Freq-PTO_pagram_set[Ch].Freq_min)*5/Time;
			PTO_pagram_run[Ch].Freq_cur=PTO_pagram_set[Ch].Freq_min;
			
			PTO_pagram_run[Ch].Freq_target=Freq;
			PTO_Freq_Set(Ch,PTO_pagram_run[Ch].Freq_cur);
			
			PTO_PlusSum_Set(Ch,PTO_pagram_run[Ch].PlusSum_target);
			PTO_Start_set(Ch);
			PTO_pagram_run[Ch].PON_cur=0;
			PTO_pagram_run[Ch].PON_cur_reg=0;	
			PTO_pagram_run[Ch].P_Stop_Req=0;	
			*Pos=PTO_pagram_run[Ch].POS_cur;		
			PTO_pagram_run[Ch].P_Dec_flag=0;			
			if(PTO_pagram_run[Ch].P_state==1) rt_kprintf("PON_cur=%d,POS_cur=%d\n",PTO_pagram_run[Ch].PON_cur,PTO_pagram_run[Ch].POS_cur);
		}
		else if(PTO_pagram_run[Ch].P_cur_FB_NO==index)
		{
			PTO_pagram_run[Ch].PON_cur=PTO_PlusCurNum_Read(Ch);
			if(PTO_pagram_run[Ch].PON_cur>=PTO_pagram_run[Ch].PlusSum_target) 
			{
				PTO_pagram_run[Ch].P_state=0;
				* State=1;
				PTO_pagram_run[Ch].Freq_cur=0;
			}
			else * State=0;
			*Pon=PTO_pagram_run[Ch].PON_cur;
			if(PTO_pagram_run[Ch].P_Dir==0) 
			{
				PTO_pagram_run[Ch].POS_cur=PTO_pagram_run[Ch].POS_cur+(PTO_pagram_run[Ch].PON_cur-PTO_pagram_run[Ch].PON_cur_reg);
			}
			else 
			{
				PTO_pagram_run[Ch].POS_cur=PTO_pagram_run[Ch].POS_cur-(PTO_pagram_run[Ch].PON_cur-PTO_pagram_run[Ch].PON_cur_reg);
			}
			PTO_pagram_run[Ch].PON_cur_reg=PTO_pagram_run[Ch].PON_cur;
			*Pos=PTO_pagram_run[Ch].POS_cur;
			//if(PTO_pagram_run[Ch].P_state==1) rt_kprintf("PON_cur=%d,POS_cur=%d\n",PTO_pagram_run[Ch].PON_cur,PTO_pagram_run[Ch].POS_cur);
			
			if(PTO_pagram_run[Ch].Exception_Code==1)
			{
				* State=0;
				PTO_pagram_run[Ch].P_Stop_Req=1;
				if(PTO_pagram_set[Ch].P_Stop_type==1) 
				{
					if(PTO_pagram_run[Ch].Freq_cur==PTO_pagram_set[Ch].Freq_min)
					{
						PTO_Rst_set(Ch);
						CPLD_IO_Config(PTO_PLSR,Ch,0);
						PTO_pagram_run[Ch].P_plus_EN=0;
						PTO_pagram_run[Ch].P_state=0;
						PTO_pagram_run[Ch].P_Stop_Req=0;
						PTO_pagram_run[Ch].P_cur_FB_NO=0;
						PTO_pagram_run[Ch].P_cur_FB_mode=0;
						PTO_pagram_run[Ch].Freq_cur=0;
					}
				}
				else
				{
					PTO_Rst_set(Ch);
					CPLD_IO_Config(PTO_PLSY,Ch,0);
					PTO_pagram_run[Ch].P_plus_EN=0;
					PTO_pagram_run[Ch].P_state=0;
					PTO_pagram_run[Ch].P_Stop_Req=0;
					PTO_pagram_run[Ch].P_cur_FB_NO=0;
					PTO_pagram_run[Ch].P_cur_FB_mode=0;
					PTO_pagram_run[Ch].Freq_cur=0;
				}
			}
		}
		else * State=0;
	}
	else if(PTO_pagram_run[Ch].P_cur_FB_NO==index)
	{
		* State=0;
		if(PTO_pagram_run[Ch].P_state==1) 
		{
			PTO_pagram_run[Ch].P_Stop_Req=1;
			PTO_pagram_run[Ch].PON_cur=PTO_PlusCurNum_Read(Ch);
			*Pon=PTO_pagram_run[Ch].PON_cur;
			if(PTO_pagram_run[Ch].P_Dir==0)  
			{
				PTO_pagram_run[Ch].POS_cur=PTO_pagram_run[Ch].POS_cur+(PTO_pagram_run[Ch].PON_cur-PTO_pagram_run[Ch].PON_cur_reg);
			}
			else 
			{
				PTO_pagram_run[Ch].POS_cur=PTO_pagram_run[Ch].POS_cur-(PTO_pagram_run[Ch].PON_cur-PTO_pagram_run[Ch].PON_cur_reg);
			}
			PTO_pagram_run[Ch].PON_cur_reg=PTO_pagram_run[Ch].PON_cur;
			*Pos=PTO_pagram_run[Ch].POS_cur;
		}
		//rt_kprintf("PON_cur=%d,POS_cur=%d\n",PTO_pagram_run[Ch].PON_cur,PTO_pagram_run[Ch].POS_cur);
		
		if((PTO_pagram_run[Ch].P_plus_EN==1)&&(PTO_pagram_run[Ch].P_Stop_Req==1))
		{
			if(PTO_pagram_run[Ch].Freq_cur==PTO_pagram_set[Ch].Freq_min)
			{
				PTO_Rst_set(Ch);
				CPLD_IO_Config(PTO_PLSR,Ch,0);
				PTO_pagram_run[Ch].P_plus_EN=0;
				PTO_pagram_run[Ch].P_state=0;
				PTO_pagram_run[Ch].P_Stop_Req=0;
				PTO_pagram_run[Ch].P_cur_FB_NO=0;
				PTO_pagram_run[Ch].P_cur_FB_mode=0;
				PTO_pagram_run[Ch].Freq_cur=0;
			}
		}
		else
		{
			PTO_Rst_set(Ch);
			CPLD_IO_Config(PTO_PLSR,Ch,0);
			PTO_pagram_run[Ch].P_plus_EN=0;
			PTO_pagram_run[Ch].P_state=0;
			PTO_pagram_run[Ch].P_Stop_Req=0;
			PTO_pagram_run[Ch].P_cur_FB_NO=0;
			PTO_pagram_run[Ch].P_cur_FB_mode=0;
			PTO_pagram_run[Ch].Freq_cur=0;
		}
	}
	else * State=0;
	return 0;
}

/////////////////myzrnԭ�������������/////////////////////////////////////////
//en:���ܿ�ʹ�� Ch:PTOͨ���� Freq:����Ƶ�� Time���Ӽ���ʱ�� state�����ܿ�״̬
int myzrn(int index, unsigned char Ch, unsigned char En, int Freq, int Time, unsigned char *State)
{
	int flag=0;
	int itmp=0;
  short Axis=0;
	if(cputype==CPU2001_2411) Axis=NA2411_Axis_MAX;
	else Axis=NA240X_Axis_MAX;
	if(Ch>=Axis) return 1;
	
	if((En==1)&&(PTO_pagram_run[Ch].P_PStop_flag==0))
	{	
		if(Time<=0) return 1;
		if((PTO_pagram_run[Ch].P_plus_EN!=1)&&(PTO_pagram_run[Ch].P_state==0)&&(PTO_pagram_run[Ch].P_cur_FB_NO!=index)&&(PTO_pagram_run[Ch].Exception_Code==0))
		{
			PTO_pagram_run[Ch].P_plus_EN=1;
			PTO_pagram_run[Ch].P_state=1;
			* State=0;
			PTO_pagram_run[Ch].P_cur_FB_NO=index;
			PTO_pagram_run[Ch].P_cur_FB_mode=PTO_ZRN;
			if(P_ABS_INC_Flag==0)	PTO_pagram_run[Ch].P_INC=0;
			else PTO_pagram_run[Ch].P_INC=1;
			PTO_Rst_set(Ch);
			PTO_Mode_Set(Ch,PTO_Mode);
			PTO_NO_Stop_Set(Ch,0);
			CPLD_IO_Config(PTO_ZRN,Ch,1);
			if(Freq>PTO_pagram_set[Ch].Freq_max) Freq=PTO_pagram_set[Ch].Freq_max;
			if(Freq<PTO_pagram_set[Ch].Freq_min) Freq=PTO_pagram_set[Ch].Freq_min;	
			if(Time>PTO_pagram_set[Ch].Time_max) Time=PTO_pagram_set[Ch].Time_max;
			if(Time<PTO_pagram_set[Ch].Time_min) Time=PTO_pagram_set[Ch].Time_min;
			
			PTO_pagram_run[Ch].ADT_cur=(Freq-PTO_pagram_set[Ch].Freq_min)*5/Time;
			PTO_pagram_run[Ch].Freq_cur=PTO_pagram_set[Ch].Freq_min;
			
			PTO_pagram_run[Ch].Freq_target=Freq;
			PTO_Freq_Set(Ch,PTO_pagram_run[Ch].Freq_cur);
			
			itmp=DI_16_Read();
			itmp=itmp&0x0fff0;
			if(Ch==0) itmp=itmp>>4;//��1��
			else if(Ch==1) itmp=itmp>>7; //��2��			
			else if(Ch==2) itmp=itmp>>10; //��3��		
			else itmp=itmp>>13; //��4��		
			if((itmp&0x07)==0)  flag=0; //��ǰλ�ò���3�����λ��
			else if((itmp&0x01)!=0) flag=1; //��ǰλ���ڸ�����λ��
			else if((itmp&0x02)!=0) flag=2; //��ǰλ����������λ��
			else flag=3; //��ǰλ����ԭ��λ��
			
			PTO_pagram_run[Ch].P_Stop_Req=0;	
			
			if(flag==0)
			{	//��ǰλ�ò���3�����λ��
				PTO_Dir_Set(Ch,1); //��������Ѱ
				PTO_pagram_run[Ch].P_Dir=1;
				PTO_pagram_run[Ch].DI_X1_Flag=0;
				PTO_pagram_run[Ch].DI_X2_Flag=0;
				PTO_pagram_run[Ch].DI_X3_Flag=0;
			}
			else if(flag==1)
			{//��ǰλ���ڸ�����λ��
				PTO_Dir_Set(Ch,0); //��������Ѱ
				PTO_pagram_run[Ch].P_Dir=0;
				PTO_pagram_run[Ch].DI_X1_Flag=1;
				PTO_pagram_run[Ch].DI_X2_Flag=0;
				PTO_pagram_run[Ch].DI_X3_Flag=0;
			}
			else if(flag==2)
			{//��ǰλ����������λ��
				PTO_Dir_Set(Ch,1); //��������Ѱ
				PTO_pagram_run[Ch].P_Dir=1;
				PTO_pagram_run[Ch].DI_X1_Flag=0;
				PTO_pagram_run[Ch].DI_X2_Flag=1;
				PTO_pagram_run[Ch].DI_X3_Flag=0;
			}
			else
			{//��ǰλ����ԭ��λ��
				PTO_Dir_Set(Ch,1); //��������Ѱ
				PTO_pagram_run[Ch].P_Dir=1;
				PTO_pagram_run[Ch].DI_X1_Flag=0;
				PTO_pagram_run[Ch].DI_X2_Flag=0;
				PTO_pagram_run[Ch].DI_X3_Flag=1;
				PTO_pagram_run[Ch].Freq_cur=PTO_pagram_set[Ch].Freq_min;
				PTO_pagram_run[Ch].P_Stop_Req=4;
			}
			PTO_PlusSum_Set(Ch,0x7fff0000);
			PTO_Start_set(Ch);
			//if(PTO_pagram_run[Ch].P_state==1) rt_kprintf("PON_cur=%d,POS_cur=%d\n",PTO_pagram_run[Ch].PON_cur,PTO_pagram_run[Ch].POS_cur);
		}
		else if(PTO_pagram_run[Ch].P_cur_FB_NO==index)
		{
			if(PTO_pagram_run[Ch].P_state==0)
			{
				PTO_pagram_run[Ch].PON_cur=0;
				PTO_pagram_run[Ch].PON_cur_reg=0;	
				PTO_pagram_run[Ch].POS_cur=0;
				* State=1;
				PTO_pagram_run[Ch].Freq_cur=0;
			}
			else * State=0;
		}
		else * State=0;
	}
	else if(PTO_pagram_run[Ch].P_cur_FB_NO==index)
	{
		* State=0;
		if((PTO_pagram_run[Ch].P_plus_EN==1)&&(PTO_pagram_run[Ch].P_state==1))
		{
			if(PTO_pagram_run[Ch].P_state==1)
			{
				PTO_pagram_run[Ch].P_Stop_Req=4;
			}
			if(PTO_pagram_run[Ch].Freq_cur==PTO_pagram_set[Ch].Freq_min)
			{
				PTO_pagram_run[Ch].P_plus_EN=0;
				PTO_pagram_run[Ch].P_Stop_Req=0;
				PTO_pagram_run[Ch].P_state=0;		
				PTO_pagram_run[Ch].P_cur_FB_NO=0;
				PTO_pagram_run[Ch].P_cur_FB_mode=0;
				PTO_pagram_run[Ch].Freq_cur=0;
				PTO_Rst_set(Ch);
				CPLD_IO_Config(PTO_ZRN,Ch,0);
			}
		}
		else
		{
			PTO_pagram_run[Ch].P_plus_EN=0;
			PTO_pagram_run[Ch].P_Stop_Req=0;
			PTO_pagram_run[Ch].P_state=0;		
			PTO_pagram_run[Ch].P_cur_FB_NO=0;
			PTO_pagram_run[Ch].P_cur_FB_mode=0;
			PTO_pagram_run[Ch].Freq_cur=0;
			PTO_Rst_set(Ch);
			CPLD_IO_Config(PTO_ZRN,Ch,0);
		}
	}
	else * State=0;
	return 0;
}

/////////////////myplsv�ɱ�Ƶ���������/////////////////////////////////////////
//en:���ܿ�ʹ�� Ch:PTOͨ���� Freq:����Ƶ�� Time���Ӽ���ʱ�� state�����ܿ�״̬
int myplsf(int index, unsigned char Ch, unsigned char En, int Freq, int Time)
{
  short Axis=0;
	if(cputype==CPU2001_2411) Axis=NA2411_Axis_MAX;
	else Axis=NA240X_Axis_MAX;
	if(Ch>=Axis) return 1;
	
			
	if((En==1)&&(PTO_pagram_run[Ch].P_PStop_flag==0))
	{	
		if(Time<0) return 1;
		if((PTO_pagram_run[Ch].P_plus_EN!=1)&&(PTO_pagram_run[Ch].P_state==0)&&(PTO_pagram_run[Ch].P_cur_FB_NO!=index)&&(PTO_pagram_run[Ch].Exception_Code==0))
		{
			PTO_pagram_run[Ch].P_plus_EN=1;
			PTO_pagram_run[Ch].P_state=1;
			PTO_pagram_run[Ch].P_cur_FB_NO=index;
			PTO_pagram_run[Ch].P_cur_FB_mode=PTO_PLSV;
			if(P_ABS_INC_Flag==0)	PTO_pagram_run[Ch].P_INC=0;
			else PTO_pagram_run[Ch].P_INC=1;
			PTO_Rst_set(Ch);
			PTO_Mode_Set(Ch,PTO_Mode);
			PTO_NO_Stop_Set(Ch,1); //��������
			CPLD_IO_Config(PTO_PLSV,Ch,1);//ʹ�ܹ��ܿ����DO����
			if(Freq>=0) 
			{
				PTO_Dir_Set(Ch,0);
				PTO_pagram_run[Ch].P_Dir=0; //������
			}
			else
			{
				PTO_Dir_Set(Ch,1);
				PTO_pagram_run[Ch].P_Dir=1; //������
				Freq=0-Freq;
			}
			if(Freq>PTO_pagram_set[Ch].Freq_max) Freq=PTO_pagram_set[Ch].Freq_max;
			if(Freq<PTO_pagram_set[Ch].Freq_min) Freq=PTO_pagram_set[Ch].Freq_min;	
			if(Time>PTO_pagram_set[Ch].Time_max) Time=PTO_pagram_set[Ch].Time_max;
			if(Time==0) 
			{
				PTO_pagram_run[Ch].ADT_cur=0;
				PTO_pagram_run[Ch].Freq_cur=Freq;
			}
			else
			{
				PTO_pagram_run[Ch].ADT_cur=(Freq-PTO_pagram_set[Ch].Freq_min)*5/Time;
				PTO_pagram_run[Ch].Freq_cur=PTO_pagram_set[Ch].Freq_min;
			}
			
			PTO_pagram_run[Ch].Freq_target=Freq;
			PTO_Freq_Set(Ch,PTO_pagram_run[Ch].Freq_cur);
			PTO_PlusSum_Set(Ch,0x7fff0000);
			PTO_Start_set(Ch);
			PTO_pagram_run[Ch].P_Stop_Req=0;	
		}
		else if(PTO_pagram_run[Ch].P_cur_FB_NO==index)
		{
			if(Freq<0) Freq=0-Freq;
			if(PTO_pagram_run[Ch].Freq_target!=Freq)
			{
				if(Time>PTO_pagram_set[Ch].Time_max) Time=PTO_pagram_set[Ch].Time_max;
				if(Time>0) 
				{
					if(PTO_pagram_run[Ch].Freq_cur<Freq) PTO_pagram_run[Ch].ADT_cur=(Freq-PTO_pagram_run[Ch].Freq_cur)*5/Time;
					else PTO_pagram_run[Ch].ADT_cur=(PTO_pagram_run[Ch].Freq_cur-Freq)*5/Time;
				}
				else
				{
					PTO_pagram_run[Ch].ADT_cur=0;
					PTO_pagram_run[Ch].Freq_cur=Freq;
				}
				PTO_pagram_run[Ch].Freq_target=Freq;
				PTO_Freq_Set(Ch,PTO_pagram_run[Ch].Freq_cur);
			}
			PTO_pagram_run[Ch].PON_cur=PTO_PlusCurNum_Read(Ch);
			if(PTO_pagram_run[Ch].P_Dir==0)  
			{
				PTO_pagram_run[Ch].POS_cur=PTO_pagram_run[Ch].POS_cur+(PTO_pagram_run[Ch].PON_cur-PTO_pagram_run[Ch].PON_cur_reg);
			}
			else 
			{
				PTO_pagram_run[Ch].POS_cur=PTO_pagram_run[Ch].POS_cur-(PTO_pagram_run[Ch].PON_cur-PTO_pagram_run[Ch].PON_cur_reg);
			}
			PTO_pagram_run[Ch].PON_cur_reg=PTO_pagram_run[Ch].PON_cur;
		}
	}
	else if(PTO_pagram_run[Ch].P_cur_FB_NO==index)
	{
		PTO_pagram_run[Ch].PON_cur=PTO_PlusCurNum_Read(Ch);
		if(PTO_pagram_run[Ch].P_Dir==0)  
		{
			PTO_pagram_run[Ch].POS_cur=PTO_pagram_run[Ch].POS_cur+(PTO_pagram_run[Ch].PON_cur-PTO_pagram_run[Ch].PON_cur_reg);
		}
		else 
		{
			PTO_pagram_run[Ch].POS_cur=PTO_pagram_run[Ch].POS_cur-(PTO_pagram_run[Ch].PON_cur-PTO_pagram_run[Ch].PON_cur_reg);
		}
		PTO_pagram_run[Ch].PON_cur_reg=PTO_pagram_run[Ch].PON_cur;
		if((PTO_pagram_run[Ch].P_plus_EN==1)&&(Time>0))
		{
			PTO_pagram_run[Ch].P_Stop_Req=1;
			if(PTO_pagram_run[Ch].Freq_cur==PTO_pagram_set[Ch].Freq_min)
			{
				PTO_pagram_run[Ch].P_plus_EN=0;
				PTO_pagram_run[Ch].P_Stop_Req=0;
				PTO_pagram_run[Ch].P_state=0;		
				PTO_pagram_run[Ch].P_cur_FB_NO=0;
				PTO_pagram_run[Ch].P_cur_FB_mode=0;
				PTO_pagram_run[Ch].Freq_cur=0;
				PTO_Rst_set(Ch);
				CPLD_IO_Config(PTO_PLSV,Ch,0);
				PTO_NO_Stop_Set(Ch,0); //��������
			}
		}
		else
		{
			PTO_pagram_run[Ch].P_plus_EN=0;
			PTO_pagram_run[Ch].P_Stop_Req=0;
			PTO_pagram_run[Ch].P_state=0;		
			PTO_pagram_run[Ch].P_cur_FB_NO=0;
			PTO_pagram_run[Ch].P_cur_FB_mode=0;
			PTO_pagram_run[Ch].Freq_cur=0;
			PTO_Rst_set(Ch);
			CPLD_IO_Config(PTO_PLSV,Ch,0);
			PTO_NO_Stop_Set(Ch,0); //��������
		}
	}
	return 0;
}
/////////////////mympto����������/////////////////////////////////////////
//en:���ܿ�ʹ�� Ch:PTOͨ���� TBL:��β����� N:�ܶ��� Time���Ӽ���ʱ�� state�����ܿ�״̬
int mypto(int index, unsigned char Ch, unsigned char En, int * TBL, int N, int Time, unsigned char *State, int *Pon, int *Pos)
{
	int i=0;
	int Plus_sum=0;
	int dir=0;
	float f_vlaue=0; 
	int itmp=0;
	int itmp1=0;
	short Axis=0;
	if(cputype==CPU2001_2411) Axis=NA2411_Axis_MAX;
	else Axis=NA240X_Axis_MAX;
	if(Ch>=Axis) return 1;
	
	if((En==1)&&(PTO_pagram_run[Ch].P_PStop_flag==0))
	{	
		if((PTO_pagram_run[Ch].P_plus_EN!=1)&&(PTO_pagram_run[Ch].P_state==0)&&(PTO_pagram_run[Ch].P_cur_FB_NO!=index)&&(PTO_pagram_run[Ch].Exception_Code==0))
		{
			if((Time<0)||(N<=1)) return 1;
			for(i=0;i<N;i++) if(TBL[2*i]<=0) return 1;
			if(P_ABS_INC_Flag==0)	PTO_pagram_run[Ch].P_INC=0;
			else PTO_pagram_run[Ch].P_INC=1;
			if(PTO_pagram_run[Ch].P_INC==0)
			{
				if(TBL[1]<TBL[3]) dir=1; //����
				else dir=0; //�ݼ�
				for(i=0;i<(N-1);i++)
				{
					if(dir==1){if(TBL[2*i+1]>=TBL[2*i+3]) return 1;}
					else {if(TBL[2*i+1]<=TBL[2*i+3]) return 1;}
				}
			}
			else
			{
				if(TBL[1]>0) dir=0;//������
				else dir=1;//������
				for(i=0;i<N;i++)
				{
					if(dir==0){if(TBL[2*i+1]<=0) return 1;}
					else {if(TBL[2*i+1]>=0) return 1;}
				}
			}
	
			PTO_pagram_run[Ch].P_plus_EN=1;
			PTO_pagram_run[Ch].P_state=1;
			* State=0;
			PTO_pagram_run[Ch].P_cur_FB_NO=index;
			PTO_pagram_run[Ch].P_cur_FB_mode=PTO_MPTO;
			PTO_Rst_set(Ch);
			PTO_Mode_Set(Ch,PTO_Mode);
			PTO_NO_Stop_Set(Ch,0); //��������
			CPLD_IO_Config(PTO_MPTO,Ch,1);//ʹ�ܹ��ܿ����DO����			
			if(Time>PTO_pagram_set[Ch].Time_max) Time=PTO_pagram_set[Ch].Time_max;
			if(Time==0) 
			{
				PTO_pagram_run[Ch].ADT_cur=0;
				PTO_pagram_run[Ch].Freq_cur=TBL[0];
			}
			else 
			{
				PTO_pagram_run[Ch].ADT_cur=(TBL[0]-PTO_pagram_set[Ch].Freq_min)*5/Time;
				PTO_pagram_run[Ch].Freq_cur=PTO_pagram_set[Ch].Freq_min;
			}
			PTO_pagram_run[Ch].P_MPTO_Time=Time;
			PTO_pagram_run[Ch].Freq_target=TBL[0];
			PTO_Freq_Set(Ch,PTO_pagram_run[Ch].Freq_cur);
			
			if(PTO_pagram_run[Ch].P_INC==0) Plus_sum=TBL[2*N-1]-PTO_pagram_run[Ch].POS_cur;
			else for(i=0;i<N;i++) Plus_sum=Plus_sum+TBL[2*i+1];
			if(Plus_sum>=0) 
			{
				PTO_Dir_Set(Ch,0);
				PTO_pagram_run[Ch].P_Dir=0; //������
				PTO_pagram_run[Ch].PlusSum_target=Plus_sum;
			}
			else
			{
				PTO_Dir_Set(Ch,1);
				PTO_pagram_run[Ch].P_Dir=1; //������
				PTO_pagram_run[Ch].PlusSum_target=0-Plus_sum;
			}
			
			f_vlaue=((float)Time)/1000;
			itmp=(TBL[2*N-2]+PTO_pagram_set[Ch].Freq_min)/2*f_vlaue;
			
			if(PTO_pagram_run[Ch].P_INC==0) 
			{
				itmp1=TBL[1]-PTO_pagram_run[Ch].POS_cur;
				if(itmp1>0) PTO_pagram_run[Ch].P_MPTO_PlusSum=itmp1;
				else PTO_pagram_run[Ch].P_MPTO_PlusSum=0-itmp1;
				itmp1=TBL[2*N-1]-TBL[2*N-3];
				if(itmp1<0)  itmp1=0-itmp1;				
				if(itmp<itmp1) PTO_pagram_run[Ch].Plus_AD=PTO_pagram_run[Ch].PlusSum_target-itmp;
				else PTO_pagram_run[Ch].Plus_AD=PTO_pagram_run[Ch].PlusSum_target-itmp1;
			}
			else 
			{
				if(TBL[1]>0) PTO_pagram_run[Ch].P_MPTO_PlusSum=TBL[1];
				else PTO_pagram_run[Ch].P_MPTO_PlusSum=0-TBL[1];
				itmp1=TBL[2*N-1];
				if(itmp1<0)  itmp1=0-itmp1;		
				if(itmp<itmp1) PTO_pagram_run[Ch].Plus_AD=PTO_pagram_run[Ch].PlusSum_target-itmp;
				else PTO_pagram_run[Ch].Plus_AD=PTO_pagram_run[Ch].PlusSum_target-itmp1;
			}
			
			PTO_PlusSum_Set(Ch,PTO_pagram_run[Ch].PlusSum_target);
			PTO_Start_set(Ch);
			PTO_pagram_run[Ch].P_Stop_Req=0;	
			PTO_pagram_run[Ch].PON_cur=0;
			PTO_pagram_run[Ch].PON_cur_reg=0;	
			PTO_pagram_run[Ch].P_MPTO_index=0;
			PTO_pagram_run[Ch].P_MPTO_N=N;
			PTO_pagram_run[Ch].P_MPTO_TBL=TBL;
			PTO_pagram_run[Ch].P_Dec_flag=0;
			*Pos=PTO_pagram_run[Ch].POS_cur;
		}
		else if(PTO_pagram_run[Ch].P_cur_FB_NO==index)
		{
			PTO_pagram_run[Ch].PON_cur=PTO_PlusCurNum_Read(Ch);
			if(PTO_pagram_run[Ch].PON_cur>=PTO_pagram_run[Ch].PlusSum_target) 
			{
				PTO_pagram_run[Ch].P_state=0;
				* State=1;
				PTO_pagram_run[Ch].Freq_cur=0;
			}
			else * State=0;
			*Pon=PTO_pagram_run[Ch].PON_cur;
			if(PTO_pagram_run[Ch].P_Dir==0) 
			{
				PTO_pagram_run[Ch].POS_cur=PTO_pagram_run[Ch].POS_cur+(PTO_pagram_run[Ch].PON_cur-PTO_pagram_run[Ch].PON_cur_reg);
			}
			else 
			{
				PTO_pagram_run[Ch].POS_cur=PTO_pagram_run[Ch].POS_cur-(PTO_pagram_run[Ch].PON_cur-PTO_pagram_run[Ch].PON_cur_reg);
			}
			PTO_pagram_run[Ch].PON_cur_reg=PTO_pagram_run[Ch].PON_cur;
			*Pos=PTO_pagram_run[Ch].POS_cur;
			//if(PTO_pagram_run[Ch].P_state==1) rt_kprintf("PON_cur=%d,POS_cur=%d\n",PTO_pagram_run[Ch].PON_cur,PTO_pagram_run[Ch].POS_cur);
			
			if(PTO_pagram_run[Ch].Exception_Code==1)
			{
				* State=0;
				PTO_pagram_run[Ch].P_Stop_Req=1;
				if((PTO_pagram_set[Ch].P_Stop_type==1) && (Time>0))
				{
					if(PTO_pagram_run[Ch].Freq_cur==PTO_pagram_set[Ch].Freq_min)
					{
						PTO_Rst_set(Ch);
						CPLD_IO_Config(PTO_MPTO,Ch,0);
						PTO_pagram_run[Ch].P_plus_EN=0;
						PTO_pagram_run[Ch].P_state=0;
						PTO_pagram_run[Ch].P_Stop_Req=0;
						PTO_pagram_run[Ch].P_cur_FB_NO=0;
						PTO_pagram_run[Ch].P_cur_FB_mode=0;
						PTO_pagram_run[Ch].Freq_cur=0;
					}
				}
				else
				{
					PTO_Rst_set(Ch);
					CPLD_IO_Config(PTO_MPTO,Ch,0);
					PTO_pagram_run[Ch].P_plus_EN=0;
					PTO_pagram_run[Ch].P_state=0;
					PTO_pagram_run[Ch].P_Stop_Req=0;
					PTO_pagram_run[Ch].P_cur_FB_NO=0;
					PTO_pagram_run[Ch].P_cur_FB_mode=0;
					PTO_pagram_run[Ch].Freq_cur=0;
				}
			}
		}
		else * State=0;
	}
	else if(PTO_pagram_run[Ch].P_cur_FB_NO==index)
	{
		* State=0;
		if(PTO_pagram_run[Ch].P_state==1) 
		{
			PTO_pagram_run[Ch].P_Stop_Req=1;
			PTO_pagram_run[Ch].PON_cur=PTO_PlusCurNum_Read(Ch);
			*Pon=PTO_pagram_run[Ch].PON_cur;
			if(PTO_pagram_run[Ch].P_Dir==0) 
			{
				PTO_pagram_run[Ch].POS_cur=PTO_pagram_run[Ch].POS_cur+(PTO_pagram_run[Ch].PON_cur-PTO_pagram_run[Ch].PON_cur_reg);
			}
			else 
			{
				PTO_pagram_run[Ch].POS_cur=PTO_pagram_run[Ch].POS_cur-(PTO_pagram_run[Ch].PON_cur-PTO_pagram_run[Ch].PON_cur_reg);
			}
			PTO_pagram_run[Ch].PON_cur_reg=PTO_pagram_run[Ch].PON_cur;
			*Pos=PTO_pagram_run[Ch].POS_cur;
		}
		
		
		if((PTO_pagram_run[Ch].P_plus_EN==1)&&(PTO_pagram_run[Ch].P_Stop_Req==1))
		{
			if(PTO_pagram_run[Ch].Freq_cur==PTO_pagram_set[Ch].Freq_min)
			{
				PTO_Rst_set(Ch);
				CPLD_IO_Config(PTO_MPTO,Ch,0);
				PTO_pagram_run[Ch].P_plus_EN=0;
				PTO_pagram_run[Ch].P_state=0;
				PTO_pagram_run[Ch].P_Stop_Req=0;
				PTO_pagram_run[Ch].P_cur_FB_NO=0;
				PTO_pagram_run[Ch].P_cur_FB_mode=0;
				PTO_pagram_run[Ch].Freq_cur=0;
			}
		}
		else
		{
			PTO_Rst_set(Ch);
			CPLD_IO_Config(PTO_MPTO,Ch,0);
			PTO_pagram_run[Ch].P_plus_EN=0;
			PTO_pagram_run[Ch].P_state=0;
			PTO_pagram_run[Ch].P_Stop_Req=0;
			PTO_pagram_run[Ch].P_cur_FB_NO=0;
			PTO_pagram_run[Ch].P_cur_FB_mode=0;
			PTO_pagram_run[Ch].Freq_cur=0;
		}
	}
	else * State=0;
	return 0;
}


/***
*mypline-�����������
*@PulsNum0/1(for 8ms):����Ϊ����������ֵ������Ϊ����������ֵ
*@Pon0/1:��ָ���������ص�ǰ�������ѷ���
*@Pos0/1:��ָ���������ص�ǰ����������λ��
*@State   :��ָ���������ظ�ָ��ִ��״̬
		  0-�ö��Ѿ�ִ�е�δ���;
		  1-�ö��Ѿ����;
		  2-������ʹĳ�ᴥ�������޶�OR ĳ���Ѿ����������޶�;
		  3-�ᱻռ�ã��������޷�ִ��;
		  4-��������ִ�й����б�Enʹ��OR ��mystop������ֹ;
		  5-����
*/
int32_t mypline(uint32_t index, uint8_t Ch0,  uint8_t Ch1,uint8_t En,uint8_t Inc, int32_t PulsNum0, int32_t PulsNum1,
				uint8_t *State, int32_t *Pon0, int32_t *Pon1, int32_t *Pos0, int32_t *Pos1)
{
	uint8_t Axis=0;
	uint32_t Freq=0;
	if(cputype==CPU2001_2411) Axis=NA2411_Axis_MAX;
	else Axis=NA240X_Axis_MAX;
	if(Ch0>=Axis||Ch1>=Axis) return 1;
	if((En==1)&&(PTO_pagram_run[Ch0].P_PStop_flag==0)&&(PTO_pagram_run[Ch1].P_PStop_flag==0))
	{	
		if((PTO_pagram_run[Ch0].P_plus_EN!=1)&&(PTO_pagram_run[Ch1].P_plus_EN!=1)&&
			(PTO_pagram_run[Ch0].P_state==0)&&(PTO_pagram_run[Ch1].P_state==0)&&
			(PTO_pagram_run[Ch0].P_cur_FB_NO!=index)&&(PTO_pagram_run[Ch1].P_cur_FB_NO!=index)&&
			(PTO_pagram_run[Ch0].Exception_Code==0)&&(PTO_pagram_run[Ch1].Exception_Code==0))
		{
			//printf("*1*");
			* State=0;
			gbiax=biax_check(Ch0,Ch1);
			switch(gbiax)
			{
				case Q1Q2 :
					PTO1_INT_Flag=0;
					PTO2_INT_Flag=0;
					break;
				case Q1Q3 :
					PTO1_INT_Flag=0;
					PTO3_INT_Flag=0;
					break;
				case Q1Q4 :
					PTO1_INT_Flag=0;
					PTO4_INT_Flag=0;
					break;
				case Q2Q3 :
					PTO2_INT_Flag=0;
					PTO3_INT_Flag=0;
					break;
				case Q2Q4 :
					PTO2_INT_Flag=0;
					PTO4_INT_Flag=0;
					break;
				case Q3Q4 :
					PTO3_INT_Flag=0;
					PTO4_INT_Flag=0;
					break;
				default:
					break;
			}
			
			PTO_pagram_run[Ch0].P_plus_EN=1;
			PTO_pagram_run[Ch0].P_state=1;
			PTO_pagram_run[Ch0].P_cur_FB_NO=index;
			PTO_pagram_run[Ch0].P_cur_FB_mode=PTO_PLIN;
			
			PTO_pagram_run[Ch1].P_plus_EN=1;
			PTO_pagram_run[Ch1].P_state=1;
			PTO_pagram_run[Ch1].P_cur_FB_NO=index;
			PTO_pagram_run[Ch1].P_cur_FB_mode=PTO_PLIN;
			
			PTO_Rst_set(Ch0);
			CPLD_IO_Config(PTO_PLIN,Ch0,1);
			PTO_Mode_Set(Ch0,PTO_Mode);
			PTO_NO_Stop_Set(Ch0,0);//��������
			
			PTO_Rst_set(Ch1);
			CPLD_IO_Config(PTO_PLIN,Ch1,1);
			PTO_Mode_Set(Ch1,PTO_Mode);
			PTO_NO_Stop_Set(Ch1,0);//��������
			
			Freq=abs(PulsNum0*1000/8);		
			PTO_Freq_Set(Ch0,Freq);
			PTO_pagram_run[Ch0].Freq_cur=Freq;
			
			Freq=abs(PulsNum1*1000/8);		
			PTO_Freq_Set(Ch1,Freq);			
			PTO_pagram_run[Ch1].Freq_cur=Freq;
			if(Inc==0)
			{	
				PTO_pagram_run[Ch0].P_INC=0;
				PulsNum0=PulsNum0-PTO_pagram_run[Ch0].POS_cur;
				PTO_pagram_run[Ch1].P_INC=0;
				PulsNum1=PulsNum1-PTO_pagram_run[Ch1].POS_cur;
			}
			else 
			{
				PTO_pagram_run[Ch0].P_INC=1;
				PTO_pagram_run[Ch1].P_INC=1;
			}
			if(PulsNum0>=0) 
			{
				PTO_Dir_Set(Ch0,0);
				PTO_pagram_run[Ch0].P_Dir=0; //������
				PTO_pagram_run[Ch0].PlusSum_target=PulsNum0;
			}
			else
			{
				PTO_Dir_Set(Ch0,1);
				PTO_pagram_run[Ch0].P_Dir=1; //������
				PTO_pagram_run[Ch0].PlusSum_target=0-PulsNum0;
			}
			if(PulsNum1>=0) 
			{
				PTO_Dir_Set(Ch1,0);
				PTO_pagram_run[Ch1].P_Dir=0; //������
				PTO_pagram_run[Ch1].PlusSum_target=PulsNum1;
			}
			else
			{
				PTO_Dir_Set(Ch1,1);
				PTO_pagram_run[Ch1].P_Dir=1; //������
				PTO_pagram_run[Ch1].PlusSum_target=0-PulsNum1;
			}
			PTO_PlusSum_Set(Ch0,PTO_pagram_run[Ch0].PlusSum_target);
			PTO_PlusSum_Set(Ch1,PTO_pagram_run[Ch1].PlusSum_target);
			PTO_Start_set(Ch0);
			PTO_Start_set(Ch1);
			PTO_pagram_run[Ch0].PON_cur=0;
			PTO_pagram_run[Ch0].PON_cur_reg=0;
			*Pos0=PTO_pagram_run[Ch0].POS_cur;
			PTO_pagram_run[Ch1].PON_cur=0;
			PTO_pagram_run[Ch1].PON_cur_reg=0;
			*Pos1=PTO_pagram_run[Ch1].POS_cur;
		}
		else if((PTO_pagram_run[Ch0].P_cur_FB_NO==index)&&(PTO_pagram_run[Ch1].P_cur_FB_NO==index))
		{
			//printf("*2*");
			PTO_pagram_run[Ch0].PON_cur=PTO_PlusCurNum_Read(Ch0);
			PTO_pagram_run[Ch1].PON_cur=PTO_PlusCurNum_Read(Ch1);
			if((PTO_pagram_run[Ch0].PON_cur>=PTO_pagram_run[Ch0].PlusSum_target)&&
				(PTO_pagram_run[Ch1].PON_cur>=PTO_pagram_run[Ch1].PlusSum_target)) 
			{
				PTO_pagram_run[Ch0].P_state=0;
				PTO_pagram_run[Ch1].P_state=0;
				//printf("*over*");
				* State=1;
				PTO_pagram_run[Ch0].Freq_cur=0;
				PTO_pagram_run[Ch1].Freq_cur=0;
			}
			else * State=0;
			*Pon0=PTO_pagram_run[Ch0].PON_cur;
			*Pon1=PTO_pagram_run[Ch1].PON_cur;
			if(PTO_pagram_run[Ch0].P_Dir==0) 
			{
				PTO_pagram_run[Ch0].POS_cur=PTO_pagram_run[Ch0].POS_cur+(PTO_pagram_run[Ch0].PON_cur-PTO_pagram_run[Ch0].PON_cur_reg);
			}
			else 
			{
				PTO_pagram_run[Ch0].POS_cur=PTO_pagram_run[Ch0].POS_cur-(PTO_pagram_run[Ch0].PON_cur-PTO_pagram_run[Ch0].PON_cur_reg);
			}
			PTO_pagram_run[Ch0].PON_cur_reg=PTO_pagram_run[Ch0].PON_cur;
			*Pos0=PTO_pagram_run[Ch0].POS_cur;
			
			if(PTO_pagram_run[Ch1].P_Dir==0) 
			{
				PTO_pagram_run[Ch1].POS_cur=PTO_pagram_run[Ch1].POS_cur+(PTO_pagram_run[Ch1].PON_cur-PTO_pagram_run[Ch1].PON_cur_reg);
			}
			else 
			{
				PTO_pagram_run[Ch1].POS_cur=PTO_pagram_run[Ch1].POS_cur-(PTO_pagram_run[Ch1].PON_cur-PTO_pagram_run[Ch1].PON_cur_reg);
			}
			PTO_pagram_run[Ch1].PON_cur_reg=PTO_pagram_run[Ch1].PON_cur;
			*Pos1=PTO_pagram_run[Ch1].POS_cur;			
			//if(PTO_pagram_run[Ch].P_state==1) rt_kprintf("PON_cur=%d,POS_cur=%d\n",PTO_pagram_run[Ch].PON_cur,PTO_pagram_run[Ch].POS_cur);
			
			if((PTO_pagram_run[Ch0].Exception_Code==1)||(PTO_pagram_run[Ch1].Exception_Code==1))
			{
				//printf("*3*");
				PTO_Rst_set(Ch0);
				CPLD_IO_Config(PTO_PLIN,Ch0,0);
				PTO_pagram_run[Ch0].P_plus_EN=0;
				PTO_pagram_run[Ch0].P_state=0;
				PTO_pagram_run[Ch0].P_cur_FB_NO=0;
				PTO_pagram_run[Ch0].P_cur_FB_mode=0;
				PTO_pagram_run[Ch0].Freq_cur=0;
				
				PTO_Rst_set(Ch1);
				CPLD_IO_Config(PTO_PLIN,Ch1,0);
				PTO_pagram_run[Ch1].P_plus_EN=0;
				PTO_pagram_run[Ch1].P_state=0;
				PTO_pagram_run[Ch1].P_cur_FB_NO=0;
				PTO_pagram_run[Ch1].P_cur_FB_mode=0;
				PTO_pagram_run[Ch1].Freq_cur=0;
				* State=2;//������ʹĳ�ᴥ�������޶�
			}
		}
		else if((PTO_pagram_run[Ch0].Exception_Code==1)||(PTO_pagram_run[Ch1].Exception_Code==1))
		{
			* State=2;//ĳ���Ѿ����������޶�
			//printf("*4*");
		}
		else if((PTO_pagram_run[Ch0].P_state==1)||(PTO_pagram_run[Ch1].P_state==1))
		{
			* State=3;//�ᱻռ�ã��������޷�ִ�С�
			//printf("*5*");
		}
	}
	else if((PTO_pagram_run[Ch0].P_cur_FB_NO==index)&&(PTO_pagram_run[Ch1].P_cur_FB_NO==index))
	{
		//printf("*6*");
		* State=4;//��������ִ�й����б�ʹ��En ��mystop������ֹ;	
		if((PTO_pagram_run[Ch0].P_state==1)||(PTO_pagram_run[Ch1].P_state==1)) 
		{
			PTO_pagram_run[Ch0].PON_cur=PTO_PlusCurNum_Read(Ch0);
			*Pon0=PTO_pagram_run[Ch0].PON_cur;
			if(PTO_pagram_run[Ch0].P_Dir==0) 
			{
				PTO_pagram_run[Ch0].POS_cur=PTO_pagram_run[Ch0].POS_cur+(PTO_pagram_run[Ch0].PON_cur-PTO_pagram_run[Ch0].PON_cur_reg);
			}
			else 
			{
				PTO_pagram_run[Ch0].POS_cur=PTO_pagram_run[Ch0].POS_cur-(PTO_pagram_run[Ch0].PON_cur-PTO_pagram_run[Ch0].PON_cur_reg);
			}
			PTO_pagram_run[Ch0].PON_cur_reg=PTO_pagram_run[Ch0].PON_cur;
			*Pos0=PTO_pagram_run[Ch0].POS_cur;
			
			PTO_pagram_run[Ch1].PON_cur=PTO_PlusCurNum_Read(Ch1);
			*Pon1=PTO_pagram_run[Ch1].PON_cur;
			if(PTO_pagram_run[Ch1].P_Dir==0) 
			{
				PTO_pagram_run[Ch1].POS_cur=PTO_pagram_run[Ch1].POS_cur+(PTO_pagram_run[Ch1].PON_cur-PTO_pagram_run[Ch1].PON_cur_reg);
			}
			else 
			{
				PTO_pagram_run[Ch1].POS_cur=PTO_pagram_run[Ch1].POS_cur-(PTO_pagram_run[Ch1].PON_cur-PTO_pagram_run[Ch1].PON_cur_reg);
			}
			PTO_pagram_run[Ch1].PON_cur_reg=PTO_pagram_run[Ch1].PON_cur;
			*Pos1=PTO_pagram_run[Ch1].POS_cur;
			
		}

		//rt_kprintf("PON_cur=%d,POS_cur=%d\n",PTO_pagram_run[Ch].PON_cur,PTO_pagram_run[Ch].POS_cur);
		if((PTO_pagram_run[Ch0].P_plus_EN==1)||(PTO_pagram_run[Ch1].P_plus_EN==1)) 
		{
			PTO_Rst_set(Ch0);
			CPLD_IO_Config(PTO_PLIN,Ch0,0);
			PTO_pagram_run[Ch0].P_plus_EN=0;
			PTO_pagram_run[Ch0].P_state=0;
			PTO_pagram_run[Ch0].P_cur_FB_NO=0;
			PTO_pagram_run[Ch0].P_cur_FB_mode=0;
			PTO_pagram_run[Ch0].Freq_cur=0;

			PTO_Rst_set(Ch1);
			CPLD_IO_Config(PTO_PLIN,Ch1,0);
			PTO_pagram_run[Ch1].P_plus_EN=0;
			PTO_pagram_run[Ch1].P_state=0;
			PTO_pagram_run[Ch1].P_cur_FB_NO=0;
			PTO_pagram_run[Ch1].P_cur_FB_mode=0;
			PTO_pagram_run[Ch1].Freq_cur=0;		
		}
	}
	else * State=5;
	return 0;
}




/////////////////mypstop����ֹͣ���ָ��/////////////////////////////////////////
//en:���ܿ�ʹ�� Ch:PTOͨ���� Time���Ӽ���ʱ�� state�����ܿ�״̬
int mypstop(int index, unsigned char Ch, unsigned char En, int Time, unsigned char *State)
{
  short Axis=0;
	if(cputype==CPU2001_2411) Axis=NA2411_Axis_MAX;
	else Axis=NA240X_Axis_MAX;
	if(Ch>=Axis) return 1;
	
	if(En==1)
	{	
		if(Time<0) return 1;
		PTO_pagram_run[Ch].P_PStop_flag=1;
		if((PTO_pagram_run[Ch].P_cur_FB_NO!=index)&&(PTO_pagram_run[Ch].Exception_Code==0))
		{
			PTO_pagram_run[Ch].Exception_Code=2;
			PTO_pagram_run[Ch].P_cur_FB_NO=index;
			if((Time>0)&&(PTO_pagram_run[Ch].Freq_cur>PTO_pagram_set[Ch].Freq_min))
			{
				PTO_pagram_run[Ch].P_plus_EN=1;
				PTO_pagram_run[Ch].P_state=1;
				* State=0;
				PTO_pagram_run[Ch].P_cur_FB_mode=PTO_PSTOP;
				if(P_ABS_INC_Flag==0)	PTO_pagram_run[Ch].P_INC=0;
				else PTO_pagram_run[Ch].P_INC=1;
				PTO_Mode_Set(Ch,PTO_Mode);
				PTO_pagram_run[Ch].Freq_target=PTO_pagram_set[Ch].Freq_min;
				PTO_pagram_run[Ch].ADT_cur=(PTO_pagram_run[Ch].Freq_cur-PTO_pagram_set[Ch].Freq_min)*5/Time;
				PTO_pagram_run[Ch].P_Stop_Req=1;	
			}
			else
			{
				PTO_Rst_set(Ch);
				CPLD_IO_Config(PTO_PSTOP,Ch,0);
				PTO_pagram_run[Ch].P_plus_EN=0;
				PTO_pagram_run[Ch].P_state=0;
				PTO_pagram_run[Ch].P_Stop_Req=0;
				* State=1;
				PTO_pagram_run[Ch].P_cur_FB_mode=0;
				PTO_NO_Stop_Set(Ch,0);
				PTO_pagram_run[Ch].Freq_cur=0;
			}
		}
		else if(PTO_pagram_run[Ch].P_cur_FB_NO==index)
		{
			* State=0;
			if((PTO_pagram_run[Ch].Freq_cur<=PTO_pagram_set[Ch].Freq_min)&&(PTO_pagram_run[Ch].P_plus_EN==1))
			{
				PTO_Rst_set(Ch);
				CPLD_IO_Config(PTO_PSTOP,Ch,0);
				PTO_pagram_run[Ch].P_plus_EN=0;
				PTO_pagram_run[Ch].P_state=0;
				PTO_pagram_run[Ch].P_Stop_Req=0;
				* State=1;
				PTO_pagram_run[Ch].P_cur_FB_mode=0;
				PTO_NO_Stop_Set(Ch,0);
				PTO_pagram_run[Ch].Freq_cur=0;
			}
		}
		else * State=0;
	}
	else
	{
		* State=0;
		PTO_pagram_run[Ch].P_PStop_flag=0;
		if(PTO_pagram_run[Ch].P_cur_FB_NO==index)
		{
			PTO_Rst_set(Ch);
			CPLD_IO_Config(PTO_PSTOP,Ch,0);
			PTO_pagram_run[Ch].P_plus_EN=0;
			PTO_pagram_run[Ch].P_state=0;
			PTO_pagram_run[Ch].P_Stop_Req=0;
			PTO_pagram_run[Ch].P_cur_FB_NO=0;
			PTO_pagram_run[Ch].P_cur_FB_mode=0;
			PTO_NO_Stop_Set(Ch,0);
			PTO_pagram_run[Ch].Freq_cur=0;
		}
	}
	return 0;
}
/////////////////myabs����λ��ָ��/////////////////////////////////////////
int mypabs(unsigned char En)
{
	if(En==1)
	{
		P_ABS_INC_Flag=0;
	}
	return 0;
}
/////////////////myinc���λ��ָ��/////////////////////////////////////////
int mypinc(unsigned char En)
{
	if(En==1)
	{
		P_ABS_INC_Flag=1;
	}
	return 0;
}
#ifdef RT_USING_FINSH

#endif

#ifdef RT_USING_FINSH
#include <finsh.h>

#endif

void rt_thread_cpld(void* parameter)
{
	
}

