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



 // HANDLE hMutex = NULL;//������
 // HANDLE hMutex_1 = NULL;//������
 // //�̺߳���
 // DWORD WINAPI Fun(LPVOID lpParamter)
 // {
 //     for (int i = 0; i < 20; i++)
 //    {
 //        //����һ����������
 //        WaitForSingleObject(hMutex, INFINITE);
 //        cout << "A Thread Fun Display!" << endl;
 //        Sleep(100);
 //        //�ͷŻ�������
 //        ReleaseMutex(hMutex);
 //    }
 //    return 0L;//��ʾ���ص���long�͵�0 
 //}
 //
 //int main()
 //{
 //    //����һ�����߳�
 //    HANDLE hThread = CreateThread(NULL, 0, Fun, NULL, 0, NULL);
 //    hMutex = CreateMutex(NULL, FALSE,L"screen");
	//// hMutex_1 = CreateMutex(NULL, FALSE, L"screen");
 //    //�ر��߳�
 //   CloseHandle(hThread);
 //   //���̵߳�ִ��·��
 //    for (int i = 0; i < 10; i++)
 //    {
 //        //������һ����������
 //       WaitForSingleObject(hMutex,INFINITE);
 //        cout << "Main Thread Display!" << endl;
	//	 Sleep(1000);
 //        //�ͷŻ�������
	//	 ReleaseMutex(hMutex);
 //    }
	// Sleep(5000);
	// cout <<"afd" << endl;
 //    return 0;
 //}