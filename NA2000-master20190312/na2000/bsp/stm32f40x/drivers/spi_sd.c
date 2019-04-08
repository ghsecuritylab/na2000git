#include "spi_sd.h"

#define SPI_SPEED_1   1
#define SPI_SPEED_2   2
#define SPI_SPEED_4   4
#define SPI_SPEED_8   8
#define SPI_SPEED_16  16
#define SPI_SPEED_256 256

#define SPI_SD_WORK_HZ			24000000
#define SPI_SD_WORK_SPEED		SPI_SPEED_1
	
u8  SD_Type=0;//SD��������
//Mini STM32������
//SD�� ����
//����ԭ��@ALIENTEK

//2010/5/13									   
//������һЩ��ʱ,ʵ�����֧��TF��(1G/2G),��ʿ��2G,4G 16G SD��
//2010/6/24
//������u8 SD_GetResponse(u8 Response)����
//�޸���u8 SD_WaitDataReady(void)����
//������USB������֧�ֵ�u8 MSD_ReadBuffer(u8* pBuffer, u32 ReadAddr, u32 NumByteToRead);
//��u8 MSD_WriteBuffer(u8* pBuffer, u32 WriteAddr, u32 NumByteToWrite);��������
struct SD_CardInfo
{
  uint32_t CardCapacity;  /*!< Card Capacity */
  uint32_t CardBlockSize; /*!< Card Block Size */
  uint16_t RCA;
  uint8_t CardType;
};

struct spi_sd_device
{
    struct rt_device                sd_device;
    struct rt_spi_device *          rt_spi_device;
};

struct spi_sd_device spi_sd_device;

struct SD_CardInfo sdinfo;

void SPI_SetSpeed(u16 speed)
{
	struct rt_spi_configuration cfg;
	cfg.data_width = 8;
	cfg.mode = RT_SPI_MODE_3 | RT_SPI_MSB; /* SPI Compatible Modes 0 and 3 */
	cfg.max_hz = SPI_SD_WORK_HZ / speed; /* Atmel RapidS Serial Interface: 66MHz Maximum Clock Frequency */
	rt_spi_configure(spi_sd_device.rt_spi_device, &cfg);
}
//SPIx ��дһ���ֽ�
//TxData:Ҫд����ֽ�
//����ֵ:��ȡ�����ֽ�
u8 SPIx_ReadWriteByte(u8 TxData)
{		
	u8 ret;
	rt_spi_transfer(spi_sd_device.rt_spi_device,&TxData,&ret,1);
	return ret;
}

//�ȴ�SD����Ӧ
//Response:Ҫ�õ��Ļ�Ӧֵ
//����ֵ:0,�ɹ��õ��˸û�Ӧֵ
//    ����,�õ���Ӧֵʧ��
u8 SD_GetResponse(u8 Response)
{
	u16 Count=0xFFF;//�ȴ�����	   						  
	while ((SPIx_ReadWriteByte(0XFF)!=Response)&&Count)Count--;//�ȴ��õ�׼ȷ�Ļ�Ӧ  	  
	if (Count==0)return MSD_RESPONSE_FAILURE;//�õ���Ӧʧ��   
	else return MSD_RESPONSE_NO_ERROR;//��ȷ��Ӧ
}
//�ȴ�SD��д�����
//����ֵ:0,�ɹ�;   
//    ����,�������;
u8 SD_WaitDataReady(void)
{
	u8 r1=MSD_DATA_OTHER_ERROR;
	u32 retry;
	retry=0;
	do
	{
		r1=SPIx_ReadWriteByte(0xFF)&0X1F;//������Ӧ
		if(retry==0xfffe)
			return 1; 
		retry++;
		switch (r1)
		{					   
		case MSD_DATA_OK://���ݽ�����ȷ��	 
			r1=MSD_DATA_OK;
		break;  
		case MSD_DATA_CRC_ERROR:  //CRCУ�����
			return MSD_DATA_CRC_ERROR;  
		case MSD_DATA_WRITE_ERROR://����д�����
			return MSD_DATA_WRITE_ERROR;  
		default://δ֪����    
			r1=MSD_DATA_OTHER_ERROR;
		break;	 
		}   
	}while(r1==MSD_DATA_OTHER_ERROR); //���ݴ���ʱһֱ�ȴ�
	
	retry=0;
	
	while(SPIx_ReadWriteByte(0XFF)==0)//��������Ϊ0,�����ݻ�δд���
	{
		retry++;
		//delay_us(10);//SD��д�ȴ���Ҫ�ϳ���ʱ��
		if(retry>=0XFFFFFFFE)return 0XFF;//�ȴ�ʧ����
	}  
	return 0;//�ɹ���
}	 
//��SD������һ������
//����: u8 cmd   ���� 
//      u32 arg  �������
//      u8 crc   crcУ��ֵ	   
//����ֵ:SD�����ص���Ӧ															  
u8 SD_SendCommand(u8 cmd, u32 arg, u8 crc)
{
	u8 r1;	
	u8 Retry=0;	         
	SPIx_ReadWriteByte(0xff);//����д������ʱ
	SPIx_ReadWriteByte(0xff);     
	SPIx_ReadWriteByte(0xff);  	 
	//����
	SPIx_ReadWriteByte(cmd | 0x40);//�ֱ�д������
	SPIx_ReadWriteByte(arg >> 24);
	SPIx_ReadWriteByte(arg >> 16);
	SPIx_ReadWriteByte(arg >> 8);
	SPIx_ReadWriteByte(arg);
	SPIx_ReadWriteByte(crc); 
	//�ȴ���Ӧ����ʱ�˳�
	while((r1=SPIx_ReadWriteByte(0xFF))==0xFF)
	{
		Retry++;	    
		if(Retry>200)break; 
	}   
	//�������϶�������8��ʱ�ӣ���SD�����ʣ�µĹ���
	SPIx_ReadWriteByte(0xFF);
	//����״ֵ̬
	return r1;
}		  																				 
//��SD������һ������(�����ǲ�ʧ��Ƭѡ�����к������ݴ�����
//����:u8 cmd   ���� 
//     u32 arg  �������
//     u8 crc   crcУ��ֵ	 
//����ֵ:SD�����ص���Ӧ															  
u8 SD_SendCommand_NoDeassert(u8 cmd, u32 arg, u8 crc)
{
	u8 Retry=0;	         
	u8 r1;			   
	SPIx_ReadWriteByte(0xff);//����д������ʱ
	SPIx_ReadWriteByte(0xff);  	 	    
	//����
	SPIx_ReadWriteByte(cmd | 0x40); //�ֱ�д������
	SPIx_ReadWriteByte(arg >> 24);
	SPIx_ReadWriteByte(arg >> 16);
	SPIx_ReadWriteByte(arg >> 8);
	SPIx_ReadWriteByte(arg);
	SPIx_ReadWriteByte(crc);   
	//�ȴ���Ӧ����ʱ�˳�
	while((r1=SPIx_ReadWriteByte(0xFF))==0xFF)
	{
		Retry++;	    
		if(Retry>200)break; 
	}  	  
	//������Ӧֵ
	return r1;
}
//��SD�����õ�����ģʽ
//����ֵ:0,�ɹ�����
//       1,����ʧ��
u8 SD_Idle_Sta(void)
{
	u16 i;
	u8 retry;
	rt_thread_delay(1); 
	//�Ȳ���>74�����壬��SD���Լ���ʼ�����
	for(i=0;i<20;i++)SPIx_ReadWriteByte(0xFF); 
	//-----------------SD����λ��idle��ʼ-----------------
	//ѭ����������CMD0��ֱ��SD������0x01,����IDLE״̬
	//��ʱ��ֱ���˳�
	retry = 0;
	do
	{	   
		//����CMD0����SD������IDLE״̬
		i = SD_SendCommand(CMD0, 0, 0x95);
		retry++;
	}while((i!=0x01)&&(retry<200));
	//����ѭ���󣬼��ԭ�򣺳�ʼ���ɹ���or ���Գ�ʱ��
	if(retry==200)return 1; //ʧ��
	return 0;//�ɹ�	 						  
}														    
//��ʼ��SD��
//����ɹ�����,����Զ�����SPI�ٶ�Ϊ18Mhz
//����ֵ:0��NO_ERR
//       1��TIME_OUT
//      99��NO_CARD																 
u8 SD_Init(void)
{			
	u8 r1;      // ���SD���ķ���ֵ
	u16 retry;  // �������г�ʱ����
	u8 buff[6];	
	SPI_SetSpeed(SPI_SPEED_256);//���õ�����ģʽ
	if(SD_Idle_Sta()) 
	{
		r1 = 1;//��ʱ����1 ���õ�idle ģʽʧ��
		goto out;
	}
	//-----------------SD����λ��idle����-----------------	 
	//��ȡ��Ƭ��SD�汾��Ϣ
	r1 = SD_SendCommand_NoDeassert(8, 0x1aa,0x87);	     
	//�����Ƭ�汾��Ϣ��v1.0�汾�ģ���r1=0x05����������³�ʼ��
	if(r1 == 0x05)
	{
		//���ÿ�����ΪSDV1.0����������⵽ΪMMC�������޸�ΪMMC
		SD_Type = SD_TYPE_V1;	   
		//�����V1.0����CMD8ָ���û�к�������
		//�෢8��CLK����SD������������
		SPIx_ReadWriteByte(0xFF);	  
		//-----------------SD����MMC����ʼ����ʼ-----------------	 
		//������ʼ��ָ��CMD55+ACMD41
		// �����Ӧ��˵����SD�����ҳ�ʼ�����
		// û�л�Ӧ��˵����MMC�������������Ӧ��ʼ��
		retry = 0;
		do
		{
			//�ȷ�CMD55��Ӧ����0x01���������
			r1 = SD_SendCommand(CMD55, 0, 0);
			if(r1 == 0XFF)
				goto out;//ֻҪ����0xff,�ͽ��ŷ���
			
			//�õ���ȷ��Ӧ�󣬷�ACMD41��Ӧ�õ�����ֵ0x00����������200��
			r1 = SD_SendCommand(ACMD41, 0, 0);
			retry++;
		}while((r1!=0x00) && (retry<400));
		
		// �ж��ǳ�ʱ���ǵõ���ȷ��Ӧ
		// ���л�Ӧ����SD����û�л�Ӧ����MMC��	  
		//----------MMC�������ʼ��������ʼ------------
		if(retry==400)
		{
			retry = 0;
			//����MMC����ʼ�����û�в��ԣ�
			do
			{
				r1 = SD_SendCommand(1,0,0);
				retry++;
			}while((r1!=0x00)&& (retry<400));
			
			if(retry==400)
			{
				r1=1;   //MMC����ʼ����ʱ
				goto out;
			}
			//д�뿨����
			SD_Type = SD_TYPE_MMC;
		}
		//----------MMC�������ʼ����������------------	    
		//����SPIΪ����ģʽ
		SPI_SetSpeed(SPI_SD_WORK_SPEED);   
		SPIx_ReadWriteByte(0xFF);	 
		//��ֹCRCУ��	   
		r1 = SD_SendCommand(CMD59, 0, 0x95);
		if(r1 != 0x00) 
			goto out;  //������󣬷���r1   	   
		//����Sector Size
		r1 = SD_SendCommand(CMD16, 512, 0x95);
		if(r1 != 0x00) 
			goto out;//������󣬷���r1		 
		//-----------------SD����MMC����ʼ������-----------------

	}
	//SD��ΪV1.0�汾�ĳ�ʼ������	 
	//������V2.0���ĳ�ʼ��
	//������Ҫ��ȡOCR���ݣ��ж���SD2.0����SD2.0HC��
	else if(r1 == 0x01)
	{
		//V2.0�Ŀ���CMD8�����ᴫ��4�ֽڵ����ݣ�Ҫ�����ٽ���������
		buff[0] = SPIx_ReadWriteByte(0xFF);  //should be 0x00
		buff[1] = SPIx_ReadWriteByte(0xFF);  //should be 0x00
		buff[2] = SPIx_ReadWriteByte(0xFF);  //should be 0x01
		buff[3] = SPIx_ReadWriteByte(0xFF);  //should be 0xAA	      
		SPIx_ReadWriteByte(0xFF);//the next 8 clocks			 
		//�жϸÿ��Ƿ�֧��2.7V-3.6V�ĵ�ѹ��Χ
		//if(buff[2]==0x01 && buff[3]==0xAA) //���жϣ�����֧�ֵĿ�����
		{	  
			retry = 0;
			//������ʼ��ָ��CMD55+ACMD41
			do
			{
				r1 = SD_SendCommand(CMD55, 0, 0);
				if(r1!=0x01)
					return r1;	   
				r1 = SD_SendCommand(ACMD41, 0x40000000, 0);
				if(retry>200)
					return r1;  //��ʱ�򷵻�r1״̬  
			}while(r1!=0);		  
			//��ʼ��ָ�����ɣ���������ȡOCR��Ϣ		   
			//-----------����SD2.0���汾��ʼ-----------
			r1 = SD_SendCommand_NoDeassert(CMD58, 0, 0);
			if(r1!=0x00)
			{
				return r1;  //�������û�з�����ȷӦ��ֱ���˳�������Ӧ��	 
			}//��OCRָ����󣬽�������4�ֽڵ�OCR��Ϣ
			buff[0] = SPIx_ReadWriteByte(0xFF);
			buff[1] = SPIx_ReadWriteByte(0xFF); 
			buff[2] = SPIx_ReadWriteByte(0xFF);
			buff[3] = SPIx_ReadWriteByte(0xFF);		 
			//OCR������ɣ�Ƭѡ�ø�
			SPIx_ReadWriteByte(0xFF);	   
			//�����յ���OCR�е�bit30λ��CCS����ȷ����ΪSD2.0����SDHC
			//���CCS=1��SDHC   CCS=0��SD2.0
			if(buff[0]&0x40)SD_Type = SD_TYPE_V2HC;    //���CCS	 
			else SD_Type = SD_TYPE_V2;	
			//-----------����SD2.0���汾����----------- 
		}	    
	}
	
out:
	//����SPIΪ����ģʽ
	SPI_SetSpeed(SPI_SD_WORK_SPEED);
	return r1;
}	 																			   
//��SD���ж���ָ�����ȵ����ݣ������ڸ���λ��
//����: u8 *data(��Ŷ������ݵ��ڴ�>len)
//      u16 len(���ݳ��ȣ�
//      u8 release(������ɺ��Ƿ��ͷ�����CS�ø� 0�����ͷ� 1���ͷţ�	 
//����ֵ:0��NO_ERR
//  	 other��������Ϣ														  
u8 SD_ReceiveData(u8 *data, u16 len, u8 release)
{
	// ����һ�δ���				  	  
	if(SD_GetResponse(0xFE))//�ȴ�SD������������ʼ����0xFE
	{
		return 1;
	}
	while(len--)//��ʼ��������
	{
		*data=SPIx_ReadWriteByte(0xFF);
		data++;
	}
	//������2��αCRC��dummy CRC��
	SPIx_ReadWriteByte(0xFF);
	SPIx_ReadWriteByte(0xFF);
	if(release==RELEASE)//�����ͷ����ߣ���CS�ø�
	{
		SPIx_ReadWriteByte(0xFF);
	}											  					    
	return 0;
}

//��ȡSD����CID��Ϣ��������������Ϣ
//����: u8 *cid_data(���CID���ڴ棬����16Byte��	  
//����ֵ:0��NO_ERR
//		 1��TIME_OUT
//       other��������Ϣ														   
u8 SD_GetCID(u8 *cid_data)
{
	u8 r1;	   
	//��CMD10�����CID
	r1 = SD_SendCommand(CMD10,0,0xFF);
	if(r1 != 0x00)return r1;  //û������ȷӦ�����˳�������  
	SD_ReceiveData(cid_data,16,RELEASE);//����16���ֽڵ�����	 
	return 0;
}																				  
//��ȡSD����CSD��Ϣ�������������ٶ���Ϣ
//����:u8 *cid_data(���CID���ڴ棬����16Byte��	    
//����ֵ:0��NO_ERR
//       1��TIME_OUT
//       other��������Ϣ														   
u8 SD_GetCSD(u8 *csd_data)
{
	u8 r1;	 
	r1=SD_SendCommand(CMD9,0,0xFF);//��CMD9�����CSD
	if(r1)return r1;  //û������ȷӦ�����˳�������  
	SD_ReceiveData(csd_data, 16, RELEASE);//����16���ֽڵ����� 
	return 0;
}  
//��ȡSD�����������ֽڣ�   
//����ֵ:0�� ȡ�������� 
//       ����:SD��������(�ֽ�)														  
u32 SD_GetCapacity(void)
{
	u8 csd[16];
	u32 Capacity;
	u8 r1;
	u16 i;
	u16 temp;  					    
//ȡCSD��Ϣ������ڼ��������0
	if(SD_GetCSD(csd)!=0) return 0;	    
	//���ΪSDHC�����������淽ʽ����
	if((csd[0]&0xC0)==0x40)
	{									  
		Capacity=((u32)csd[8])<<8;
		Capacity+=(u32)csd[9]+1;	 
		Capacity = (Capacity)*1024;//�õ�������
		Capacity*=512;//�õ��ֽ���
		sdinfo.CardCapacity = 	Capacity;	
		sdinfo.CardBlockSize = 512;
	}
	else
	{		    
		i = csd[6]&0x03;
		i<<=8;
		i += csd[7];
		i<<=2;
		i += ((csd[8]&0xc0)>>6);
		//C_SIZE_MULT
		r1 = csd[9]&0x03;
		r1<<=1;
		r1 += ((csd[10]&0x80)>>7);	 
		r1+=2;//BLOCKNR
		temp = 1;
		while(r1)
		{
			temp*=2;
			r1--;
		}
		Capacity = ((u32)(i+1))*((u32)temp);
		sdinfo.CardBlockSize = temp;			
		// READ_BL_LEN
		i = csd[5]&0x0f;
		//BLOCK_LEN
		temp = 1;
		while(i)
		{
			temp*=2;
			i--;
		}
		//The final result
		Capacity *= (u32)temp;//�ֽ�Ϊ��λ 	
		sdinfo.CardCapacity = Capacity;			
	}
	return (u32)Capacity;
}	    																			    
//��SD����һ��block
//����:u32 sector ȡ��ַ��sectorֵ���������ַ�� 
//     u8 *buffer ���ݴ洢��ַ����С����512byte�� 		   
//����ֵ:0�� �ɹ�
//       other��ʧ��															  
u8 SD_ReadSingleBlock(u32 sector, u8 *buffer)
{
	u8 r1;	    
	//����Ϊ����ģʽ
	SPI_SetSpeed(SPI_SD_WORK_SPEED);  		   
	//�������SDHC����������sector��ַ������ת����byte��ַ
	if(SD_Type!=SD_TYPE_V2HC)
	{
		sector = sector<<9;
	} 
	r1 = SD_SendCommand(CMD17, sector, 0);//������	 		    
	if(r1 != 0x00)
	return r1; 		   							  
	r1 = SD_ReceiveData(buffer, 512, RELEASE);
	//	r1 = SD_DMA_ReceiveData(buffer, 512);
	if(r1 != 0)
		return r1;   //�����ݳ���
	else 
		return 0; 
}

//////////////////////////////////////////////////////////////////////////
//д��SD����һ��block(δʵ�ʲ��Թ�)										    
//����:u32 sector ������ַ��sectorֵ���������ַ�� 
//     u8 *buffer ���ݴ洢��ַ����С����512byte�� 		   
//����ֵ:0�� �ɹ�
//       other��ʧ��															  
u8 SD_WriteSingleBlock(u32 sector, const u8 *data)
{
	u8 r1;
	u16 i;
	u16 retry;
 
	//�������SDHC����������sector��ַ������ת����byte��ַ
	if(SD_Type!=SD_TYPE_V2HC)
	{
			sector = sector<<9;
	}   
	r1 = SD_SendCommand(CMD24, sector, 0x00);
	if(r1 != 0x00)
	{
			return r1;  //Ӧ����ȷ��ֱ�ӷ���
	}
	
	//�ȷ�3�������ݣ��ȴ�SD��׼����
	SPIx_ReadWriteByte(0xff);
	SPIx_ReadWriteByte(0xff);
	SPIx_ReadWriteByte(0xff);
	//����ʼ����0xFE
	SPIx_ReadWriteByte(0xFE);

//		SD_DMA_SendData((u8 *)data,512);
	//��һ��sector������
	for(i=0;i<512;i++)
	{
			SPIx_ReadWriteByte(*data++);
	}
	//��2��Byte��dummy CRC
	SPIx_ReadWriteByte(0xff);
	SPIx_ReadWriteByte(0xff);
	
	//�ȴ�SD��Ӧ��
	r1 = SPIx_ReadWriteByte(0xff);
	if((r1&0x1F)!=0x05)
	{
		return r1;
	}
	
	//�ȴ��������
	retry = 0;
	while(!SPIx_ReadWriteByte(0xff))
	{
		rt_thread_delay(1);
		if(retry++>100)
			return 1;
	}
	
	//д�����
	SPIx_ReadWriteByte(0xff);

	return 0;
}				           
//��SD���Ķ��block(ʵ�ʲ��Թ�)										    
//����:u32 sector ������ַ��sectorֵ���������ַ�� 
//     u8 *buffer ���ݴ洢��ַ����С����512byte��
//     u8 count ������count��block 		   
//����ֵ:0�� �ɹ�
//       other��ʧ��															  
u8 SD_ReadMultiBlock(u32 sector, u8 *buffer, u8 count)
{
	u8 r1;	 			 
	//�������SDHC����sector��ַת��byte��ַ
	if(SD_Type!=SD_TYPE_V2HC)sector = sector<<9;  
	//SD_WaitDataReady();
	//�����������
	r1 = SD_SendCommand(CMD18, sector, 0);//������
	if(r1 != 0x00)return r1;	 
	do//��ʼ��������
	{
			if(SD_ReceiveData(buffer, 512, NO_RELEASE) != 0x00)break; 
			buffer += 512;
	} while(--count);		 
	//ȫ��������ϣ�����ֹͣ����
	SD_SendCommand(CMD12, 0, 0);
	
	SPIx_ReadWriteByte(0xFF);    
	if(count != 0)return count;   //���û�д��꣬����ʣ�����	 
	else return 0;	 
}											  
//д��SD����N��block(δʵ�ʲ��Թ�)									    
//����:u32 sector ������ַ��sectorֵ���������ַ�� 
//     u8 *buffer ���ݴ洢��ַ����С����512byte��
//     u8 count д���block��Ŀ		   
//����ֵ:0�� �ɹ�
//       other��ʧ��															   
u8 SD_WriteMultiBlock(u32 sector, const u8 *data, u8 count)
{
	u8 r1;
	u16 i;	 		 
	if(SD_Type != SD_TYPE_V2HC)sector = sector<<9;//�������SDHC����������sector��ַ������ת����byte��ַ  
	if(SD_Type != SD_TYPE_MMC) r1 = SD_SendCommand(ACMD23, count, 0x00);//���Ŀ�꿨����MMC��������ACMD23ָ��ʹ��Ԥ����   
	r1 = SD_SendCommand(CMD25, sector, 0x00);//�����д��ָ��
	if(r1 != 0x00)return r1;  //Ӧ����ȷ��ֱ�ӷ���
	SPIx_ReadWriteByte(0xff);//�ȷ�3�������ݣ��ȴ�SD��׼����
	SPIx_ReadWriteByte(0xff);   
	//--------������N��sectorд���ѭ������
	do
	{
		//����ʼ����0xFC �����Ƕ��д��
		SPIx_ReadWriteByte(0xFC);	  
		//��һ��sector������
		for(i=0;i<512;i++)
		{
			SPIx_ReadWriteByte(*data++);
		}
		//��2��Byte��dummy CRC
		SPIx_ReadWriteByte(0xff);
		SPIx_ReadWriteByte(0xff);

		//�ȴ�SD��Ӧ��
		r1 = SPIx_ReadWriteByte(0xff);
		if((r1&0x1F)!=0x05)
		{
			//���Ӧ��Ϊ��������������ֱ���˳�
			return r1;
		}		   
		//�ȴ�SD��д�����
		if(SD_WaitDataReady()==1)
		{
			//�ȴ�SD��д����ɳ�ʱ��ֱ���˳�����
			return 1;
		}	   
	}while(--count);//��sector���ݴ������  
	//��������������0xFD
	r1 = SPIx_ReadWriteByte(0xFD);
	if(r1==0x00)
	{
		count =  0xfe;
	}		   
	if(SD_WaitDataReady()) //�ȴ�׼����
	{
		return 1;  
	}
	//д�����
	SPIx_ReadWriteByte(0xff);  
	return count;   //����countֵ�����д����count=0������count=1
}						  					  

rt_size_t spi_sd_read(rt_device_t dev, rt_off_t offset, void * buf, rt_size_t size)
{
	rt_size_t res = size;
	rt_size_t ret = 0;
	rt_off_t off = 0;
	char *buff = (char *)buf;
	
	while(res>0)
	{
		ret += (rt_size_t)SD_ReadSingleBlock((u32)(offset+off), (u8 *)buff);
		off++;
		res -= 1;
		buff += 512;
	}

	if(!res)
		return size;
	else
		return 0;
}

rt_size_t spi_sd_write(rt_device_t dev, rt_off_t offset,const void * buf, rt_size_t size)
{
	rt_size_t res = size;
	rt_size_t ret = 0;
	rt_size_t off = 0;
	char *buff = (char *)buf;

	while(res>0)
	{
		ret += (rt_size_t)SD_WriteSingleBlock((u32)offset+off, (u8 *)buff);
		off++;
		res -= 1;
		buff += 512;
	}
	
	if(!res)
		return size;
	else
		return 0;
}
static rt_err_t spi_sd_init(rt_device_t dev)
{
	u8 state;
	state=SD_Init();
	if(state)
	 return RT_EIO;
	
	return RT_EOK;
}
static rt_err_t spi_sd_open(rt_device_t dev, rt_uint16_t oflag)
{
	SD_GetCapacity();
	return RT_EOK;
}
static rt_err_t spi_sd_close(rt_device_t dev)
{

	return RT_EOK;
}

static rt_err_t spi_sd_control(rt_device_t dev, int cmd, void *args)
{
	RT_ASSERT(dev != RT_NULL);

	if (cmd == RT_DEVICE_CTRL_BLK_GETGEOME)
	{
		struct rt_device_blk_geometry *geometry;

		geometry = (struct rt_device_blk_geometry *)args;
		if (geometry == RT_NULL) return -RT_ERROR;

		geometry->bytes_per_sector = 512;
		geometry->block_size = sdinfo.CardBlockSize;
		geometry->sector_count = sdinfo.CardCapacity/sdinfo.CardBlockSize;
	}

return RT_EOK;
}
	
void spi_sd_hw_init(const char * sd_device_name, const char * spi_device_name)
{
	struct rt_spi_device * rt_spi_device;
	rt_spi_device = (struct rt_spi_device *)rt_device_find(spi_device_name);
	if(rt_spi_device == RT_NULL)
			return;
	spi_sd_device.rt_spi_device = rt_spi_device;
	
 /* config spi */
	{
			struct rt_spi_configuration cfg;
			cfg.data_width = 8;
			cfg.mode = RT_SPI_MODE_0 | RT_SPI_MSB; /* SPI Compatible Modes 0 and 3 */
			cfg.max_hz = SPI_SD_WORK_HZ; 
			rt_spi_configure(spi_sd_device.rt_spi_device, &cfg);
	}

		
	spi_sd_device.sd_device.type    = RT_Device_Class_Block;
	spi_sd_device.sd_device.init    = spi_sd_init;
	spi_sd_device.sd_device.open    = spi_sd_open;
	spi_sd_device.sd_device.close   = spi_sd_close;
	spi_sd_device.sd_device.read 	 	= spi_sd_read;
	spi_sd_device.sd_device.write   = spi_sd_write;
	spi_sd_device.sd_device.control = spi_sd_control;
	/* no private */
	spi_sd_device.sd_device.user_data = RT_NULL;

	rt_device_register(&spi_sd_device.sd_device, sd_device_name,
										 RT_DEVICE_FLAG_RDWR | RT_DEVICE_FLAG_STANDALONE);

}

void spi_sd_hw_uninit()
{
	rt_device_unregister(&spi_sd_device.sd_device);
}

#ifdef SD_HOT_PLUG_TEST

#ifdef RT_USING_DFS
/* dfs init */
//#include <dfs_init.h>
/* dfs filesystem:ELM filesystem init */
#include <dfs_elm.h>
/* dfs Filesystem APIs */
#include <dfs_fs.h>
#endif

#define SD_MOUNTPOINT		"/sd0"

#define SD_CD_PIN               GPIO_Pin_11
#define SD_CD_PORT          	  GPIOD
#define SD_CD_PIN_SOURCE				GPIO_PinSource11
#define SD_CD_CLK_PORT					RCC_AHB1Periph_GPIOD

extern void rt_spi_sd_device_init(void);
extern void rt_spi_sd_device_uninit(void);

void sd_hotplug_server(void *param)
{
	uint8_t sd_plugged_in = 0;
	uint8_t mounted = 0;			//sd has been mounted success flag
	
	while(1) 
	{
		// when no sd card, gpio read value is 1
		sd_plugged_in = !GPIO_ReadInputDataBit(SD_CD_PORT,SD_CD_PIN);
		
		// this for elimination of jitter
		rt_thread_delay(50*RT_TICK_PER_SECOND/1000);
		
		// when no sd card, gpio read value is 1
		sd_plugged_in &= !GPIO_ReadInputDataBit(SD_CD_PORT,SD_CD_PIN);
		
		// sd card plugged out
		if(sd_plugged_in)
		{
			if(!mounted)
			{
				#ifdef RT_USING_DFS
					spi_sd_hw_init("sd0","spi12");
				#endif
			
				#ifdef RT_USING_DFS_ELMFAT
				/* mount sd card fat partition 1 as /sd0 directory */
					mounted = !dfs_mount("sd0", SD_MOUNTPOINT, "elm", 0, 0);
				#endif
			}
		}
		else
		{
			if(mounted)
			{
				#ifdef RT_USING_DFS_ELMFAT
					dfs_unmount(SD_MOUNTPOINT); //we don't care unmount return value cause it's safe
				#endif
				#ifdef RT_USING_DFS
					spi_sd_hw_uninit();
				#endif
				mounted = 0;
			}
		}
	}
}

void sd_hotplug_init(void)
{
	rt_thread_t tid;

	tid = rt_thread_create("sd_hp",sd_hotplug_server,RT_NULL,256*3,20,20);
	
	if(tid != RT_NULL) 
	{
		rt_kprintf("create sd_hotplug_server thread successed\n");
		rt_thread_startup(tid);
	}
}

#endif
