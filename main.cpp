/*
 * @Author: yanghongxu@bhap.com.cn
 * @Date: 2024-07-23 14:18:29
 * @LastEditors: yanghongxu@bhap.com.cn
 * @LastEditTime: 2024-11-21 22:54:50
 * @FilePath: /parseCANData/main.cpp
 * @Description: 
 * 
 * Copyright (c) 2024 by ${git_email}, All Rights Reserved. 
 */
#include<string>
#include<iostream>
#include <iomanip>
#include<chrono>
#include <iomanip>
#include<fstream>
#include<vector>
#include<algorithm>
#include "database.hpp"
#include "my_interfaces.hpp"


using namespace std;
using namespace AS::CAN::DbcLoader;
// #define PLATFORM_LINUX

#ifdef PLATFORM_LINUX
string log_file = "/home/hh/桌面/0716DBCLOg/qllog/ql_log.txt";
string dbc_file = "/home/hh/桌面/B41VTB_LogParser/local/TBX_B41V_EP_20231221.dbc";
string outp_logfile_name = "/home/hh/桌面/0716DBCLOg/qllog/parsed_log.csv";
#else
string log_file = "C:/Users/yangh/Desktop/0716DBCLOg/qllog/ql_log.txt";
string dbc_file = "C:/Users/yangh/Desktop/B41VTB_LogParser/local/TBX_B41V_EP_20231221.dbc";
string outp_logfile_name = "C:/Users/yangh/Desktop/0716DBCLOg/qllog/parsed_log.csv";
#endif

void parse_dbc_signals(unordered_map<unsigned int, const Message *>& messages,unordered_map<string,int>& signals_pos_map);
void print_title(unordered_map<unsigned int, const Message *>& messages);
void parse_logfile(string log_file);
void parse_logfile_content(string fileContent);
void parse_aSingle_line(string s);




unordered_map<unsigned int, const Message *> msg_inDBC;

string* strSigTitles = NULL;
string* strUnits = NULL;
string* sigValues = NULL;
uint16_t Signal_Num = 0;
const string CSV_SPLIT_NOTICE = ",";

#define GPS_INFO_NUM 9
const string gpsInfoKeys[GPS_INFO_NUM] = {"VIN","Date","Time","longitude","latitude","Altitude","bearing","speed","IPC1_N_OdoMeter"};
string dataLineHead[] = {"--","date","00:00:00","0","0","0","0","0","0"};

#define NUM_BUG_STR 2
const string bugStr[] = {"，",","};
const string replacedStr[] = {"-","_"};
const string initSignalValue = "null";
const string invalidSignalValue = "NULL";

/*
1, 原始日志每行一帧CAN 报文，dbc中一共几十帧报文，接收频率100ms
2，解析后的日志格式是首行为信号名，第二行是单位，以下行为几十帧报文的信号值。
3，每行日志除了can信号值以外，行首还有VIN，timestamp，GPS信息。
4, *目前关于文件结尾的处理是：一个读入的org日志文件结尾时，如果没有完成整行输出文件，仍然关闭当前输出文件。
5, *目前关于文件头的处理是，每个org日志文件的第一帧开始，到所有帧（或所有信号）都收集齐全后，就认为一整行输出文件完成。
   4，5的原则会导致最后一行文件可能不完整，但是这个问题不大，数据没有丢失,只影响时间戳对齐。由于MCU发送给nad时，不带时间戳，时间对齐本身精度就不大。
6, *保证每秒10行数据的逻辑放在python里实现，这里只保证每行数据的正确整性。
7, 原始报文的DLC最高位表示当前帧数据是否有效，1表示无效
*/
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
			// cout<<"outp_logfile_name is "<<outp_logfile_name<<endl;

		}else{
			cout<<"argc is "<<argc<<endl;
			for (size_t i = 0; i < argc; i++)
			{
				cout<<"argv["<<i<<"] is "<<argv[i]<<endl;
			}
		}
		
		
	}


	/* parse dbcfile with 3rd src*/
	Database dbc(dbc_file);
	msg_inDBC = dbc.getMessages();

	ofstream outfile(outp_logfile_name);//输出文件
	streambuf* default_bkp = cout.rdbuf(outfile.rdbuf());//重定向cout 到outputfile

	print_title(msg_inDBC);

	try
	{
		/* 主解析函数*/
		parse_logfile(log_file);
	}
	catch(const std::exception& e)
	{
		std::cerr << e.what() << '\n';
		// cout<<"Error: parse log file failed!"<<endl;
		return 0;
	}
	

	cout.rdbuf(default_bkp);//重定向cout 到默认输出
	outfile.close();


	return 0;
}

string repalce_bugStr(const string & strBug){
	string newStr= strBug;
	for(int i=0;i<NUM_BUG_STR;i++){
		size_t pos = newStr.find(bugStr[i]);
		if (pos != string::npos)
		{
			newStr.replace(pos,bugStr[i].size(),replacedStr[i]);
		}

	}
	return newStr;
}


//print 2 lines title and unit 
void print_title(unordered_map<unsigned int, const Message *>& messages){
	Signal_Num = 0;
	for (auto msg:messages)
	{
		Signal_Num+=msg.second->getSignals().size();
	}

	strSigTitles = new string[Signal_Num];
	strUnits = new string[Signal_Num];
	string unitLine = "";
	//构造文件头，首行头，非CAN信号;第二行 单位行
	for (size_t i = 0; i < GPS_INFO_NUM; i++)
	{
		cout<<gpsInfoKeys[i];
		cout<<CSV_SPLIT_NOTICE;

		unitLine += CSV_SPLIT_NOTICE;
	}


	for(auto msg:messages)
	{
		for(auto signal:msg.second->getSignals())
		{
			//找到信号名中含有 “,”的信号，将其替换,
			string sigTitle = repalce_bugStr(signal.second->getName());
			strSigTitles[signal.second->getSigId()] = sigTitle;
			strUnits[signal.second->getSigId()] = signal.second->getUnit();
		}
	}
	
	//print title line
	for (size_t i = 0; i < Signal_Num; i++)
	{
		cout<<strSigTitles[i]<<CSV_SPLIT_NOTICE;
	}
	cout<<endl;

	//print unit line
	cout<<unitLine;
	for (size_t i = 0; i < Signal_Num; i++)
	{
		cout<<strUnits[i]<<CSV_SPLIT_NOTICE;
	}
	cout<<endl;

	delete[] strSigTitles;
	strSigTitles = NULL;
	delete[] strUnits;
	strUnits = NULL;


}

//parse log file from tbox,input is file path
void parse_logfile(string log_file){
    ifstream inf;//文件读操作
	string s;
	inf.open(log_file);

	//逐行解析
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

}

	
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
		// uint64_t time_stamp = stol(token);

		// gettm(time_stamp);
		
		for (size_t i = 0; i < GPS_INFO_NUM-3; i++)//
		{
			getline(iss, token, ',');
			dataLineHead[i+3] = token;
			// cout<<gpsInfoKeys[i]<<" = "<<token<<",";
		}

		getline(iss, token, ',');//VIN
		dataLineHead[0] = token;

	}	
	
}

void parse_aSingle_line(string s){

	static uint16_t signalCntrParsed = 0;//已解析的信号数,如果等于Signal_Num,则输出一行文件
	string tmp;
	//获取s单位时间戳字符串
	int idx_tstp=s.find(",");  //从每行的第一个引号位置开始截取
	const int date_len = 5;//sample:07-23
	string date_str = s.substr(idx_tstp+1,date_len);
	const int time_s_stamp_len  =8;//sample 16:03:37
	string time_s_stamp_str = s.substr(idx_tstp+2+date_len,time_s_stamp_len);
	dataLineHead[1] = date_str;
	dataLineHead[2] = time_s_stamp_str;

	string ts_s_str = time_s_stamp_str.substr(6,2);//取秒,最后两位
	int ts_s_int = stoi(ts_s_str);
	//case1,normal CAN frame,case2,parsed msg per 1s
	if (s.length() > 500)
	{//长行数据，更新行头
		parse_gpsInfo(s,time_s_stamp_str);
		return;
		
	}

	if (0==signalCntrParsed)
	{//普通行CAN数据
		/* 初始化，数据值 */
		sigValues = new string[Signal_Num];
		for (size_t i = 0; i < Signal_Num; i++)
		{
			sigValues[i] = initSignalValue;
		}
		
	}
	
	int idx=s.find("\"");  //从每行的第一个引号位置开始截取
	int num=s.find_last_of(",");//从每行的最后一个逗号位置开结束,当前帧（行）数据个数
	if(idx != -1 && num != -1)
	{
		tmp = s.substr(idx+1,num-idx-1);
		vector<uint16_t> num = parseNumbers(tmp);//将数字字符串提取成数字
		vector<uint8_t> numbers;
		
		for (size_t i = 1; i < num.size(); i++)
		{
			numbers.push_back(num[i]);
		}
		uint16_t msgID= numbers[1];
		msgID = msgID<<8;
		msgID = msgID + numbers[0];

		int dlc = numbers[2];//第三个字节是DLC,然后都是数据
		
		bool bMsgValid = (dlc & 0x80) == 0;//最高位表示当前帧数据是否有效，1表示无效
		dlc &= 0x7F;//取低7位，表示数据长度

		
		//从dbc中找到msgID对应的msg
		if (msg_inDBC.find(msgID)!=msg_inDBC.end())
		{
			
			unordered_map<string, const Signal *> sigs = msg_inDBC[msgID]->getSignals();
			// cout<<"signals counter in this Frame = "<<dec<<sigs.size()<<endl;
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

				// string sigName = sig.second->getName();
				int posn = sig.second->getSigId();
				// cout<<to_string(physicalVal);
				if (sigValues[posn] == initSignalValue)
				{
					/* code */
					signalCntrParsed++;
				}

				if (bMsgValid)
				{
					sigValues[posn] = to_string(physicalVal);
				}else{
					sigValues[posn] = invalidSignalValue;
				}
				

				if (signalCntrParsed == Signal_Num)
				{
					if (sigValues)
					{
						//行头
						for (size_t i = 0; i < GPS_INFO_NUM; i++)
						{
							cout<<dataLineHead[i];
							cout<<CSV_SPLIT_NOTICE;
						}
						/* can data*/
						for (size_t i = 0; i < Signal_Num; i++)
						{						
							cout<<sigValues[i];//not a safety call
							cout<<CSV_SPLIT_NOTICE;
						}
					}
					cout<<endl;

					signalCntrParsed = 0;
					delete [] sigValues;
					sigValues = NULL;
					return;
				}
				
			}
		}
		else
		{
			signalCntrParsed = 0;
			cout<<endl;
		}

	}else{
		cout<<endl;
		signalCntrParsed = 0;
	}

}
