/* MAIN.C file
 *
 * Copyright (c) 2002-2005 STMicroelectronics
 */
#include "STM8S103f3p.h"
// 全局常量、变量定义
#define DELAYTIME 30
#define DEVICEID 0x6ff0
unsigned char recvTable[32] = "";
unsigned char sendTable[32] = "";
unsigned char dataTable[16] = "";
unsigned char hex2strTable[2] = "";
unsigned char i;
//
// 串口数据存入指针
void Init_UART1(void) {
    UART1_CR1 = 0x00;
    UART1_CR2 = 0x00;
    UART1_CR3 = 0x00;
    UART1_BRR2 = 0x00;
    UART1_BRR1 = 0x0d;
    UART1_CR2 = 0x2c;
}


void initGPIO(void) {    
    PB_DDR|=0x20;
    // 0010 0000 PX.5 为输出
    PB_CR1|=0x20;
    // 0010 0000 Px.5 为推挽
    PB_CR2|=0x00;
    // 低速输出模式
}



void hex2str(unsigned char input) {
    /*
        没有想出什么理想的算法，就先这样了
        需要定义一个全局的table hex2strTable
        两个元素，作为中转
     */
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
void wipeRecvTable(void) {
    unsigned char i;
    i = 31;
    while(i--) {
        recvTable[i] = 0;
    }
}

void wipeSendTable(void) {
    unsigned char i;
    i = 31;
    while(i--) {
        sendTable[i] = 0;
    }
}

void writeOpGet(void) {
    wipeSendTable();
    // 数据表清空
    sendTable[0] = 0x05;
    // 帧长度信息
    sendTable[1] = DEVICEID >> 8;
    sendTable[2] = (DEVICEID & 0x00ff);
    // 将设备ID写入数据表中
    sendTable[3] = 0x5f;
    sendTable[4] = 0x01;
}


void UART1_sendchar(unsigned char c) {
    while((UART1_SR & 0x80) == 0x00);
    UART1_DR = c;
}

void UART1_sendstr(unsigned char *pointer, unsigned char length) {
    unsigned char i = 0;
    while(length--) {
        UART1_sendchar( *(pointer + i) );
        i++;
    }
}

void readDataPower(void) {
    unsigned char i;
    wipeSendTable();
    for(i=4; i<32; i+=2) {
        hex2str(recvTable[(i/2-2)]);
        sendTable[i] = hex2strTable[0];
        sendTable[i+1] = hex2strTable[1];
        // 按位将数据表中的内容赋值到串口发送表中
    }
}

void writeOpPost(void) {
    sendTable[0] = 0x00;
    sendTable[1] = 0xfd;
    sendTable[2] = 0xfe;
    sendTable[3] = 0xff;
}

void delays(unsigned char second) {
    unsigned char i;
    unsigned int j, k;
    i = second;
    while(i--) {
        j = 300;
        while(j--) {
            k = 500;
            while(k--);
        }
    }
}
void ledBling(void) {
    PB_ODR = 0x00;
    delays(1);
    PB_ODR = 0x20;
}
void decidePlugOp(void) {
    if (recvTable[3] == 0x20) {
        initGPIO();
        switch(recvTable[4]) {
        case 0xa0:
            // P0^0 = recvTable[5] & 0x0f;
            PB_ODR = 0x20;
            // 
            break;
        case 0xa1:
            PB_ODR = 0x00;
            // P0^1 = recvTable[5] & 0x0f;
            break;
        }
    } else {
        // jump out
    }
}




void main() {
    Init_UART1();
    _asm("rim");//开中断，sim为关中断
    initGPIO();
    PB_ODR = 0xff;
    // 默认各个插口关闭
    while (1) {
        i = 0;
        // 一轮开始，串口接收指针归零
        ledBling();
        delays(29);
        readDataPower();
        writeOpPost();
        ledBling();
        UART1_sendstr(sendTable, 24);
        delays(9);
        writeOpGet();
        // 写“获取当前动作”指令到串口发送表
        
        ledBling();
        UART1_sendstr(sendTable, 5);
        // 发送数据，获取当前动作
        delays(8);
        // 适当的延时，给出响应的时间
        
        ledBling();
        decidePlugOp();
        // 根据传入的数据做出动作，若无有效数据则跳出
        delays(9);
        wipeRecvTable();
        // 清空串口接收表
        
        ledBling();
    }
}
//将收到的数据再发送出去
@far @interrupt void UART1_Recv_IRQHandler (void) {
    recvTable[i] = UART1_DR;
    i++;
}
