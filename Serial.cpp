//
//  Serial.cpp
//  matSenser
//
//  Created by Naoyuki Kubota on 2018/11/28.
//  Copyright © 2018年 Naoyuki Kubota. All rights reserved.
//

#include "Serial.hpp"
#define BAUD_RATE B38400
Serial::Serial(char* dev, int baudRate, bool debug){
    dev_name = dev;
    baud_rate = baudRate;
    if(debug) return;
    
    while((df = open(dev_name, O_RDWR | O_NONBLOCK)) < 0){
        printf("シリアルポートを開けません\nerror code >> %d\ndevice name >> %s",df, dev_name);
        if(getchar() == 'q'){
            errorFlag = true;
            return;
        }
    }
    //シリアル通信の設定を行うための構造体を定義 termiosに関してはここ参照：　https://linuxjm.osdn.jp/html/LDP_man-pages/man3/termios.3.html
    struct termios tio;
    memset(&tio, 0, sizeof(tio));
    tio.c_cflag = CS8 | CLOCAL | CREAD | HUPCL;
    tio.c_cc[VTIME] = 0.1;                  /* special characters */
    tio.c_cc[VMIN] = 0;
    
    // ボーレートの設定
    cfsetispeed(&tio, baud_rate);
    cfsetospeed(&tio, baud_rate);
    // デバイスに反映
    tcsetattr(df, TCSANOW, &tio);
    
    //バッファとかの初期化 多分やらなくても平気だけど一応
    memset(buffer, '\0', BUFF_SIZE);
    sampling_time = 1e10;
    preClock = (double)clock();
}

Serial::Serial(char* fname){
    
}

Serial::~Serial(){
//    printf("\nClass: Serial が破棄されました\n");
//    printf("%s を確実に取り外してください\n",dev_name);
    close(df);
    
}

int Serial::readMat(){
    int len, i, j = 0, time = 0;
    char readData[30];
    bool readFlag = false;
    
    while(time < 100){
        while((len = (int) read(df, buffer, BUFF_SIZE)) == 0){}
        //IO エラー
        if(len < 0){
            fprintf(stderr, "IOError \n");
            exit(2);
        }
        for(i = 0; i < len; i++){
            if(buffer[i] == '\r' || buffer[i] == '\n'){
                
                if(readFlag) {
                    //printf("readMat finished: time count >> %d \n",time);
                    double tmp = preClock;
                    sampling_time = (double)((preClock = clock()) - tmp) / CLOCKS_PER_SEC ;
                    return atoi(readData);
                }
            }else if(readFlag && buffer[i] != ' '){
                readData[j] = buffer[i];
                printf("%c",readData[j]);
                j++;
            }else if(buffer[i] == ','){
                //カンマの後ろから読み取りを開始
                readFlag = true;
            }
        }
        printf("|");
        time++;
    }
    
    //time out
    fprintf(stderr, "Error: time out\n");
    return -1;
}

std::string Serial::test(){
    int len;
    memset(buffer, '\0', BUFF_SIZE);
    while((len = (int) read(df, buffer, BUFF_SIZE)) == 0){}
    //IO エラー
    if(len < 0){
        fprintf(stderr, "IOError \n");
        exit(2);
    }
    
    //if(len != 0)printf("%s",buffer);
    
    return buffer;
}

//format A____\r\n
int Serial::test2(int data[BUFF_SIZE]){
    int len, i=0, j=0, dataCount = 0;
    static char remaining[BUFF_SIZE];
    char tmp[50];
    memset(buffer, '\0', BUFF_SIZE);
    while((len = (int) read(df, buffer, BUFF_SIZE)) == 0){}
    //IO エラー
    if(len < 0){
        fprintf(stderr, "IOError \n");
        exit(2);
    }
    
    while ( i < len ) {
        if(buffer[i] == 'A'){
            break;
        }
        i++;
    }
    
    for(; i<len; i++){
        if(buffer[i] == '\r' || buffer[i] == 'A') {
            continue;
        }
        if(buffer[i] == '\n'){
            data[dataCount] = atoi(tmp);
            memset(tmp, '\0', 50);
            j = 0;
            dataCount++;
        }
        tmp[j] = buffer[i];
    }
    
    //if(len != 0)printf("%s",buffer);
    
    return dataCount;
}

//prolific ubs serial
int Serial::readNewMat(){
    int len, j=0, dataCount = 0;
    bool readFlag = false;
    char readData[4];
    int readNum = 0;
    
    
    while (1) {
        while((len = (int) read(df, buf_1byte, 1)) == 0){}
        //IO エラー
        if(len < 0){
            fprintf(stderr, "IOError \n");
            exit(2);
        }
        
        if(buf_1byte[0] >=48 && buf_1byte[0] <= 57){
            readData[j] = buf_1byte[0];
            j++;
        }else if(buf_1byte[0] == ','){
            readNum = atoi(readData);
            return readNum;
        }
        
        if(j >= 4) {
            
            do{
                while((len = (int) read(df, buffer, BUFF_SIZE)) == 0){}
            }while(buffer[0] != ',');
            return -1;
        }
    }
}

int Serial::readNewMat2(double data[500]){
    int len, j=0, i=0, dataCount = 0;
    char tmp;
    char readData[3];
    
    memset(buffer, '\0', BUFF_SIZE);
    while((len = (int) read(df, buffer, BUFF_SIZE)) == 0){}
    //IO エラー
    if(len < 0){
        fprintf(stderr, "IOError \n");
        exit(2);
    }
    //printf("buffer = \n%s\n",buffer);
    //最初の一つは読み飛ばす
    while(buffer[i] != ',' && i < len){
        i++;
    }
    i++;
    for(;i<len;i++){
        
        if(buffer[i] == ','){
            data[dataCount] = (double)atoi(readData);
            //printf("data = %d\n",data[dataCount]);
            dataCount++;
            memset(readData, '\0', 3);
            j=0;
        }else if(j>=3){
            while(buffer[i] != ',' && i < len){
                i++;
            }
            i++;
            memset(readData, '\0', 3);
            j=0;
        }else if(buffer[i] >=48 && buffer[i] <= 57){
            readData[j] = buffer[i];
            j++;
        }
        
    }
    
    return dataCount;
}

//拾い物　windowsのkbhitとだいたい同じ　ちょっと反応が悪い？
//参考；　http://tricky-code.net/mine/c/mc06linuxkbhit.php
int Serial::kbhit(void)
{
    struct termios oldt, newt;
    int ch;
    int oldf;
    
    tcgetattr(STDIN_FILENO, &oldt);
    newt = oldt;
    newt.c_lflag &= ~(ICANON | ECHO);
    tcsetattr(STDIN_FILENO, TCSANOW, &newt);
    oldf = fcntl(STDIN_FILENO, F_GETFL, 0);
    fcntl(STDIN_FILENO, F_SETFL, oldf | O_NONBLOCK);
    
    ch = getchar();
    
    tcsetattr(STDIN_FILENO, TCSANOW, &oldt);
    fcntl(STDIN_FILENO, F_SETFL, oldf);
    
    if (ch != EOF) {
        ungetc(ch, stdin);
        return 1;
    }
    
    return 0;
}

