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
	bool CompressFile(const wstring &archiveFileName, const wstring &fileNames);//archiveFile="xxx.7z",FileName="test1.txt test2.txt"多个文件之间以'/'分开,若有和压缩
	bool ExtractFile(const wstring &archiveFileName,const wstring &outputPathName);//archiveFile="xxx.7z",解压文件到指定的目录，outputPathName以'\\'结尾
	bool ShowArchivefileList(const wstring &archiveFileName, map<wstring,int> &archivefilelist);//展示压缩文件里面的文件,map中first为文件名，second为大小
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