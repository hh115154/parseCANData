/*
 * @Author: yanghongxu@bhap.com.cn
 * @Date: 2024-07-23 14:18:29
 * @LastEditors: yanghongxu@bhap.com.cn
 * @LastEditTime: 2024-08-01 16:35:34
 * @FilePath: /parseCANData/main.cpp
 * @Description: 
 * 
 * Copyright (c) 2024 by ${git_email}, All Rights Reserved. 
 */
#include <stdio.h>
#include<string>
#include<iostream>
#include <iomanip>
#include<chrono>
#include <iomanip>
#include<fstream>
#include<vector>
#include"string.h"
#include "database.hpp"
#include "my_interfaces.hpp"

using namespace std;
using namespace AS::CAN::DbcLoader;
#define PLATFORM_LINUX

#ifdef PLATFORM_LINUX
string log_file = "/home/hh/桌面/0716DBCLOg/qllog/ql_log.txt";
string dbc_file = "/home/hh/桌面/B41VTB_LogParser/local/TBX_B41V_EP_20231221.dbc";
string outp_logfile_name = "/home/hh/桌面/0716DBCLOg/qllog/parsed_log.txt";
#else
string log_file = "C:/Users/yangh/Desktop/0716DBCLOg/qllog/ql_log1.txt";
string dbc_file = "C:/Users/yangh/Desktop/B41VTB_LogParser/local/TBX_B41V_EP_20231221.dbc";
string outp_logfile_name = "C:/Users/yangh/Desktop/0716DBCLOg/qllog/parsed_log.txt";
#endif

void parse_dbc(string dbc_file, unordered_map<unsigned int, const Message *>& messages);
void parse_logfile(string log_file);
void parse_logfile_content(string fileContent);
void parse_aSingle_line(string s);



unordered_map<unsigned int, const Message *> msg_inDBC;

int main(int argc, char* argv[]){
	if (argc>1)
	{
		if (4 == argc)
		{
			log_file = argv[1];
			dbc_file = argv[2];
			outp_logfile_name = argv[3];
			// cout<<"excuting process is "<< argv[0]<<endl;
			// cout<<"org_log_file is "<<log_file<<endl;
			// cout<<"dbc_file is "<<dbc_file<<endl;
			cout<<"outp_logfile_name is "<<outp_logfile_name<<endl;

		}else{
			cout<<"argc is "<<argc<<endl;
			for (size_t i = 0; i < argc; i++)
			{
				cout<<"argv["<<i<<"] is "<<argv[i]<<endl;
			}
		}
		
		
	}


	/* code */
	Database dbc(dbc_file);
	msg_inDBC = dbc.getMessages();

	
	try
	{
		/* code */
		string strLogfile = getFileContent(log_file);
	}
	catch(const std::exception& e)
	{
		std::cerr << e.what() << '\n';
		cout<<"Error: get log file contnet faild!"<<endl;
		return 0;
	}
	

	ofstream outfile(outp_logfile_name);
	streambuf* default_bkp = cout.rdbuf(outfile.rdbuf());

	// parse_logfile_content(strLogfile);
	try
	{
		/* code */
		parse_logfile(log_file);
	}
	catch(const std::exception& e)
	{
		std::cerr << e.what() << '\n';
		cout<<"Error: parse log file failed!"<<endl;
		return 0;
	}
	

	cout.rdbuf(default_bkp);
	outfile.close();


	return 0;
}

void parse_logfile_content(string fileContent){
	vector<string> lines = split_string_by_newline(fileContent);
	for (string line:lines)
	{
		parse_aSingle_line(line);
	}
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

void parse_logfile(string log_file){
	ofstream outf;//文件写操作 
    ifstream inf;//文件读操作
	string s;
	inf.open(log_file);


    while (getline(inf, s)){
		parse_aSingle_line(s);
    }

	inf.close();

}

void gettm(long long timestamp)
{
    auto milli = timestamp + (long long)8 * 60 * 60 * 1000; //此处转化为东八区北京时间，如果是其它时区需要按需求修改
    auto mTime = std::chrono::milliseconds(milli);
    auto tp = std::chrono::time_point<std::chrono::system_clock, std::chrono::milliseconds>(mTime);
    auto tt = std::chrono::system_clock::to_time_t(tp);
    std::tm *now = gmtime(&tt);

	//打印时间字符串
	cout<<endl<<"GnssInfo : ";
	cout << std::put_time(now, "%Y-%m-%d %H:%M:%S");
	cout<<"."<<dec<<timestamp%1000<<endl;


}

#define GPS_INFO_NUM 6
const string gpsInfoKeys[GPS_INFO_NUM] = {"longitude","latitude","Altitude","bearing","speed","IPC1_N_OdoMeter"};	
void parse_gpsInfo(const string& orgLog,const string& timeStamp){
	string outpStr = timeStamp;
	vector<string> gpsInfoVals;

	int idx=orgLog.find("\"");  //从每行的第一个引号位置开始截取
	int num=orgLog.find_last_of(",");//从每行的最后一个逗号位置开结束
	if(idx != -1 && num != -1)
	{
		string tmp = orgLog.substr(idx+1,num-idx-1);
		istringstream iss(tmp);
		string token;

		getline(iss, token, ',');
		uint64_t time_stamp = stol(token);

		gettm(time_stamp);
		int siz = gpsInfoKeys->length();
		
		for (size_t i = 0; i < GPS_INFO_NUM; i++)//
		{
			getline(iss, token, ',');
			cout<<gpsInfoKeys[i]<<" = "<<token<<",";
		}
		

		cout<<endl;

	}	
	
}




void parse_aSingle_line(string s){
	string tmp;
	//获取s单位时间戳字符串
	string time_s_stamp_str;
	int idx_tstp=s.find(",");  //从每行的第一个引号位置开始截取
	const int time_s_stamp_len = 14;//sample:07-23 16:03:37
	time_s_stamp_str = s.substr(idx_tstp+1,time_s_stamp_len);


	//case1,normal CAN frame,case2,parsed msg per 1s
	if (s.length() > 500)
	{
		parse_gpsInfo(s,time_s_stamp_str);
		return;
	}
	
	
	int idx=s.find("\"");  //从每行的第一个引号位置开始截取
	int num=s.find_last_of(",");//从每行的最后一个逗号位置开结束
	// cout<<s<<endl;
	if(idx != -1 && num != -1)
	{

		tmp = s.substr(idx+1,num-idx-1);
		vector<uint16_t> num = parseNumbers(tmp);//将数字字符串提取成数字
		uint16_t ms_stamp = num[0];
		vector<uint8_t> numbers;
		
		for (size_t i = 1; i < num.size(); i++)
		{
			numbers.push_back(num[i]);
		}
		uint16_t msgID= numbers[1];
		msgID = msgID<<8;
		msgID = msgID + numbers[0];

		int dlc = numbers[2];//第三个字节是DLC,然后都是数据


		cout<<endl;
		cout<<time_s_stamp_str<<".";
		cout<<right << setw(3) << setfill('0')<<dec<<ms_stamp;

		static int tmpStmp = 0;
		if (tmpStmp != ms_stamp)
		{
			/* code */
			tmpStmp = ms_stamp;
		}
		

		cout<<", MSG ID =0x"<<hex<<msgID<< endl;
		// cout<<"MSG ID = 0x"<<hex<<msgID<< ", DLC = "<<dec<<dlc<<", buf = ";
		// for (size_t i = 0; i < dlc; i++)
		// {
		// 	cout<<"0x"<<hex<<numbers[i+3]<<",";
		// }
		// cout<<endl;



		if (msg_inDBC.find(msgID)!=msg_inDBC.end())
		{
			
			unordered_map<string, const Signal *> sigs = msg_inDBC[msgID]->getSignals();
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

	}else{
		cout<<"Error: parse a normal frame failed!"<<endl;
	}

}
