#include "StdAfx.h"
#include "CompExtrAPI.h"
#include <iostream>
#include <string>
int main()
{
	CompressExtract aa;
	aa.CompressFile(L"D:\\1.7z", L"D:\\2/D:\\1.txt",L".\\7z.dll");
	/*map<wstring,int>abc = {};
	aa.ShowArchivefileList(L"D:\\2.7z",abc,L".\\7z.dll");
	auto map_iter = abc.begin();
	while (map_iter!=abc.end())
	{
		wcout << map_iter->first << "  " << map_iter->second<<endl;
		map_iter++;
	}
	*/
	//bb.ExtractFile(L"D:\\1.7z", L"C:\\abc", L".\\7z.dll");
	return 0;
}