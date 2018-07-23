#include <string>
#include <vector>
#include <iostream>
#include <map>
#include "../Common/iarchive.h"
#include "../Common/MyCom.h"
using namespace std;

class CompressExtract
{
public:
	CompressExtract();
	~CompressExtract();
	
	bool CompressFile(const wstring &archiveFileName, const wstring &fileNames, const wstring &load7zDllName);//archiveFile="xxx.7z",FileName="test1.txt test2.txt"����ļ�֮����'/'�ֿ�,���к�ѹ��
	bool ExtractFile(const wstring &archiveFileName, const wstring &outputPathName, const wstring &load7zDllName);//archiveFile="xxx.7z",��ѹ�ļ���ָ����Ŀ¼��outputPathName��'\\'��β
	bool ShowArchivefileList(const wstring &archiveFileName, map<wstring, int> &archivefilelist, const wstring &load7zDllName);//չʾѹ���ļ�������ļ�,map��firstΪ�ļ�����secondΪ��С
private:
	bool Load7zDLL(const wstring &load7zDllName);
	bool createObjectInit();
	void FileStringSepar(const wstring &fileNames);
	bool findOpenInit(const wstring &archiveFileName);
	bool filePathExist(const wstring &wsfileName, vector<wstring>& filesList);
	bool DirectoryPathExit(const wstring &wsDirName, vector<wstring>&filesList);
	bool GetAllFiles();
	wstring FindCompressFilePath(const wstring  &filecompresspath, const wstring &filecompressFullpath);
	
private:

	HMODULE DllHandleName;
	Func_CreateObject _createObjectFunc;
	CMyComPtr<IInArchive> _archive;
	vector<wstring>_filename;
	vector<wstring>_allfileList;
	vector<wstring>_compressfilePath;
};