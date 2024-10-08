// Copyright (c) 2019 AutonomouStuff, LLC
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
// THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
// THE SOFTWARE.

#include "message.hpp"

#include <memory>
#include <sstream>
#include <string>
#include <vector>

#include <math.h>
#include<iostream>

namespace AS
{
namespace CAN
{
namespace DbcLoader
{

Message::Message(std::string && message_text)
  : transmitting_node_(BusNode("")),
    comment_(nullptr)
{
  dbc_text_ = std::move(message_text);
  // std::cout<<"dbc_text_ is "<<dbc_text_<<std::endl;
  parse();
  // std::cout<<"dbc_text_ dlc is "<<dlc_str<<std::endl;
}

Message::Message(
  unsigned int id,
  std::string && name,
  unsigned char dlc,
  BusNode && transmitting_node,
  std::vector<Signal> && signals)
  : id_(id),
    name_(name),
    dlc_(dlc),
    transmitting_node_(transmitting_node),
    comment_(nullptr)
{
  for (auto & signal : signals) {
    signals_.emplace(std::make_pair(signal.getName(), std::move(signal)));
  }

  generateText();
}

Message::Message(const Message & other)
  : id_(other.id_),
    name_(other.name_),
    dlc_(other.dlc_),
    transmitting_node_(other.transmitting_node_),
    signals_(other.signals_)
{
  if (other.comment_) {
    comment_ = std::make_unique<std::string>(*(other.comment_));
  } else {
    comment_ = nullptr;
  }
}

Message & Message::operator=(const Message & other)
{
  return *this = Message(other);
}

unsigned int Message::getId() const
{
  return id_;
}

std::string Message::getName() const
{
  return name_;
}

unsigned char Message::getDlc() const
{
  return dlc_;
}

unsigned char Message::getLength() const
{
  return dlcToLength(dlc_);
}

BusNode Message::getTransmittingNode() const
{
  return BusNode(transmitting_node_);
}

std::unordered_map<std::string, const Signal *> Message::getSignals() const
{
  std::unordered_map<std::string, const Signal *> sigs;

  for (auto & sig : signals_) {
    sigs[sig.first] = &(sig.second);
  }

  return sigs;
}

std::unordered_map<std::string, Signal> Message::getSignals_bhap() const
{
    return signals_;
}

const std::string * Message::getComment() const
{
  return comment_.get();
}

void Message::generateText()
{
  std::ostringstream output;

  output << "BO_ " << id_ << " ";
  output << name_ << ": ";
  output << dlc_ << " ";
  output << transmitting_node_.name_;
  output << std::endl;

  dbc_text_ = output.str();
}

void Message::parse()
{
  std::istringstream input(dbc_text_);

  input.ignore(4);
  input >> id_;
  input >> name_;
  /* BPB:将dlc改成字符串格式，保证可以解析到多位长度 */
  input >> dlc_str;
  input >> transmitting_node_.name_;
  dlc_ = atoi(dlc_str.c_str());
  // Remove colon after name
  name_ = name_.substr(0, name_.length() - 1);
}

unsigned char Message::dlcToLength(const unsigned char & dlc)
{
  return DLC_LENGTH[dlc];
}

MessageTranscoder::MessageTranscoder(Message * dbc_msg)
  : msg_def_(dbc_msg),
    data_()
{
  /* BPB:分配data_ payload内存 */
  data_.assign(Message::dlcToLength(dbc_msg->getDlc()), 0);

  for (auto sig = msg_def_->signals_.begin(); sig != msg_def_->signals_.end(); ++sig) {
    signal_xcoders_.emplace(sig->first, &(sig->second));
  }
}

const Message * MessageTranscoder::getMessageDef()
{
  return msg_def_;
}

void MessageTranscoder::decode(std::vector<uint8_t> raw_data, TranscodeError * err)
{
  data_ = std::move(raw_data);

  decodeRawData(err);
}

void MessageTranscoder::decodeRawData(TranscodeError * err)
{
  err = new TranscodeError(TranscodeErrorType::NONE, "");
  for(auto signal:msg_def_->getSignals())
  {
    float value = 0.F;
    bool signed_flag = signal.second->isSigned();
    int dlc = msg_def_->getDlc();
    int start_bit = signal.second->getStartBit();
    int bit_length = signal.second->getLength();
    float fac = signal.second->getFactor();
    float offset = signal.second->getOffset();

    int byte_pos = (start_bit)/8; //获取信号最低字节所在的字节位置
    int bit_pos = (start_bit)%8; //获取信号最低字节所在的bit位置
    // std::cout<<"start_bit is "<<start_bit<<std::endl;
    // std::cout<<"byte_pos is "<<byte_pos<<std::endl;
    // std::cout<<"bit_pos is "<<bit_pos<<std::endl;
    // std::cout<<"bit_length is "<<bit_length<<std::endl;
    // std::cout<<"dlc is "<<dlc<<std::endl;
    long long unsigned int value_ = 0;
    unsigned char* value_ptr = (unsigned char*)&value_;
    int num = ceil((bit_length + bit_pos)/8.f);//BPB:确定信号占有多少个字节数
    /* BPB:填充信号的payload,最大可解析的信号量长度为64位 */ 
    for(int i=0;i<num;i++)
    {
      *(value_ptr+i) = data_[byte_pos-i];
    }
    if(!signed_flag)//BPB:如果是无符号信号
    {
      value_ <<= (64 - bit_pos - bit_length);
      value_ >>= (64 - bit_length);
      value  = value_ * fac + offset;
    }
    else//有符号信号
    {
      /* 获取最高位：符号位 */
      long long unsigned value_buf = value_;
      value_buf <<= (64 - bit_pos- bit_length);
      value_buf >>= 63;
      if(value_buf == 1) //判断符号位是1
      {
        value_ <<= (64 - bit_pos- bit_length + 1);
        value_ >>= (64 - bit_length + 1);
        value = -(value_ * fac + offset);
      }
      else//判断符号位是0
      {
        value_ <<= (64 - bit_pos- bit_length);
        value_ >>= (64 - bit_length);
        value = value_ * fac + offset;
      }
    }
      //std::cout<<"signal value is "<<value<<std::endl;
  }

  // TODO(jwhitleyastuff): Do the thing.
}

std::vector<uint8_t> MessageTranscoder::encode(TranscodeError * err)
{
  return std::vector<uint8_t>(data_.begin(), data_.end());
}

}  // namespace DbcLoader
}  // namespace CAN
}  // namespace AS
