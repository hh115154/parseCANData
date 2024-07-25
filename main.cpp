/*
 * @Author: yanghongxu@bhap.com.cn
 * @Date: 2024-07-23 14:18:29
 * @LastEditors: yanghongxu@bhap.com.cn
 * @LastEditTime: 2024-07-25 13:58:31
 * @FilePath: /parseCANData/main.cpp
 * @Description: 
 * 
 * Copyright (c) 2024 by ${git_email}, All Rights Reserved. 
 */
#include <stdio.h>
#include<string>
#include<iostream>
#include<fstream>
#include<vector>
#include"string.h"
#include "database.hpp"
#include "my_interfaces.hpp"

using namespace std;
using namespace AS::CAN::DbcLoader;
string log_file = "/home/hh/桌面/parseCANData/ql_log.txt";
string dbc_file = "/home/hh/桌面/parseCANData/TBX_B41V_EP_20231221.dbc";
string outp_folder = "/home/hh/桌面/parseCANData/";
string outp_logfile_name = "parsed_log.txt";

void parse_dbc(string dbc_file, unordered_map<unsigned int, const Message *>& messages);
void parse_logfile(string log_file);
vector<uint8_t> parseNumbers(const string& str);


unordered_map<unsigned int, const Message *> msg_inDBC;

int main(int argc, char* argv[]){
	if (argc>1)
	{
		if (3 == argc)
		{
			log_file = argv[1];
			dbc_file = argv[2];
			cout<<"log_file is "<<log_file<<endl;
			cout<<"dbc_file is "<<dbc_file<<endl;

		}else{
			cout<<"argc is "<<argc<<endl;
			for (size_t i = 0; i < argc; i++)
			{
				cout<<"argv["<<i<<"] is "<<argv[i]<<endl;
			}
		}
		
		
	}

	Database dbc(dbc_file);
	msg_inDBC = dbc.getMessages();
	
	parse_logfile(log_file);
	return 0;
}

void parse_logfile(string log_file){
	ofstream outf;//文件写操作 
    ifstream inf;//文件读操作
	string s,tmp;
	inf.open(log_file);

	ofstream outfile(outp_logfile_name);
	streambuf* default_bkp = cout.rdbuf(outfile.rdbuf());
    while (getline(inf, s))      
    {    
        int idx=s.find("\"");  //从每行的第一个分号位置开始截取
        int num=s.find_last_of(",");//从每行的最后一个逗号位置开结束
        // cout<<s<<endl;
        if(idx != -1 && num != -1)
        {
            tmp = s.substr(idx+1,num-idx-1);
            vector<uint8_t> numbers = parseNumbers(tmp);//将数字字符串提取成数字

			uint16_t msgID= numbers[1];
			msgID = msgID<<8;
			msgID = msgID + numbers[0];

			int dlc = numbers[2];//第三个字节是DLC



			// 然后都是数据
			cout<<endl<<"MSG ID =0x"<<hex<<msgID<< endl;
			// cout<<"MSG ID = 0x"<<hex<<msgID<< ", DLC = "<<dec<<dlc<<", buf = ";
			// for (size_t i = 0; i < dlc; i++)
			// {
			// 	cout<<"0x"<<hex<<numbers[i+3]<<",";
			// }
			// cout<<endl;



			if (msg_inDBC.find(msgID)!=msg_inDBC.end())
			{
				unordered_map<std::string, const Signal *> sigs = msg_inDBC[msgID]->getSignals();
				cout<<"signals counter in this Frame = "<<dec<<sigs.size()<<endl;
				for (auto sig:sigs)
				{
					uint32_t rawVal =  getSignalValInCanBuf(numbers.data()+3,
								sig.second->getStartBit(),
								sig.second->getLength(),
								sig.second->isSigned(),
								sig.second->getEndianness() == AS::CAN::DbcLoader::Order::BE);
					float physicalVal = getPhysicalValFromRawVal(rawVal,
								sig.second->getFactor(),
								sig.second->getOffset());
					cout.precision(5);
					cout.width(6);
					cout<<sig.second->getName()<<" = "<< physicalVal<<endl;
				}
			}
			else
			{
				//parsed msg per 1s
				cout<<s<<endl;
			}

        }
    }
	cout.rdbuf(default_bkp);
	outfile.close();
	inf.close();

}

vector<uint8_t> parseNumbers(const string& str) {
    vector<uint8_t> numbers;
    istringstream iss(str);
    string token;
 
    while (getline(iss, token, ',')) {
        numbers.push_back(stoi(token));
    }
 
    return numbers;
}

void unpack_zipfile(const string& zip_file){
	//解压zip文件
	
}

void parse_dbc(string dbc_file, unordered_map<unsigned int, const Message *>& messages)
{

	uint32_t signal_num = 0;
	for(auto msg:messages)
	{
		cout<<"DBC_FILE Parse, MSG id is 0x" <<hex<< msg.first<<endl;
		for(auto signal:msg.second->getSignals())
		{
			signal_num++;
			cout<<"DBC_FILE Parse signal name is " << signal.second->getName()<<endl;

		}
	}
}
