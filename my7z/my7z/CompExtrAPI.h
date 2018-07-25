#include <string>
#include <vector>
#include <iostream>
#include <map>


using namespace std;

class CompressExtract
{
public:
	CompressExtract();
	~CompressExtract();
	
	bool CompressFile(const wstring &archiveFileName, const wstring &fileNames, const wstring &load7zDllName);//archiveFile="xxx.7z",FileName="test1.txt test2.txt"����ļ�֮����'/'�ֿ�,���к�ѹ��
	bool ExtractFile(const wstring &archiveFileName, const wstring &outputPathName, const wstring &load7zDllName);//archiveFile="xxx.7z",��ѹ�ļ���ָ����Ŀ¼��outputPathName
	bool ShowArchivefileList(const wstring &archiveFileName, map<wstring, int> &archivefilelist, const wstring &load7zDllName);//չʾѹ���ļ�������ļ�,map��firstΪ�ļ�����secondΪ��С
private:
	bool Load7zDLL(const wstring &load7zDllName);
	
	void FileStringSepar(const wstring &fileNames);
	bool filePathExist(const wstring &wsfileName, vector<wstring>& filesList);
	bool DirectoryPathExit(const wstring &wsDirName, vector<wstring>&filesList);
	bool GetAllFiles();
	wstring FindCompressFilePath(const wstring  &filecompresspath, const wstring &filecompressFullpath);
	
private:

	HMODULE DllHandleName;
	vector<wstring>_filename;
	vector<wstring>_allfileList;
	vector<wstring>_compressfilePath;
};