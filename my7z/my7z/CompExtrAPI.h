#include <string>
#include <vector>
#include <map>
#include "../Common/iarchive.h"
#include "../Common/MyCom.h"
using namespace std;

class CompressExtract
{
public:
	CompressExtract();
	bool CompressFile(const wstring &archiveFileName, const wstring &fileNames);//archiveFile="xxx.7z",FileName="test1.txt test2.txt"����ļ�֮����'/'�ֿ�,���к�ѹ��
	bool ExtractFile(const wstring &archiveFileName,const wstring &outputPathName);//archiveFile="xxx.7z",��ѹ�ļ���ָ����Ŀ¼��outputPathName��'\\'��β
	bool ShowArchivefileList(const wstring &archiveFileName, map<wstring,int> &archivefilelist);//չʾѹ���ļ�������ļ�,map��firstΪ�ļ�����secondΪ��С
private:
	bool Load7zDLL();
	bool createObjectInit();
	void FileStringToChar(const wstring &fileNames);
	bool findOpenInit(const wstring &archiveFileName);
	bool filePathExist(const wstring &wsfileName, vector<wstring>& filesList);
	bool DirectoryPathExit(const wstring &wsDirName, vector<wstring>&filesList);
	bool GetAllFiles();
	wstring string2wstring(const string &str);
	string wstring2string(const wstring &wstr);
private:
	wstring load7zDllName;
	HMODULE DllHandleName;
	Func_CreateObject _createObjectFunc;
	CMyComPtr<IInArchive> _archive;
	vector<wstring>_filename;
	vector<wstring>_allfileLise;
};