/*
 * @Author: yanghongxu@bhap.com.cn
 * @Date: 2024-07-24 16:10:53
 * @LastEditors: yanghongxu@bhap.com.cn
 * @LastEditTime: 2024-07-24 16:11:46
 * @FilePath: /parseCANData/dbc_parse/my_interfaces.hpp
 * @Description: 
 * 
 * Copyright (c) 2024 by ${git_email}, All Rights Reserved. 
 */

#include <stdio.h>
#include "string.h"
#include <iostream>


using namespace std;

void my_memcpy(void* dst,const void* src,size_t len,bool isBE);
int32_t getSignalValInCanBuf(const uint8_t* buf,uint16_t startBit,uint16_t len,bool isSigned,bool isBE);
float getPhysicalValFromRawVal(const int32_t& rawVal,float factor,float offset);
int32_t usrDefChannelVal(const float& physicalVal);