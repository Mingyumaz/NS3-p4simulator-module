#include "ns3/helper.h"
#include <algorithm>
#include <iostream>
#include <math.h>
#include <vector>

namespace ns3 {

char *IntToStr(int num) {
  char *ss = new char[10];
  int pos = 0;

  if (num == 0)
    ss[0] = '0';
  while (num) {
    ss[pos++] = num % 10 + '0';
    num = num / 10;
  }
  char temp;
  for (int i = 0; i < pos / 2; i++) {
    temp = ss[i];
    ss[i] = ss[pos - 1 - i];
    ss[pos - 1 - i] = temp;
  }
  ss[pos] = '\0';
  return ss;
}

unsigned int StrToInt(const std::string &str) {

  unsigned int res = 0;
  if (str.find("0x") < str.size()) // 16
  {
    for (size_t i = 2; i < str.size(); i++) {
      if (str[i] >= '0' && str[i] <= '9') {
        res = res * 16 + str[i] - '0';
      } else {
        if (str[i] >= 'a' && str[i] <= 'f') {
          res = res * 16 + str[i] - 'a' + 10;
        } else {
          if (str[i] >= 'A' && str[i] <= 'F')
            res = res * 16 + str[i] - 'A' + 10;
          else {
            std::cout << "in P4Model::StrToInt, action data error!"
                      << std::endl;
          }
        }
      }
    }
  } else {
    if (str.find("0b") < str.size()) // 2
    {
      for (size_t i = 2; i < str.size(); i++) {
        if (str[i] >= '0' && str[i] <= '1') {
          res = res * 2 + str[i] - '0';
        } else {
          std::cout << "in P4Model::StrToInt, action data error!" << std::endl;
        }
      }
    } else // 10
    {
      for (size_t i = 0; i < str.size(); i++) {
        if (str[i] >= '0' && str[i] <= '9') {
          res = res * 10 + str[i] - '0';
        } else {
          std::cout << "in P4Model::StrToInt, action data error!" << std::endl;
        }
      }
    }
  }
  return res;
}

int HexcharToInt(char c) {

  int temp = 0;
  if (c >= '0' && c <= '9')
    temp = c - '0';
  else {
    if (c >= 'a' && c <= 'f')
      temp = c - 'a' + 10;
    else {
      if (c >= 'A' && c <= 'F')
        temp = c - 'A' + 10;
      else {
        std::cout << " Attention: HexcharToInt error" << std::endl;
      }
    }
  }
  return temp;
}

std::string HexstrToBytes(const std::string &str) {

  std::string hexStr;
  if (str.find("0x") < str.size()) {
    hexStr = str.substr(2);
  } else {
    hexStr = str;
  }
  std::string res;
  res.resize(hexStr.size() / 2);
  for (size_t i = 0, j = 0; i < hexStr.size(); i += 2, j++) {
    res[j] = HexcharToInt(hexStr[i]) * 16 + HexcharToInt(hexStr[i + 1]);
  }
  return res;
}

/*
std::string HexstrToBytes(const std::string& str, unsigned int bitWidth)
{
    std::string res;
    res.resize(ceil(double(bitWidth) / 8));
    std::string full_ip_res(str);
    for (size_t i = 0; i < res.size(); i++) {
        res[i] = full_ip_res[i];
    }
    return res;
}
*/

std::string HexstrToBytes(const std::string &str, unsigned int bitWidth) {

  std::string hexStr;
  if (str.find("0x") < str.size()) {
    hexStr = str.substr(2);
  } else {
    hexStr = str;
  }
  std::string res;
  res.resize(ceil(double(bitWidth) / 8));
  std::cout << res.size() << std::endl;
  for (size_t i = 0, j = 0, w = 0; i < hexStr.size(); i += 2, j++, w += 8) {
    if (w + 8 < bitWidth) {
      res[j] = HexcharToInt(hexStr[i]) * 16 + HexcharToInt(hexStr[i + 1]);
    } else {
      if (w + 4 >= bitWidth) {
        std::string binaryStr(4, '0');
        size_t k = 3;
        int value = HexcharToInt(hexStr[i]);
        while (value) {
          binaryStr[k--] = value % 2 + '0';
          value /= 2;
        }
        res[j] = 0;
        unsigned int leftLen = bitWidth - w;
        for (size_t t = 0; t < leftLen; t++) {
          res[j] = res[j] * 2 + binaryStr[t] - '0';
        }
        for (size_t t = leftLen; t < 4; t++)
          res[j] *= 2;
        res[j] *= 16;
      } else {
        res[j] = HexcharToInt(hexStr[i]) * 16;
        std::string binaryStr(4, '0');
        size_t k = 3;
        int value = HexcharToInt(hexStr[i + 1]);
        while (value) {
          binaryStr[k--] = value % 2 + '0';
          value /= 2;
        }
        int temp = 0;
        unsigned int leftLen = bitWidth - w - 4;
        for (size_t t = 0; t < leftLen; t++) {
          temp = temp * 2 + binaryStr[t] - '0';
        }
        for (size_t t = leftLen; t < 4; t++)
          temp *= 2;
        res[j] += temp;
      }
      break;
    }
  }
  return res;
}

std::string UintToString(unsigned int num) {
  std::string res;
  if (num == 0)
    res = "0";
  while (num) {
    res.insert(res.begin(), num % 10 + '0');
    num /= 10;
  }
  return res;
}
std::string Uint32ipToHex(unsigned int ip) {
  if (ip != 0) {
    std::string res("0x00000000");
    int k = 9;
    int tmp;
    while (ip) {
      tmp = ip % 16;
      if (tmp < 10)
        res[k] = tmp + '0';
      else
        res[k] = tmp - 10 + 'a';
      k--;
      ip /= 16;
    }
    return res;
  } else
    return std::string("0x00000000");
}
double StrToDouble(const std::string &str) {
  double integerRes = 0;
  double decimalRes = 0;
  size_t pos = str.find('.');
  if (pos >= str.size())
    pos = str.size();
  // handle integer part
  for (size_t i = 0; i < pos; i++)
    integerRes = integerRes * 10 + str[i] - '0';
  // handle decimal part
  for (size_t i = str.size() - 1; i > pos; i--)
    decimalRes = decimalRes * 0.1 + str[i] - '0';

  return integerRes + decimalRes * 0.1;
}

std::string IpStrToBytes(const std::string &str) {
  std::string res;
  int j = 0, temp = 0;
  res.resize(4); // ip with 4 parts.
  for (size_t i = 0; i < str.size(); i++) {
    if (str[i] == '.') {
      res[j] = temp;
      temp = 0;
      j++;
    } else {
      temp = temp * 10 + (str[i] - '0');
    }
  }

  res[3] = temp; // deal with the last number.
  return res;
}

std::string IpStrToBytes(const std::string &str, unsigned int bitWidth) {
  std::string res;
  res.resize(ceil(double(bitWidth) / 8));
  std::string full_ip_res = IpStrToBytes(str);
  for (size_t i = 0; i < res.size(); i++) {
    res[i] = full_ip_res[i];
  }
  return res;
}

std::string IntToBytes(std::string input_str, int bitwidth) {
  // transfer from Python <runtime_CLI.py> by handwork

  std::vector<unsigned int> byte_array;
  std::string res;
  unsigned int input_num = std::stoi(input_str); // assume decimal 10

  while (input_num > 0) {
    byte_array.push_back(input_num % 256);
    input_num = input_num / 256;
    bitwidth--;
  }
  if (bitwidth < 0) {
    std::cout << "UIn_BadParamError: too large parameter!" << std::endl;
  }
  while (bitwidth > 0) {
    byte_array.push_back(0);
    bitwidth--;
  }
  std::reverse(byte_array.begin(), byte_array.end());

  // copy to the res.
  res.resize(byte_array.size());
  for (int i = 0; i < int(byte_array.size()); i++) {
    res[i] = byte_array[i];
  }
  return res;
}

std::string ParseParam(std::string &input_str, unsigned int bitwidth) {
  if (bitwidth == 32) {
    return HexstrToBytes(input_str);
  } else if (bitwidth == 48) {
    // return macAddr_to_bytes(input_str);
  } else if (bitwidth == 128) {
    // return ipv6Addr_to_bytes(input_str);
  }

  // with int
  int bw = (bitwidth + 7) / 8;
  return IntToBytes(input_str, bw);
}

} // namespace ns3
