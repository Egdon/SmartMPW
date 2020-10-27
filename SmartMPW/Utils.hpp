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

	//class Date {
	//public:
	//	// ���ر�ʾ���ڸ�ʽ���ַ�����������
	//	static string to_short_str() {
	//		ostringstream os;
	//		time_t now = time(0);
	//		os << put_time(localtime(&now), "%y%m%d");
	//		return os.str();
	//	}
	//	// ���ر�ʾ���ڸ�ʽ���ַ�����������ʱ����
	//	static string to_long_str() {
	//		ostringstream os;
	//		time_t now = time(0);
	//		//os << put_time(localtime(&now), "%y-%m-%e-%H_%M_%S");
	//		os << put_time(localtime(&now), "%y%m%d%H%M%S");
	//		return os.str();
	//	}
	//};

	static void split_filename(const string &str, string &path, string &file) {
		size_t found = str.find_last_of("/\\");
		path = str.substr(0, found + 1);
		file = str.substr(found + 1);
	}
}
