#include "java/String.h"


#include <Windows.h>

namespace String
{

jstring fromUTF8(const std::string &str)
{
	if (str.empty()) return jstring();
	int len = MultiByteToWideChar(CP_UTF8, 0, str.data(), (int)str.size(), NULL, 0);
	std::u16string result(len, 0);
	MultiByteToWideChar(CP_UTF8, 0, str.data(), (int)str.size(), reinterpret_cast<LPWSTR>(&result[0]), len);
	return result;
}

std::string toUTF8(const jstring &str)
{
	if (str.empty()) return std::string();
	int len = WideCharToMultiByte(CP_UTF8, 0, reinterpret_cast<LPCWCH>(str.data()), (int)str.size(), NULL, 0, NULL, NULL);
	std::string result(len, 0);
	WideCharToMultiByte(CP_UTF8, 0, reinterpret_cast<LPCWCH>(str.data()), (int)str.size(), &result[0], len, NULL, NULL);
	return result;
}

template <typename T>
static jstring intToStringImpl(T v, int_t base)
{
	jstring out;
	bool negative = v < 0;
	if (negative)
		v = -v;

	while (v)
	{
		int_t digit = v % base;
		v /= base;
		out.insert(out.begin(), digit + (digit < 10 ? '0' : 'a' - 10));
	}

	if (out.empty())
		out.push_back('0');

	if (negative)
		out.insert(out.begin(), '-');

	return out;
}

jstring toString(int_t v, int_t base)
{
	return intToStringImpl<int_t>(v, base);
}

jstring toString(long_t v, int_t base)
{
	return intToStringImpl<long_t>(v, base);
}

jstring toString(uint_t v, int_t base)
{
	return intToStringImpl<uint_t>(v, base);
}

jstring toString(ulong_t v, int_t base)
{
	return intToStringImpl<ulong_t>(v, base);
}

jstring toString(float v)
{
	return fromUTF8(std::to_string(v));
}

jstring toString(double v)
{
	return fromUTF8(std::to_string(v));
}

}
