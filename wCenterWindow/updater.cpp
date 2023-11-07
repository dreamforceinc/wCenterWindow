// wCenterWindow
// updater.h
//
#include "globals.h"
#include "logger.h"
#include "updater.h"
#include "picojson.h"
#include <cwctype>

#define GITHUB_URL TEXT("api.github.com")
#define GITHUB_URI TEXT("/repos/dreamforceinc/wCenterWindow/releases/latest")

picojson::value json;
WCHAR errMessageBuffer[DBUFLEN]{ 0 };

struct Version
{
	UINT Major = 0;
	UINT Minor = 0;
	UINT Build = 0;
	UINT Revision = 0;
} verApp, verGh;

bool GetLatestRelease(const std::wstring& urn);
void FillVersionStructure(Version& ver, const std::wstring& str);
std::vector<std::wstring> Split(const std::wstring& s, wchar_t delim);
std::wstring ConvertUtf8ToWide(const std::string& str);

DWORD WINAPI Updater(LPVOID)
{
	LOG_TO_FILE(L"Entering the %s() function", TEXT(__FUNCTION__));

	if (!GetLatestRelease(GITHUB_URI))
	{
		LOG_TO_FILE(L"[UPDT] %s(%d): Failed getting releases!", TEXT(__FUNCTION__), __LINE__);
		StringCchPrintfW(errMessageBuffer, DBUFLEN, L"Failed getting releases!");
		MessageBoxW(NULL, errMessageBuffer, szTitle, MB_OK | MB_ICONERROR);
		return 101;
	}

	std::wstring j_tag_name, j_file_name, j_file_ext, j_file_url, j_page_url;
	int64_t j_file_size = 0;
	picojson::object obj, obj2;
	picojson::object::iterator it, it2;

	if (json.is<picojson::object>())
	{
		LOG_TO_FILE(L"[UPDT] %s(%d): Parsing JSON object", TEXT(__FUNCTION__), __LINE__);

		obj = json.get<picojson::object>();
		it = obj.find("message"), it2;
		if (it != obj.end())
		{
			std::string u = (*it).second.get<std::string>();
			LOG_TO_FILE(L"[UPDT] %s(%d): Error! The url is %s", TEXT(__FUNCTION__), __LINE__, u);
			return 102;
		}

		for (it = obj.begin(); it != obj.end(); it++)
		{
			if ((*it).first == "tag_name") j_tag_name = ConvertUtf8ToWide((*it).second.to_str());
			if ((*it).first == "html_url") j_page_url = ConvertUtf8ToWide((*it).second.to_str());
			if ((*it).first == "assets" && (*it).second.is<picojson::array>())
			{
				picojson::array a = (*it).second.get<picojson::array>();
				obj2 = a[0].get<picojson::object>();
				for (it2 = obj2.begin(); it2 != obj2.end(); it2++)
				{
					if ((*it2).first == "name") j_file_name = ConvertUtf8ToWide((*it2).second.to_str());
					if ((*it2).first == "browser_download_url") j_file_url = ConvertUtf8ToWide((*it2).second.to_str());
					if ((*it2).first == "size") j_file_size = static_cast<int64_t>((*it2).second.get<double>());
				}
			}
		}
	}
	else
	{
		LOG_TO_FILE(L"[UPDT] %s(%d): Error! Cannot recognize JSON object!", TEXT(__FUNCTION__), __LINE__);
		return 103;
	}

	size_t pos = 0;
	while (std::iswdigit(j_tag_name.at(pos)) == 0) pos++;
	std::wstring gh_version = j_tag_name.substr(pos);

	LOG_TO_FILE(L"[UPDT] %s(%d): AppVersion : %s", TEXT(__FUNCTION__), __LINE__, TEXT(VERSION_STR));
	LOG_TO_FILE(L"[UPDT] %s(%d): GitVersion : %s", TEXT(__FUNCTION__), __LINE__, gh_version.c_str());
	//LOG_TO_FILE(L"[UPDT] %s(%d):   FileName : %s", TEXT(__FUNCTION__), __LINE__, j_file_name.c_str());
	//LOG_TO_FILE(L"[UPDT] %s(%d):   FileSize : %d", TEXT(__FUNCTION__), __LINE__, j_file_size);
	//LOG_TO_FILE(L"[UPDT] %s(%d):   File Url : %s", TEXT(__FUNCTION__), __LINE__, j_file_url.c_str());
	//LOG_TO_FILE(L"[UPDT] %s(%d):   Page Url : %s", TEXT(__FUNCTION__), __LINE__, j_page_url.c_str());

	FillVersionStructure(verApp, TEXT(VERSION_STR));
	FillVersionStructure(verGh, gh_version);

	if ((verGh.Major > verApp.Major) || (verGh.Minor > verApp.Minor) || (verGh.Build > verApp.Build) || (verGh.Revision > verApp.Revision))
	{

		LOG_TO_FILE(L"[UPDT] %s(%d): An update is available!", TEXT(__FUNCTION__), __LINE__);

		if (IDYES == MessageBoxW(NULL, L"An update is available!\nDo you want to open the download page?", szTitle, MB_YESNO | MB_ICONINFORMATION))
		{
			LOG_TO_FILE(L"[UPDT] %s(%d): Opening download page by default browser", TEXT(__FUNCTION__), __LINE__);
			ShellExecuteW(NULL, L"open", j_page_url.c_str(), NULL, NULL, SW_SHOW);
		}
		else
		{
			LOG_TO_FILE(L"[UPDT] %s(%d): The user refused the update", TEXT(__FUNCTION__), __LINE__);
		}
	}
	else
	{
		LOG_TO_FILE(L"[UPDT] %s(%d): No updates is available", TEXT(__FUNCTION__), __LINE__);
	}

	LOG_TO_FILE(L"[UPDT] Exit from the %s() function", TEXT(__FUNCTION__));

	return 0;
}

bool GetLatestRelease(const std::wstring& urn)
{
	std::wstring user_agent = L"User-Agent: ";
	user_agent.append(szTitle);
	const std::wstring url = GITHUB_URL;
	bool ret = true;
	DWORD err = 0;

	HINTERNET hInternet = InternetOpenW(user_agent.c_str(), INTERNET_OPEN_TYPE_PRECONFIG, NULL, NULL, 0);
	if (hInternet != NULL)
	{
		HINTERNET hConnect = InternetConnectW(hInternet, url.c_str(), INTERNET_DEFAULT_HTTPS_PORT, L"", L"", INTERNET_SERVICE_HTTP, INTERNET_FLAG_SECURE, 0);
		if (hConnect != NULL)
		{
			HINTERNET hRequest = HttpOpenRequestW(hConnect, L"GET", urn.c_str(), NULL, NULL, NULL, INTERNET_FLAG_SECURE | INTERNET_FLAG_RELOAD | INTERNET_FLAG_NO_UI, 0);
			if (hRequest != NULL)
			{
				BOOL isSend = HttpSendRequestW(hRequest, NULL, 0, 0, 0);
				if (isSend)
				{
					char szData[1024]{ 0 };
					DWORD dwBytesRead = 0;
					std::string buffer;
					do
					{
						InternetReadFile(hRequest, szData, sizeof(szData), &dwBytesRead);
						buffer.append(szData, dwBytesRead);
					} while (dwBytesRead != 0);

					picojson::parse(json, buffer);
					std::string jerr = picojson::get_last_error();
					if (!jerr.empty())
					{
						std::cout << jerr << std::endl;
						ret = false;
					}

				}
				else
				{
					err = GetLastError();
					LOG_TO_FILE(L"[UPDT] %s(%d): HttpSendRequestW() error: %d", TEXT(__FUNCTION__), __LINE__, err);
					ret = false;
				}
			}
			else
			{
				err = GetLastError();
				LOG_TO_FILE(L"[UPDT] %s(%d): HttpOpenRequestW() error: %d", TEXT(__FUNCTION__), __LINE__, err);
				ret = false;
			}
			InternetCloseHandle(hRequest);
		}
		else
		{
			err = GetLastError();
			LOG_TO_FILE(L"[UPDT] %s(%d): InternetConnectW() error: %d", TEXT(__FUNCTION__), __LINE__, err);
			ret = false;
		}
		InternetCloseHandle(hConnect);
	}
	else
	{
		err = GetLastError();
		LOG_TO_FILE(L"[UPDT] %s(%d): InternetOpenW() error: %d", TEXT(__FUNCTION__), __LINE__, err);
		ret = false;
	}
	InternetCloseHandle(hInternet);
	return ret;
}

std::vector<std::wstring> Split(const std::wstring& s, wchar_t delim)
{
	std::vector<std::wstring> result;
	std::wstringstream ss(s);
	std::wstring item;
	while (getline(ss, item, delim)) result.push_back(item);
	return result;
}

void FillVersionStructure(Version& ver, const std::wstring& str)
{
	std::vector<std::wstring> v;
	v = Split(str, '.');
	if (v.size() < 4) v.push_back(L"0");
	ver.Major = std::stoul(v[0]);
	ver.Minor = std::stoul(v[1]);
	ver.Build = std::stoul(v[2]);
	ver.Revision = std::stoul(v[3]);
}

std::wstring ConvertUtf8ToWide(const std::string& str)
{
	int count = MultiByteToWideChar(CP_UTF8, 0, str.c_str(), str.length(), NULL, 0);
	std::wstring wstr(count, 0);
	MultiByteToWideChar(CP_UTF8, 0, str.c_str(), str.length(), &wstr[0], count);
	return wstr;
}
