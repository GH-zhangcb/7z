#include "StdAfx.h"

#include<iostream>
#include<windows.h>
#include<shlwapi.h>
#include<vector>
#include <io.h>
#include <initguid.h>
#include "../Common/MyString.h"
#include "../Common/StringConvert.h"
#include "../Common/MyCom.h"
#include "../Common/iarchive.h"
#include "../Common/FileStreams.h"
//#include "../Windows/NtCheck.h"
#include "../Windows/FileName.h"
#include "../Windows/PropVariant.h"
#include "../Windows/PropVariantConv.h"
#include "../Common/IntToString.h"
#include "../Windows/FileFind.h"
#include "../Windows/FileDir.h"
#include "CompExtrAPI.h"

#pragma comment(lib,"Shlwapi.lib")
using namespace std;
using namespace NWindows;
using namespace NFile;
using namespace NDir;

DEFINE_GUID(CLSID_CFormat7z,
	0x23170F69, 0x40C1, 0x278A, 0x10, 0x00, 0x00, 0x01, 0x10, 0x07, 0x00, 0x00);

#define CLSID_Format CLSID_CFormat7z

static FString StringToFString(const wchar_t *s)
{
	return us2fs(GetUnicodeString(s));
}

static HRESULT IsArchiveItemProp(IInArchive *archive, UInt32 index, PROPID propID, bool &result)
{
	NCOM::CPropVariant prop;
	RINOK(archive->GetProperty(index, propID, &prop));
	if (prop.vt == VT_BOOL)
		result = VARIANT_BOOLToBool(prop.boolVal);
	else if (prop.vt == VT_EMPTY)
		result = false;
	else
		return E_FAIL;
	return S_OK;
}

static HRESULT IsArchiveItemFolder(IInArchive *archive, UInt32 index, bool &result)
{
	return IsArchiveItemProp(archive, index, kpidIsDir, result);
}

////////////////////////////////////////////////////////////////////
class CArchiveOpenCallback :
	public IArchiveOpenCallback
{
public:
	STDMETHOD(SetTotal)(const UInt64 *files, const UInt64 *bytes);
	STDMETHOD(SetCompleted)(const UInt64 *files, const UInt64 *bytes);
	STDMETHOD(QueryInterface)(const IID & iid, void **ppvObject){ return 0; };
	ULONG __stdcall AddRef(){ return 0; };
	ULONG __stdcall Release(){ return 0; };
};
//定义OPEN
STDMETHODIMP CArchiveOpenCallback::SetTotal(const UInt64 * /* files */, const UInt64 * /* bytes */)
{
	return S_OK;
}

STDMETHODIMP CArchiveOpenCallback::SetCompleted(const UInt64 * /* files */, const UInt64 * /* bytes */)
{
	return S_OK;
}

static const wchar_t * const kEmptyFileAlias = L"[Content]";
static const char * const kTestingString = "Testing     ";
static const char * const kExtractingString = "Extracting  ";
static const char * const kSkippingString = "Skipping    ";
static const char * const kUnsupportedMethod = "Unsupported Method";
static const char * const kCRCFailed = "CRC Failed";
static const char * const kDataError = "Data Error";
static const char * const kUnavailableData = "Unavailable data";
static const char * const kUnexpectedEnd = "Unexpected end of data";
static const char * const kDataAfterEnd = "There are some data after the end of the payload data";
static const char * const kIsNotArc = "Is not archive";
static const char * const kHeadersError = "Headers Error";
//////////////////////////////////////////////////////////////
class CArchiveExtractCallback :
	public IArchiveExtractCallback
{
public:
	// IProgress
	STDMETHOD(SetTotal)(UInt64 size);
	STDMETHOD(SetCompleted)(const UInt64 *completeValue);

	// IArchiveExtractCallback
	STDMETHOD(GetStream)(UInt32 index, ISequentialOutStream **outStream, Int32 askExtractMode);
	STDMETHOD(PrepareOperation)(Int32 askExtractMode);
	STDMETHOD(SetOperationResult)(Int32 resultEOperationResult);

	STDMETHOD(QueryInterface)(const IID & iid, void **ppvObject){ return 0; };
	ULONG __stdcall AddRef(){ return 0; };
	ULONG __stdcall Release(){ return 0; };

private:
	CMyComPtr<IInArchive> _archiveHandler;
	FString _directoryPath;  // Output directory
	UString _filePath;       // name inside arcvhive
	FString _diskFilePath;   // full path to file on disk
	bool _extractMode;
	struct CProcessedFileInfo
	{
		FILETIME MTime;
		UInt32 Attrib;
		bool isDir;
		bool AttribDefined;
		bool MTimeDefined;
	} _processedFileInfo;

	COutFileStream *_outFileStreamSpec;
	CMyComPtr<ISequentialOutStream> _outFileStream;
public:
	void Init(IInArchive *archiveHandler, const FString &directoryPath);

	UInt64 NumErrors;
	bool PasswordIsDefined;
	UString Password;
	CArchiveExtractCallback() : PasswordIsDefined(false) {}
};

void CArchiveExtractCallback::Init(IInArchive *archiveHandler, const FString &directoryPath)
{
	NumErrors = 0;
	_archiveHandler = archiveHandler;
	_directoryPath = directoryPath;
	NName::NormalizeDirPathPrefix(_directoryPath);
}

UInt64 eFullSize;
UInt64 eCompleteSize;
STDMETHODIMP CArchiveExtractCallback::SetTotal(UInt64 size )
{
	eFullSize = size;
	return S_OK;
}

STDMETHODIMP CArchiveExtractCallback::SetCompleted(const UInt64 *completeValue)
{
	eCompleteSize=*completeValue;
	return S_OK;
}

void CompressExtract::eGetFullandCompleteSize()
{
	_eFullSize = eFullSize;
	_eCompleteSize = eCompleteSize;
}

STDMETHODIMP CArchiveExtractCallback::GetStream(UInt32 index,
	ISequentialOutStream **outStream, Int32 askExtractMode)
{
	*outStream = 0;
	_outFileStream.Release();

	{
		// Get Name
		NCOM::CPropVariant prop;
		RINOK(_archiveHandler->GetProperty(index, kpidPath, &prop));

		UString fullPath;
		if (prop.vt == VT_EMPTY)
			fullPath = kEmptyFileAlias;
		else
		{
			if (prop.vt != VT_BSTR)
				return E_FAIL;
			fullPath = prop.bstrVal;
		}
		_filePath = fullPath;
	}

	if (askExtractMode != NArchive::NExtract::NAskMode::kExtract)
		return S_OK;

	{
		// Get Attrib
		NCOM::CPropVariant prop;
		RINOK(_archiveHandler->GetProperty(index, kpidAttrib, &prop));
		if (prop.vt == VT_EMPTY)
		{
			_processedFileInfo.Attrib = 0;
			_processedFileInfo.AttribDefined = false;
		}
		else
		{
			if (prop.vt != VT_UI4)
				return E_FAIL;
			_processedFileInfo.Attrib = prop.ulVal;
			_processedFileInfo.AttribDefined = true;
		}
	}

	RINOK(IsArchiveItemFolder(_archiveHandler, index, _processedFileInfo.isDir));

	{
		// Get Modified Time
		NCOM::CPropVariant prop;
		RINOK(_archiveHandler->GetProperty(index, kpidMTime, &prop));
		_processedFileInfo.MTimeDefined = false;
		switch (prop.vt)
		{
		case VT_EMPTY:
			// _processedFileInfo.MTime = _utcMTimeDefault;
			break;
		case VT_FILETIME:
			_processedFileInfo.MTime = prop.filetime;
			_processedFileInfo.MTimeDefined = true;
			break;
		default:
			return E_FAIL;
		}

	}
	{
		// Get Size
		NCOM::CPropVariant prop;
		RINOK(_archiveHandler->GetProperty(index, kpidSize, &prop));
		UInt64 newFileSize;
		/* bool newFileSizeDefined = */ ConvertPropVariantToUInt64(prop, newFileSize);
	}

	//int slashPos_back = _filePath.ReverseFind_PathSepar();//得到文件所在的路径，返回路径的长度，不包括最后的‘\’
	//int slashPos_front = _filePath.ReturnUString_PathSeparFront();
	//int slashPos_length=_filePath.ReturnUStringLength();//字符串长度
	//FString fullProcessedPath = StringToFString(L"");//初始化路径
	//	// Create folders for file
	//if (slashPos_front >= 0)
	//	{
	//	//CreateComplexDir(_directoryPath + us2fs(_filePath.Left(slashPos_back)));
	//	fullProcessedPath = _directoryPath + us2fs(_filePath.Mid(slashPos_front + 1, slashPos_length - slashPos_front - 1));
	//	}
    //       else
	//	{
	//		fullProcessedPath = _directoryPath + us2fs(_filePath.Mid(slashPos_front + 1, slashPos_length));
   //        }

	FString fullProcessedPath = _directoryPath + us2fs(_filePath);
	_diskFilePath = fullProcessedPath;

	if (_processedFileInfo.isDir)//是为了建立输出路径，即建立文件夹，所以在压缩时需要将文件夹压缩进去(否则找不到路径)
	{
		CreateComplexDir(fullProcessedPath);
	}
	else
	{
		NFind::CFileInfo fi;
		if (fi.Find(fullProcessedPath))
		{
			if (!DeleteFileAlways(fullProcessedPath))
			{
				cout << "Can not delete output file" << endl; 
				return E_ABORT;
			}
		}

		_outFileStreamSpec = new COutFileStream;
		CMyComPtr<ISequentialOutStream> outStreamLoc(_outFileStreamSpec);
		if (!_outFileStreamSpec->Open(fullProcessedPath, CREATE_ALWAYS))
		{
			cout<<"Can not open output file"<<endl;
			return E_ABORT;
		}
		_outFileStream = outStreamLoc;
		*outStream = outStreamLoc.Detach();
	}
	return S_OK;
}

STDMETHODIMP CArchiveExtractCallback::PrepareOperation(Int32 askExtractMode)
{
	_extractMode = false;
	switch (askExtractMode)
	{
	case NArchive::NExtract::NAskMode::kExtract:  _extractMode = true; break;
	};
	switch (askExtractMode)
	{
	case NArchive::NExtract::NAskMode::kExtract: ; break;//cout<<kExtractingString<<endl
	case NArchive::NExtract::NAskMode::kTest:   break;//cout<<kExtractingString<<endl
	case NArchive::NExtract::NAskMode::kSkip:  break;//cout<<kExtractingString<<endl
	};
	
	//wcout<<_filePath<<endl;输出解压文件名
	return S_OK;
}

STDMETHODIMP CArchiveExtractCallback::SetOperationResult(Int32 operationResult)
{
	switch (operationResult)
	{
	case NArchive::NExtract::NOperationResult::kOK:
		break;
	default:
	  {
			   NumErrors++;
			   cout<<"  :  "<<endl;
			   const char *s = NULL;
			   switch (operationResult)
			   {
			   case NArchive::NExtract::NOperationResult::kUnsupportedMethod:
				   s = kUnsupportedMethod;
				   break;
			   case NArchive::NExtract::NOperationResult::kCRCError:
				   s = kCRCFailed;
				   break;
			   case NArchive::NExtract::NOperationResult::kDataError:
				   s = kDataError;
				   break;
			   case NArchive::NExtract::NOperationResult::kUnavailable:
				   s = kUnavailableData;
				   break;
			   case NArchive::NExtract::NOperationResult::kUnexpectedEnd:
				   s = kUnexpectedEnd;
				   break;
			   case NArchive::NExtract::NOperationResult::kDataAfterEnd:
				   s = kDataAfterEnd;
				   break;
			   case NArchive::NExtract::NOperationResult::kIsNotArc:
				   s = kIsNotArc;
				   break;
			   case NArchive::NExtract::NOperationResult::kHeadersError:
				   s = kHeadersError;
				   break;
			   }
			   if (s)
			   {
				   cout<<"Error : "<<endl;
				   cout<<s<<endl;
			   }
			   else
			   {
				   char temp[16];
				   ConvertUInt32ToString(operationResult, temp);
				   cout<<"Error #"<<endl;
				   cout<<temp<<endl;
			   }
	  }
	}

	if (_outFileStream)
	{
		if (_processedFileInfo.MTimeDefined)
			_outFileStreamSpec->SetMTime(&_processedFileInfo.MTime);
		RINOK(_outFileStreamSpec->Close());
	}
	_outFileStream.Release();
	if (_extractMode && _processedFileInfo.AttribDefined)
		SetFileAttrib_PosixHighDetect(_diskFilePath, _processedFileInfo.Attrib);
	return S_OK;
}

/////////////////////////////////////////////////////////
//打包
struct CDirItem
{
	UInt64 Size;
	FILETIME CTime;
	FILETIME ATime;
	FILETIME MTime;
	UString Name;
	FString FullPath;
	UInt32 Attrib;

	bool isDir() const { return (Attrib & FILE_ATTRIBUTE_DIRECTORY) != 0; }
};//

class CArchiveUpdateCallback :
	public IArchiveUpdateCallback2
{
public:
		// IProgress
	STDMETHOD(SetTotal)(UInt64 size);
	STDMETHOD(SetCompleted)(const UInt64 *completeValue);

	// IUpdateCallback2
	STDMETHOD(GetUpdateItemInfo)(UInt32 index,
	Int32 *newData, Int32 *newProperties, UInt32 *indexInArchive);
	STDMETHOD(GetProperty)(UInt32 index, PROPID propID, PROPVARIANT *value);
	STDMETHOD(GetStream)(UInt32 index, ISequentialInStream **inStream);
	STDMETHOD(SetOperationResult)(Int32 operationResult);
	STDMETHOD(GetVolumeSize)(UInt32 index, UInt64 *size);
	STDMETHOD(GetVolumeStream)(UInt32 index, ISequentialOutStream **volumeStream);
	STDMETHOD(QueryInterface)(const IID & iid, void **ppvObject){ return 0; };
	ULONG __stdcall AddRef(){ return 0; };
	ULONG __stdcall Release(){ return 0; };

public:
	CRecordVector<UInt64> VolumesSizes;
	UString VolName;
	UString VolExt;

	FString DirPrefix;
	const CObjectVector<CDirItem> *DirItems;

	bool PasswordIsDefined;
	UString Password;
	bool AskPassword;
	bool m_NeedBeClosed;

	FStringVector FailedFiles;
	CRecordVector<HRESULT> FailedCodes;
	CArchiveUpdateCallback() : PasswordIsDefined(false), AskPassword(false), DirItems(0) {};

	~CArchiveUpdateCallback() { Finilize(); }
	HRESULT Finilize();

	void Init(const CObjectVector<CDirItem> *dirItems)
	{
		DirItems = dirItems;
		m_NeedBeClosed = false;
		FailedFiles.Clear();
		FailedCodes.Clear();
	}
};

UInt64 cFullSize;
STDMETHODIMP CArchiveUpdateCallback::SetTotal(UInt64 size)
{
	cFullSize = size;
	return S_OK;
}

UInt64 cCompleteSize;

STDMETHODIMP CArchiveUpdateCallback::SetCompleted(const UInt64 *completeValue )
{
	cCompleteSize = *completeValue;	
	return S_OK;
}

void CompressExtract::cGetFullandCompleteSize()
{
	_cFullSize = cFullSize;
	_cCompleteSize = cCompleteSize;
}
STDMETHODIMP CArchiveUpdateCallback::GetUpdateItemInfo(UInt32 /* index */,
	Int32 *newData, Int32 *newProperties, UInt32 *indexInArchive)
{
	if (newData)
		*newData = BoolToInt(true);
	if (newProperties)
		*newProperties = BoolToInt(true);
	if (indexInArchive)
		*indexInArchive = (UInt32)(Int32)-1;
	return S_OK;
}

STDMETHODIMP CArchiveUpdateCallback::GetProperty(UInt32 index, PROPID propID, PROPVARIANT *value)
{
	NCOM::CPropVariant prop;

	if (propID == kpidIsAnti)
	{
		prop = false;
		prop.Detach(value);
		return S_OK;
	}
	{
		const CDirItem &dirItem = (*DirItems)[index];
		switch (propID)
		{
		case kpidPath:  prop = dirItem.Name; break;
		case kpidIsDir:  prop = dirItem.isDir(); break;
		case kpidSize:  prop = dirItem.Size; break;
		case kpidAttrib:  prop = dirItem.Attrib; break;
		case kpidCTime:  prop = dirItem.CTime; break;
		case kpidATime:  prop = dirItem.ATime; break;
		case kpidMTime:  prop = dirItem.MTime; break;
		}
	}
	prop.Detach(value);
	return S_OK;
}

HRESULT CArchiveUpdateCallback::Finilize()
{
	if (m_NeedBeClosed)
	{
		m_NeedBeClosed = false;
	}
	return S_OK;
}

static void GetStream2(const wchar_t *name)
{
	cout<<"Compressing"<<endl;
	if (name[0] == 0)
		name = kEmptyFileAlias;
	//wcout<<name<<endl;
}

STDMETHODIMP CArchiveUpdateCallback::GetStream(UInt32 index, ISequentialInStream **inStream)
{
	RINOK(Finilize());

	const CDirItem &dirItem = (*DirItems)[index];
	//GetStream2(dirItem.Name);输出解压名

	if (dirItem.isDir())
		return S_OK;

	{
		CInFileStream *inStreamSpec = new CInFileStream;
		CMyComPtr<ISequentialInStream> inStreamLoc(inStreamSpec);
		FString path = DirPrefix + dirItem.FullPath;
		if (!inStreamSpec->Open(path))
		{
			DWORD sysError = ::GetLastError();
			FailedCodes.Add(sysError);
			FailedFiles.Add(path);
			// if (systemError == ERROR_SHARING_VIOLATION)
			{
				cout<<"WARNING: can't open file"<<endl;
				// Print(NError::MyFormatMessageW(systemError));
				return S_FALSE;
			}
			// return sysError;
		}
		*inStream = inStreamLoc.Detach();
	}
	return S_OK;
}

STDMETHODIMP CArchiveUpdateCallback::SetOperationResult(Int32 /* operationResult */)
{
	m_NeedBeClosed = true;
	return S_OK;
}

STDMETHODIMP CArchiveUpdateCallback::GetVolumeSize(UInt32 index, UInt64 *size)
{
	if (VolumesSizes.Size() == 0)
		return S_FALSE;
	if (index >= (UInt32)VolumesSizes.Size())
		index = VolumesSizes.Size() - 1;
	*size = VolumesSizes[index];
	return S_OK;
}

STDMETHODIMP CArchiveUpdateCallback::GetVolumeStream(UInt32 index, ISequentialOutStream **volumeStream)
{
	wchar_t temp[16];
	ConvertUInt32ToString(index + 1, temp);
	UString res = temp;
	while (res.Len() < 2)
		res.InsertAtFront(L'0');
	UString fileName = VolName;
	fileName += '.';
	fileName += res;
	fileName += VolExt;
	COutFileStream *streamSpec = new COutFileStream;
	CMyComPtr<ISequentialOutStream> streamLoc(streamSpec);
	if (!streamSpec->Create(us2fs(fileName), false))
		return ::GetLastError();
	*volumeStream = streamLoc.Detach();
	return S_OK;
}

///////////////////////////主函数////////////////////////
CompressExtract::CompressExtract()
{
	DllHandleName = NULL;
	_filename = {};
	_allfileList = {};
	_compressfilePath = {};
}

CompressExtract::~CompressExtract()
{
	if (DllHandleName != NULL)
	{
		FreeLibrary(DllHandleName);
		DllHandleName = NULL;
	}
}

bool CompressExtract::Load7zDLL(const wstring &load7zDllName)
{
	if (load7zDllName.empty())
		return false;
	//加载7z
	DllHandleName = LoadLibrary(load7zDllName.c_str());
	if (!DllHandleName)
	{
		cout << "load 7z.dll failed" << endl;
		return false;
	}
	return true;
}

bool CompressExtract::ExtractFile(const wstring &archiveFileName, const wstring &outputPathName, const wstring &load7zDllName)
{
	
	if (!Load7zDLL(load7zDllName))
		return false;
	if (archiveFileName.empty() ||outputPathName.empty())
		return false;
	//加载7z里面的函数
	Func_CreateObject createObjectFunc = (Func_CreateObject)GetProcAddress(DllHandleName, "CreateObject");
	if (!createObjectFunc)
	{
		cout << "load function: CreatObject failed" << endl;
		return false;
	}

	FString archiveName = StringToFString(archiveFileName.c_str());
	CMyComPtr<IInArchive> archive;
	if (createObjectFunc(&CLSID_Format, &IID_IInArchive, (void **)&archive) != S_OK)
	{
		cout << "Can not get class object" << endl;
		return false;
	}
	CInFileStream *fileSpec = new CInFileStream;
	CMyComPtr<IInStream> file = fileSpec;
	//找不到该文件
	if (!fileSpec->Open(archiveName))
	{
		cout << "Can not open archive file" << endl;
		return false;
	}
	{//open
		CArchiveOpenCallback *openCallbackSpec = new CArchiveOpenCallback;
		CMyComPtr<IArchiveOpenCallback> openCallback(openCallbackSpec);
		
		const UInt64 scanSize = 1 << 23;
		//文件虽说找到了，但格式不对，如.rar格式的
		if (archive->Open(file, &scanSize, openCallback) != S_OK)
		{
			cout << "Can not open file as archive" << endl;
			return false;
		}
	}
	   //uncompress
		CArchiveExtractCallback *extractCallbackSpec = new CArchiveExtractCallback;
		CMyComPtr<IArchiveExtractCallback> extractCallback(extractCallbackSpec);
		wstring aoutputPathName = outputPathName + L"\\";
		FString outputPath = StringToFString(aoutputPathName.c_str());//输出的路径
		extractCallbackSpec->Init(archive, outputPath);//outputPath必须以"\\"结尾
		extractCallbackSpec->PasswordIsDefined = false;
		HRESULT result = archive->Extract(NULL, (UInt32)(Int32)(-1), false, extractCallback);
		if (result != S_OK)
		{
			cout << "Extract Error" << endl;
			return false;
		}
	return true;
}

bool CompressExtract::ShowArchivefileList(const wstring &archiveFileName, map<wstring, int> &archivefilelist, const wstring &load7zDllName)
{
	if (!Load7zDLL(load7zDllName) || archiveFileName.empty())
		return false;
	//加载7z里面的函数
	Func_CreateObject createObjectFunc = (Func_CreateObject)GetProcAddress(DllHandleName, "CreateObject");
	if (!createObjectFunc)
	{
		cout << "load function: CreatObject failed" << endl;
		return false;
	}

	FString archiveName = StringToFString(archiveFileName.c_str());
	CMyComPtr<IInArchive> archive;
	if (createObjectFunc(&CLSID_Format, &IID_IInArchive, (void **)&archive) != S_OK)
	{
		cout << "Can not get class object" << endl;
		return false;
	}
	CInFileStream *fileSpec = new CInFileStream;
	CMyComPtr<IInStream> file = fileSpec;
	//找不到改文件
	if (!fileSpec->Open(archiveName))
	{
		cout << "Can not open archive file" << endl;
		return false;
	}
	{//open
		CArchiveOpenCallback *openCallbackSpec = new CArchiveOpenCallback;
		CMyComPtr<IArchiveOpenCallback> openCallback(openCallbackSpec);
		
		const UInt64 scanSize = 1 << 23;
		//文件虽说找到了，但格式不对，如.rar格式的
		if (archive->Open(file, &scanSize, openCallback) != S_OK)
		{
			cout << "Can not open file as archive" << endl;
			return false;
		}
	}
	UInt32 numItems = 0;
	archive->GetNumberOfItems(&numItems);
	for (UInt32 i = 0; i < numItems; i++)
	{
		// Get uncompressed size of file
		NCOM::CPropVariant prop;
		archive->GetProperty(i, kpidSize, &prop);
		char s[32];//之前是char
		ConvertPropVariantToShortString(prop, s);//s为文件的大小	
		// Get name of file
		//NCOM::CPropVariant prop;
		archive->GetProperty(i, kpidPath, &prop);
		if (prop.vt == VT_BSTR)
		{
			archivefilelist.insert(make_pair(prop.bstrVal,atoi(s)));
		}
		else if (prop.vt != VT_EMPTY)
			cout << "ERROR!" << endl;  
	}
	return true;
}

void CompressExtract::FileStringSepar(const wstring &fileNames)
{
	if (fileNames.empty())
		return ;
	//取出文件
	vector<wstring>filename;
	int start = 0;
	wstring cstr = L"";
	for (int i = 0; (size_t)i < fileNames.size(); i++)
	{
		if (fileNames[i] == '|')//以'/'分开，最后一个以长度分开
		{
			wstring cstr = fileNames.substr(start, i - start);
			filename.push_back(cstr);
			start = i + 1;
		}
		else if (i == fileNames.size() - 1)//最后一个
		{
			wstring cstr = fileNames.substr(start, i - start + 1);
			filename.push_back(cstr);
		}
	}
	_filename = filename;
}

bool CompressExtract::filePathExist(const wstring &wsfileName, vector<wstring>& filesList)//wsfileName可以用通配符(可以是某一类文件)
{
	if (wsfileName.empty())
		return false;
	WIN32_FIND_DATA fileinfo;
	HANDLE hFile = FindFirstFile(wsfileName.c_str(), &fileinfo);
	if (hFile == INVALID_HANDLE_VALUE)
	{
		cout << "file not existed" << endl;
		return false;
	}
	else
	{
		do
		{
			filesList.push_back(wsfileName);
		} while (FindNextFile(hFile, &fileinfo));
	}
	if (!FindClose(hFile))
	{
		wcout << "close handle failed" << endl;
	}
	return true;
}

bool CompressExtract::DirectoryPathExit(const wstring &wsDirName, vector<wstring>&filesList)
{
	if (wsDirName.empty())
		return false;
	WIN32_FIND_DATA fileinfo;
	wstring sDirName_append = wsDirName + L"\\*";
	HANDLE hFile = FindFirstFile(sDirName_append.c_str(), &fileinfo);
	if (hFile == INVALID_HANDLE_VALUE)
	{
		cout << "can't match path" << endl;
		return false;
	}
	do
	{
		wstring newPath = wsDirName + L"\\" + fileinfo.cFileName;
		if (fileinfo.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)//目录
		{
			if ((StrCmpW(fileinfo.cFileName, L".") != 0) && (StrCmpW(fileinfo.cFileName, L"..") != 0))
			{
				DirectoryPathExit(newPath, filesList);
			}
		}
		else
		{
			filesList.push_back(newPath);
		}
	} while (FindNextFile(hFile, &fileinfo));
	//需将文件夹压进去
	filesList.push_back(wsDirName);	
	if (!FindClose(hFile))
	{
		wcout << "close handle failed" << endl;
	}
	return true;
}

bool CompressExtract::GetAllFiles()
{
	size_t star = 0;
	for (int i = 0; (size_t)i <  _filename.size(); i++)
	{
		if (_filename[i].empty())
			return false;
		WIN32_FIND_DATA fileinfo;
		HANDLE hFile = FindFirstFile(_filename[i].c_str(), &fileinfo);
		if (fileinfo.dwFileAttributes &FILE_ATTRIBUTE_DIRECTORY)
			//得到文件夹下所有的文件
			DirectoryPathExit(_filename[i], _allfileList);	
		else
			filePathExist(_filename[i], _allfileList);
		//得到需压缩的文件路径
		for (; star < _allfileList.size(); star++)
		{
			wstring strtemp = FindCompressFilePath(_filename[i], _allfileList[star]);
			_compressfilePath.push_back(strtemp);
		}
	}
	return true;
}

wstring CompressExtract::FindCompressFilePath(const wstring  &filecompresspath, const wstring &filecompressFullpath)
{
	if (filecompresspath.empty() || filecompressFullpath.empty())
		return L"";
	size_t Fpoint=0;
	size_t Fpoint_2 = filecompresspath.rfind(L"\\");//取出文件名
	size_t Fpoint_1 = filecompresspath.rfind(L"/");
	Fpoint= Fpoint_2 > Fpoint ? Fpoint_1 : Fpoint_2;
	if (Fpoint != string::npos)
	{
		wstring temp = filecompresspath.substr(Fpoint + 1);
		Fpoint = filecompressFullpath.find(temp);
		return filecompressFullpath.substr(Fpoint);
	}
	return filecompressFullpath;
}

bool CompressExtract::CompressFile(const wstring &archiveFileName, const wstring &fileNames, const wstring &load7zDllName)
{

	if (archiveFileName.empty() || fileNames.empty())
		return false;

	if (!Load7zDLL(load7zDllName))
		return false;
	//加载7z里面的函数
	Func_CreateObject createObjectFunc = (Func_CreateObject)GetProcAddress(DllHandleName, "CreateObject");
	if (!createObjectFunc)
	{
		cout << "load function: CreatObject failed" << endl;
		return false;
	}

	FString archiveName = StringToFString(archiveFileName.c_str());
	FileStringSepar(fileNames);
	if (!GetAllFiles())
	{
		cout << "file get failed" << endl;
		return false;
	}
	CObjectVector<CDirItem> dirItems;
	{
		size_t i;
		for (i = 0; i < _allfileList.size(); i++)
		{
			CDirItem di;
			FString name = StringToFString(_allfileList[i].c_str());//文件名
			NFind::CFileInfo fi;
			if (!fi.Find(name))
			{
				wcout << "Can't find file:" << name << endl;
				return false;
			}
			di.Attrib = fi.Attrib;
			di.Size = fi.Size;
			di.CTime = fi.CTime;
			di.ATime = fi.ATime;
			di.MTime = fi.MTime;
			di.Name = fs2us(StringToFString(_compressfilePath[i].c_str()));//注：name是文件的名称，会把name字符串全部压缩，所以name若带路径，则连路径也一块压缩进去
			di.FullPath = name;//fullPath是文件所在的全路径
			dirItems.Add(di);
		}
	}

	COutFileStream *outFileStreamSpec = new COutFileStream;
	CMyComPtr<IOutStream> outFileStream = outFileStreamSpec;
	if (!outFileStreamSpec->Create(archiveName, true))//创建打包文件，若把false改成true，则为若压缩文件存在就覆盖
	{
		cout << "can't create archive file" << endl;
		return false;
	}
	CMyComPtr<IOutArchive> outArchive;
	if (createObjectFunc(&CLSID_Format, &IID_IOutArchive, (void **)&outArchive) != S_OK)
	{
		cout << "Can not get class object" << endl;
		return false;
	}
     
	CArchiveUpdateCallback *updateCallbackSpec = new CArchiveUpdateCallback;
	CMyComPtr<IArchiveUpdateCallback2> updateCallback(updateCallbackSpec);
	updateCallbackSpec->Init(&dirItems);
	
	HRESULT result = outArchive->UpdateItems(outFileStream, dirItems.Size(), updateCallback);//压缩
	updateCallbackSpec->Finilize();
	if (result != S_OK)
	{
		cout << "Update Error" << endl;
		return false;
	}
	FOR_VECTOR(i, updateCallbackSpec->FailedFiles)
	{
		cout << endl;
		cout << "Error for file" << endl;
	}
	if (updateCallbackSpec->FailedFiles.Size() != 0)
		return false;
	return true;
}