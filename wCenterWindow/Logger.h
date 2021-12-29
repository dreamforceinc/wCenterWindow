#pragma once
#include <iostream>
#include <fstream>

extern std::wofstream logfile;
std::wstring GetTimeStamp();
std::wstring PrintTitle();

template <typename T1>
void diag_log(T1 arg1) {
	//std::wcout << GetTimeStamp() << arg1 << std::endl;
	logfile << GetTimeStamp() << arg1 << std::endl;
}

template <typename T1, typename T2>
void diag_log(T1 arg1, T2 arg2) {
	//std::wcout << GetTimeStamp() << arg1 << ' ' << arg2 << std::endl;
	logfile << GetTimeStamp() << arg1 << ' ' << arg2 << std::endl;
}

template <typename T1, typename T2, typename T3>
void diag_log(T1 arg1, T2 arg2, T3 arg3) {
	//std::wcout << GetTimeStamp() << arg1 << ' ' << arg2 << ' ' << arg3 << std::endl;
	logfile << GetTimeStamp() << arg1 << ' ' << arg2 << ' ' << arg3 << std::endl;
}

template <typename T1, typename T2, typename T3, typename T4>
void diag_log(T1 arg1, T2 arg2, T3 arg3, T4 arg4) {
	//std::wcout << GetTimeStamp() << arg1 << ' ' << arg2 << ' ' << arg3 << ' ' << arg4 << std::endl;
	//if (typeid(T4) == typeid(WCHAR)) {
	//	logfile << GetTimeStamp() << arg1 << ' ' << arg2 << ' ' << arg3 << ' ' << PrintTitle() << std::endl;
	//	return;
	//}
	logfile << GetTimeStamp() << arg1 << ' ' << arg2 << ' ' << arg3 << ' ' << arg4 << std::endl;
}

template <typename T1, typename T2, typename T3, typename T4, typename T5>
void diag_log(T1 arg1, T2 arg2, T3 arg3, T4 arg4, T5 arg5) {
	//std::wcout << GetTimeStamp() << arg1 << ' ' << arg2 << ' ' << arg3 << ' ' << arg4 << ' ' << arg5 << std::endl;
	logfile << GetTimeStamp() << arg1 << ' ' << arg2 << ' ' << arg3 << ' ' << arg4 << ' ' << arg5 << std::endl;
}

template <typename T1, typename T2, typename T3, typename T4, typename T5, typename T6>
void diag_log(T1 arg1, T2 arg2, T3 arg3, T4 arg4, T5 arg5, T6 arg6) {
	//std::wcout << GetTimeStamp() << arg1 << ' ' << arg2 << ' ' << arg3 << ' ' << arg4 << ' ' << arg5 << ' ' << arg6 << std::endl;
	logfile << GetTimeStamp() << arg1 << ' ' << arg2 << ' ' << arg3 << ' ' << arg4 << ' ' << arg5 << ' ' << arg6 << std::endl;
}

template <typename T1, typename T2, typename T3, typename T4, typename T5, typename T6, typename T7>
void diag_log(T1 arg1, T2 arg2, T3 arg3, T4 arg4, T5 arg5, T6 arg6, T7 arg7) {
	//std::wcout << GetTimeStamp() << arg1 << ' ' << arg2 << ' ' << arg3 << ' ' << arg4 << ' ' << arg5 << ' ' << arg6 << ' ' << arg7 << std::endl;
	logfile << GetTimeStamp() << arg1 << ' ' << arg2 << ' ' << arg3 << ' ' << arg4 << ' ' << arg5 << ' ' << arg6 << ' ' << arg7 << std::endl;
}

template <typename T1, typename T2, typename T3, typename T4, typename T5, typename T6, typename T7, typename T8>
void diag_log(T1 arg1, T2 arg2, T3 arg3, T4 arg4, T5 arg5, T6 arg6, T7 arg7, T8 arg8) {
	//std::wcout << GetTimeStamp() << arg1 << ' ' << arg2 << ' ' << arg3 << ' ' << arg4 << ' ' << arg5 << ' ' << arg6 << ' ' << arg7 << ' ' << arg8 << std::endl;
	logfile << GetTimeStamp() << arg1 << ' ' << arg2 << ' ' << arg3 << ' ' << arg4 << ' ' << arg5 << ' ' << arg6 << ' ' << arg7 << ' ' << arg8 << std::endl;
}

void OpenLogFile();
void CloseLogFile();
