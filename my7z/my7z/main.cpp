#include "StdAfx.h"
#include "CompExtrAPI.h"
#include <iostream>
#include <string>
int main()
{
	CompressExtract aa,bb;

	
	aa.CompressFile(L"F:\\1.7z", L"D:\\itest\\1.txt");
	//vector<wstring>abc = {};
	//aa.ShowArchivefileList(L"D:\\achive11111111.7z",abc);
    //for (int i = 0; (size_t)i < abc.size(); i++)
	//	wcout << abc[i] << "dfa"<<endl;
	//bb.ExtractFile(L"F:\\AA\\1.7z");
	return 0;
}