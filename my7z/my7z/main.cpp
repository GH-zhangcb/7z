#include "StdAfx.h"
#include "CompExtrAPI.h"
#include <iostream>
#include <string>
int main()
{
	CompressExtract aa,bb;
	//aa.CompressFile(L"D:\\1.7z", L"1.txt");
	//vector<wstring>abc = {};
	//aa.ShowArchivefileList(L"D:\\achive11111111.7z",abc);
    //for (int i = 0; (size_t)i < abc.size(); i++)
	//	wcout << abc[i] << "dfa"<<endl;
	bb.ExtractFile(L"D:\\1.7z",L"C:\\abc\\");
	return 0;
}