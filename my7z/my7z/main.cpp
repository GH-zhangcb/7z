#include "StdAfx.h"
#include "CompExtrAPI.h"
#include <map>

#include <windows.h>


int main()
{
	
	
	//HANDLE hThread2 = CreateThread(NULL, 0, thread_complete, NULL, 0, NULL);
	// CloseHandle(hThread2);
	

	 CompressExtract aa;

	aa.CompressFile(L"D:\\1.7z", L"D:\\YoudaoNote", L".\\7z.dll");
	// aa.ExtractFile(L"D:/1.7z",L"C:/abc", L".\\7z.dll");

	return 0;
}



 // HANDLE hMutex = NULL;//互斥量
 // HANDLE hMutex_1 = NULL;//互斥量
 // //线程函数
 // DWORD WINAPI Fun(LPVOID lpParamter)
 // {
 //     for (int i = 0; i < 20; i++)
 //    {
 //        //请求一个互斥量锁
 //        WaitForSingleObject(hMutex, INFINITE);
 //        cout << "A Thread Fun Display!" << endl;
 //        Sleep(100);
 //        //释放互斥量锁
 //        ReleaseMutex(hMutex);
 //    }
 //    return 0L;//表示返回的是long型的0 
 //}
 //
 //int main()
 //{
 //    //创建一个子线程
 //    HANDLE hThread = CreateThread(NULL, 0, Fun, NULL, 0, NULL);
 //    hMutex = CreateMutex(NULL, FALSE,L"screen");
	//// hMutex_1 = CreateMutex(NULL, FALSE, L"screen");
 //    //关闭线程
 //   CloseHandle(hThread);
 //   //主线程的执行路径
 //    for (int i = 0; i < 10; i++)
 //    {
 //        //请求获得一个互斥量锁
 //       WaitForSingleObject(hMutex,INFINITE);
 //        cout << "Main Thread Display!" << endl;
	//	 Sleep(1000);
 //        //释放互斥量锁
	//	 ReleaseMutex(hMutex);
 //    }
	// Sleep(5000);
	// cout <<"afd" << endl;
 //    return 0;
 //}