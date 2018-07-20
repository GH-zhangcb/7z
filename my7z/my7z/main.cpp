#include "StdAfx.h"
#include "CompExtrAPI.h"
#include <iostream>
#include <string>
int main()
{
	CompressExtract aa,bb;
	//aa.CompressFile(L"D:\\1.7z", L"D:\\YoudaoNote");
	/*map<wstring,int>abc = {};
	aa.ShowArchivefileList(L"D:\\1.7z",abc);
	auto map_iter = abc.begin();
	while (map_iter!=abc.end())
	{
		wcout << map_iter->first << "  " << map_iter->second<<endl;
		map_iter++;
	}*/

	bb.ExtractFile(L"D:\\1.7z",L"C:\\abc\\");
	return 0;
}