/*
芯片：STC12C5A60S2
晶振：22.1184MHz  	波特率：9600bps
功能：STC12双串口通信（中断方式）
      当串口-1接收数据后，将此数据由串口-2发送出去
      当串口-2接收数据后，将此数据由串口-1发送出去
*/
#include "12C5A60S2.h"
#define S2RI 0x01
//串口-2接收中断请求标志位
#define S2TI 0x02
//串口-2发送中断请求标志位
unsigned char flag1,flag2,temp1,temp2;

unsigned char hex2strTable[2] = "";
unsigned char str2hexTable[2] = "";
unsigned char str2hexData = 0;
unsigned char flagTwiceCtn = 0;

void hex2str (unsigned char input) {
	unsigned char high = input >> 4;
	unsigned char low = input & 0x0f;
	if(high > 0x09) {
		hex2strTable[0] = high + 55;
	}else{
		hex2strTable[0] = high + 48;
	}
	if(low > 0x09) {
		hex2strTable[1] = low + 55;
	}else{
		hex2strTable[1] = low + 48;
	}
}

void twiceCtn (void) {
	flagTwiceCtn ++;
	flagTwiceCtn = flagTwiceCtn % 2;
}

void str2hex (unsigned char high, unsigned char low) {
 	unsigned char hex;
	if(high > 64) {
		hex = (high - 55);
	}else{
		hex = (high - 48);
	} 
	if(low > 64) {
		hex = hex << 4;
		hex += (low - 55);
	}else{
		hex = hex << 4;
		hex += (low - 48);
	}
	str2hexData = hex;
}


/*
	串口初始化函数
*/
void InitUART(void)
{
    TMOD = 0x20;
    //定时器-1工作在方式2  8位自动重装
    SCON = 0x50;
    //串口-1工作在方式1  10位异步收发 REN=1允许接收
    TH1 = 0xfd;
    //定时器-1初值  9600
    TL1 = TH1;
    TR1 = 1;
    //定时器-1开始计数
    EA =1;
    //开总中断
    ES =1;
    //开串口-1中断

    S2CON = 0x50;
    //串口-2工作在方式1  10位异步收发 S2REN=1允许接收
    BRT = 0xfd;
    //独立波特率发生器初值 9600
    AUXR = 0x10;
    //BRTR=1 独立波特率发生器开始计数
    IE2 =0x01;
    //开串口-2中断  ES2=1
}
/*
	串口-1 发送
 */
void UART_1SendOneByte(unsigned char c)
{
    SBUF = c;
    while(!TI);
    // 若TI=0，在此等待
    TI = 0;	 
}
/*
	串行口2发送
*/
void UART_2SendOneByte(unsigned char c)
{
    S2BUF = c;
    while(!(S2CON&S2TI));
    // 若S2TI=0，在此等待
    S2CON&=~S2TI;
    // S2TI=0
}
/*
	主函数
*/
void main(void)
{
	InitUART();
	// 串行口初始化
	while(1)
	{
		// 如果串口-1接收到数据，将此数据由串口-2发送
		if(flag1==1)
		{
			flag1=0;
			hex2str(temp1);
			UART_2SendOneByte(hex2strTable[0]);
			UART_2SendOneByte(hex2strTable[1]);
		}
		// 如果串口-2接收到数据，将此数据由串口-1发送
		if(flag2==1)
		{
			flag2=0;
			twiceCtn();
			str2hexTable[flagTwiceCtn] = temp2;
			if( flagTwiceCtn == 1){
				str2hex(str2hexTable[0], str2hexTable[1]);
				UART_1SendOneByte(str2hexData);
			}
		}
	}
}
/*
	串口-1中断处理函数
 */
void UART_1Interrupt(void) interrupt 4
{
	if(RI==1)
	{
		RI=0;
		flag1=1;
		temp1=SBUF;
	}
}
/*
	串口-2中断处理函数
*/
void UART_2Interrupt(void) interrupt 8
{
	if(S2CON&S2RI)
	{
		S2CON&=~S2RI;
		flag2=1;
		temp2=S2BUF;
	} 
}
