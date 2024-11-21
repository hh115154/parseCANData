/*
 * @Author: yanghongxu@bhap.com.cn
 * @Date: 2024-07-24 16:08:36
 * @LastEditors: yanghongxu@bhap.com.cn
 * @LastEditTime: 2024-11-21 17:23:50
 * @FilePath: /parseCANData/dbc_parse/my_interfaces.cpp
 * @Description: 
 * 
 * Copyright (c) 2024 by ${git_email}, All Rights Reserved. 
 */

#include <iostream>
#include <fstream>
#include<sstream>
#include<vector>

#include<cstring>

#include "my_interfaces.hpp"

void my_memcpy(void* dst,const void* src,size_t len,bool isBE)
{
    if(isBE){
        for(int i=0;i<len;i++){
            ((uint8_t*)dst)[i] = ((uint8_t*)src)[len-1-i];
        }
    }else{
        ::memcpy(dst,src,len);
    }
}


//parse physical signal value from can buf
int32_t getSignalValInCanBuf(const uint8_t* buf,uint16_t startBit,uint16_t len,bool isSigned,bool isBE)
{   
//we define myStrtBit as the least bit idx in the signal. so it is the nearest bit to the bit0.
    if(len > 32)
    {
        //now we only sppport parsing signals with len no more than 32bits
        // cout<< "DBC_PARSE_TAG,signal is too long with len = "<<len<<endl;
        return 0;
    }
    uint8_t startByte=0;
    uint8_t startBitInByte=0;
    uint16_t myStrtBit = 0;

    

    if(isBE){//big endian, the start is bit MSB
        myStrtBit = startBit/8*8+7-startBit%8;
    }else{//startBit is LSB
        myStrtBit = startBit;
    }

    startByte = myStrtBit / 8;
    startBitInByte = myStrtBit % 8;

    uint8_t sigBytesNr = (len + startBitInByte)/8 +1;//the number of bytes that the signal occupies,ceil
    
    int32_t val_res = 0;
    uint64_t mask = 0x00000000FFFFFFFF;
    uint64_t tmpWindow = 0;

    
    my_memcpy((uint8_t*)&tmpWindow,buf+startByte,sigBytesNr,isBE);


    mask>>=(32-len);
    tmpWindow>>=(sigBytesNr*8 -len - startBitInByte);


    val_res =(int32_t)(tmpWindow & mask);

    if (isSigned)
    {
        if (val_res & (1<<(len-1)))//if the sign bit is 1
        {
            val_res |= 0xFFFFFFFF;
        }
        
    }

    return val_res;
    
}

float getPhysicalValFromRawVal(const int32_t& rawVal,float factor,float offset)
{
    return rawVal*factor+offset;
}

int32_t usrDefChannelVal(const float& physicalVal)
{
    return (int32_t)(physicalVal*1000);
}

string getFileContent(const string& filename) {
    ifstream file(filename);
    if (!file.is_open()) {
        throw runtime_error("Unable to open file");
    }
    string content{istreambuf_iterator<char>(file), istreambuf_iterator<char>()};
    file.close();
    return content;
}

vector<uint16_t> parseNumbers(const string& str) {
    vector<uint16_t> numbers;
    istringstream iss(str);
    string token;
 
    while (getline(iss, token, ',')) {
        numbers.push_back(stoi(token));
    }
 
    return numbers;
}

vector<string> split_string_by_newline(const string& str) {
    vector<string> lines;
    size_t start = 0;
    size_t end = str.find('\n');
 
    while (end != string::npos) {
        lines.push_back(str.substr(start, end - start));
        start = end + 1; // 跳过换行符
        end = str.find('\n', start);
    }
 
    // 添加最后一行，它可能不以换行符结束
    lines.push_back(str.substr(start));
 
    return lines;
}


