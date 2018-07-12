#include <string>
#include <vector>
#include "../Common/iarchive.h"
#include "../Common/MyCom.h"
using namespace std;

class CompressExtract
{
public:
	CompressExtract();
	bool CompressFile(const wstring &archiveFileName, const wstring &fileNames);//archiveFile="xxx.7z",FileName="test1.txt test2.txt"����ļ�֮����'/'�ֿ�,���к�ѹ��
	bool ExtractFile(const wstring &archiveFileName);//archiveFile="xxx.7z",��ѹ����ǰ�ļ���,���ǵ�ǰ��֮ͬ�����ļ�
	bool ShowArchivefileList(const wstring &archiveFileName,vector<wstring> &archivefilenamelist);//չʾѹ���ļ�������ļ�
private:
	bool Load7zDLL();
	bool createObjectInit();
	void FileStringToChar(const wstring &fileNames);
	bool findOpenInit(const wstring &archiveFileName);
private:
	wstring load7zDllName;
	HMODULE DllHandleName;
	Func_CreateObject _createObjectFunc;
	CMyComPtr<IInArchive> _archive;
	vector<wstring>_filename;
};