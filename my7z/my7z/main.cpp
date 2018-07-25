#include "StdAfx.h"
#include "CompExtrAPI.h"

#include <map>
int main()
{
	CompressExtract aa;
	//aa.CompressFile(L"D:\\1.7z",L"D:\\1",L".\\7z.dll");
	//aa.ExtractFile(L"D:\\1.7z", L"C:\\abc", L".\\7z.dll");
	map<wstring, int>map1;
	aa.ShowArchivefileList(L"D:\\1.7z", map1, L".\\7z.dll");
	auto map_iter=map1.begin();
	while (map_iter!=map1.end())
	{
		wcout << map_iter->first << "  " << map_iter->second<<endl;
		map_iter++;
	}
	return 0;
}