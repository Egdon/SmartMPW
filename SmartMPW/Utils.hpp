//
// @author   liyan
// @contact  lyan_dut@outlook.com
//
#pragma once

#include <string>
#include <ctime>
#include <sstream>
#include <iomanip>

namespace utils {

	using namespace std;

	class Date {
	public:
		// ���ر�ʾ���ڸ�ʽ���ַ�����������
		static const string to_short_str() {
			ostringstream os;
			time_t now = time(0);
			os << put_time(localtime(&now), "%y%m%d");
			return os.str();
		}
		// ���ر�ʾ���ڸ�ʽ���ַ�����������ʱ����
		static const string to_long_str() {
			ostringstream os;
			time_t now = time(0);
			//os << put_time(localtime(&now), "%y-%m-%e-%H_%M_%S");
			os << put_time(localtime(&now), "%y%m%d%H%M%S");
			return os.str();
		}
	};

}
