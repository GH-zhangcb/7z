#include "StdAfx.h"

#include<iostream>
#include<windows.h>
#include<vector>
#include <io.h>
#include <initguid.h>
#include "../Common/MyString.h"
#include "../Common/StringConvert.h"
#include "../Common/MyCom.h"
#include "../Common/iarchive.h"
#include "../Common/FileStreams.h"
#include "../Windows/NtCheck.h"
#include "../Common/IPassword.h"
#include "../Windows/FileName.h"
#include "../Windows/PropVariant.h"
#include "../Windows/PropVariantConv.h"
#include "../Common/IntToString.h"
#include "../Windows/FileFind.h"
#include "../Windows/FileDir.h"
#include "CompExtrAPI.h"
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
	public IArchiveOpenCallback,
	public ICryptoGetTextPassword,
	public CMyUnknownImp
{
public:
	MY_UNKNOWN_IMP1(ICryptoGetTextPassword)

		STDMETHOD(SetTotal)(const UInt64 *files, const UInt64 *bytes);
	STDMETHOD(SetCompleted)(const UInt64 *files, const UInt64 *bytes);

	STDMETHOD(CryptoGetTextPassword)(BSTR *password);

	bool PasswordIsDefined;
	UString Password;

	CArchiveOpenCallback() : PasswordIsDefined(false) {}
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

STDMETHODIMP CArchiveOpenCallback::CryptoGetTextPassword(BSTR *password)
{
	if (!PasswordIsDefined)
	{
		// You can ask real password here from user
		// Password = GetPassword(OutStream);
		// PasswordIsDefined = true;
		cout<<"Password is not defined"<<endl;
		return E_ABORT;
	}
	return StringToBstr(Password, password);
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
	public IArchiveExtractCallback,
	public ICryptoGetTextPassword,
	public CMyUnknownImp
{
public:
	MY_UNKNOWN_IMP1(ICryptoGetTextPassword)

		// IProgress
		STDMETHOD(SetTotal)(UInt64 size);
	STDMETHOD(SetCompleted)(const UInt64 *completeValue);

	// IArchiveExtractCallback
	STDMETHOD(GetStream)(UInt32 index, ISequentialOutStream **outStream, Int32 askExtractMode);
	STDMETHOD(PrepareOperation)(Int32 askExtractMode);
	STDMETHOD(SetOperationResult)(Int32 resultEOperationResult);

	// ICryptoGetTextPassword
	STDMETHOD(CryptoGetTextPassword)(BSTR *aPassword);

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

//定义
void CArchiveExtractCallback::Init(IInArchive *archiveHandler, const FString &directoryPath)
{
	NumErrors = 0;
	_archiveHandler = archiveHandler;
	_directoryPath = directoryPath;
	NName::NormalizeDirPathPrefix(_directoryPath);
}

STDMETHODIMP CArchiveExtractCallback::SetTotal(UInt64 /* size */)
{
	return S_OK;
}

STDMETHODIMP CArchiveExtractCallback::SetCompleted(const UInt64 * /* completeValue */)
{
	return S_OK;
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
	int slashPos_front = _filePath.ReturnUString_PathSeparFront();
	int slashPos_length=_filePath.ReturnUStringLength();//字符串长度
	FString fullProcessedPath = StringToFString(L"");//初始化路径
		// Create folders for file
	if (slashPos_front >= 0)
		{
		//CreateComplexDir(_directoryPath + us2fs(_filePath.Left(slashPos_back)));
		fullProcessedPath = _directoryPath + us2fs(_filePath.Mid(slashPos_front + 1, slashPos_length - slashPos_front - 1));
		}
        else
		{
			fullProcessedPath = _directoryPath + us2fs(_filePath.Mid(slashPos_front + 1, slashPos_length));
         }

	//FString fullProcessedPath = _directoryPath + us2fs(_filePath);
	_diskFilePath = fullProcessedPath;

	if (_processedFileInfo.isDir)
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
	case NArchive::NExtract::NAskMode::kExtract: cout<<kExtractingString<<endl; break;
	case NArchive::NExtract::NAskMode::kTest:  cout<<kTestingString<<endl; break;
	case NArchive::NExtract::NAskMode::kSkip:  cout<<kSkippingString<<endl; break;
	};
	cout<<_filePath<<endl;
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
	cout<<endl;
	return S_OK;
}

STDMETHODIMP CArchiveExtractCallback::CryptoGetTextPassword(BSTR *password)
{
	if (!PasswordIsDefined)
	{
		// You can ask real password here from user
		// Password = GetPassword(OutStream);
		// PasswordIsDefined = true;
		cout<<"Password is not defined"<<endl;
		return E_ABORT;
	}
	return StringToBstr(Password, password);
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
	public IArchiveUpdateCallback2,
	public ICryptoGetTextPassword2,
	public CMyUnknownImp
{
public:
	MY_UNKNOWN_IMP2(IArchiveUpdateCallback2, ICryptoGetTextPassword2)

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

	STDMETHOD(CryptoGetTextPassword2)(Int32 *passwordIsDefined, BSTR *password);

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

STDMETHODIMP CArchiveUpdateCallback::SetTotal(UInt64 /* size */)
{
	return S_OK;
}

STDMETHODIMP CArchiveUpdateCallback::SetCompleted(const UInt64 * /* completeValue */)
{
	return S_OK;
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
		cout << endl;
		m_NeedBeClosed = false;
	}
	return S_OK;
}

static void GetStream2(const wchar_t *name)
{
	cout<<"Compressing"<<endl;
	if (name[0] == 0)
		name = kEmptyFileAlias;
	cout<<name<<endl;
}

STDMETHODIMP CArchiveUpdateCallback::GetStream(UInt32 index, ISequentialInStream **inStream)
{
	RINOK(Finilize());

	const CDirItem &dirItem = (*DirItems)[index];
	GetStream2(dirItem.Name);

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
				cout << endl;
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

STDMETHODIMP CArchiveUpdateCallback::CryptoGetTextPassword2(Int32 *passwordIsDefined, BSTR *password)
{
	if (!PasswordIsDefined)
	{
		if (AskPassword)
		{
			// You can ask real password here from user
			// Password = GetPassword(OutStream);
			// PasswordIsDefined = true;
			cout<<"Password is not defined"<<endl;
			return E_ABORT;
		}
	}
	*passwordIsDefined = BoolToInt(PasswordIsDefined);
	return StringToBstr(Password, password);
}

///////////////////////////主函数////////////////////////
CompressExtract::CompressExtract()
{
	load7zDllName = L".\\7z.dll";
	DllHandleName = NULL;
	_createObjectFunc = NULL;
	_archive = NULL;
	_filename = {};
	_allfileLise = {};
}

bool CompressExtract::Load7zDLL()
{
	if (load7zDllName.empty())
		return false;
	//加载7z
	HMODULE load7z = LoadLibrary(load7zDllName.c_str());
	DllHandleName = load7z;
	if (!load7z)
	{
		cout << "load 7z.dll failed" << endl;
		return false;
	}
	return true;
}

bool CompressExtract::createObjectInit()
{
	//加载7z里面的函数
	Func_CreateObject createObjectFunc = (Func_CreateObject)GetProcAddress(DllHandleName, "CreateObject");
	if (!createObjectFunc)
	{
		cout << "load function: CreatObject failed" << endl;
		return false;
	}
	_createObjectFunc = createObjectFunc;
	return true;
}

bool CompressExtract::findOpenInit(const wstring &archiveFileName)
{
	FString archiveName = StringToFString(archiveFileName.c_str());
	CMyComPtr<IInArchive> archive;
	if (_createObjectFunc(&CLSID_Format, &IID_IInArchive, (void **)&archive) != S_OK)
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
		openCallbackSpec->PasswordIsDefined = false;
		const UInt64 scanSize = 1 << 23;
		//文件虽说找到了，但格式不对，如.rar格式的
		if (archive->Open(file, &scanSize, openCallback) != S_OK)
		{
			cout << "Can not open file as archive" << endl;
			return false;
		}
		_archive = archive;
	}
	return true;
}

bool CompressExtract::ExtractFile(const wstring &archiveFileName,const wstring &outputPathName)
{
	  if (!Load7zDLL())
		return false;
	  if (archiveFileName.empty() || !createObjectInit()||!findOpenInit(archiveFileName))
		 return false;
	   //uncompress
		CArchiveExtractCallback *extractCallbackSpec = new CArchiveExtractCallback;
		CMyComPtr<IArchiveExtractCallback> extractCallback(extractCallbackSpec);
		FString outputPath = StringToFString(outputPathName.c_str());//输出的路径
		extractCallbackSpec->Init(_archive, outputPath);
		extractCallbackSpec->PasswordIsDefined = false;
		HRESULT result = _archive->Extract(NULL, (UInt32)(Int32)(-1), false, extractCallback);
		if (result != S_OK)
		{
			cout << "Extract Error" << endl;
			return false;
		}
	return true;
}

bool CompressExtract::ShowArchivefileList(const wstring &archiveFileName,map<wstring,int> &archivefilelist)
{
	if (!Load7zDLL())
		return false;
	if (archiveFileName.empty() || !createObjectInit() || !findOpenInit(archiveFileName))
		return false;
	UInt32 numItems = 0;
	_archive->GetNumberOfItems(&numItems);
	for (UInt32 i = 0; i < numItems; i++)
	{
		// Get uncompressed size of file
		NCOM::CPropVariant prop;
		_archive->GetProperty(i, kpidSize, &prop);
		char s[32];//之前是char
		ConvertPropVariantToShortString(prop, s);//s为文件的大小	
		// Get name of file
		//NCOM::CPropVariant prop;
		_archive->GetProperty(i, kpidPath, &prop);
		if (prop.vt == VT_BSTR)
		{
			archivefilelist.insert(make_pair(prop.bstrVal,atoi(s)));
		}
		else if (prop.vt != VT_EMPTY)
			cout << "ERROR!" << endl;
		
	     
	}
	return true;
}

void CompressExtract::FileStringToChar(const wstring &fileNames)
{
	if (fileNames.empty())
		return ;
	//取出文件
	vector<wstring>filename;
	int start = 0;
	wstring cstr = L"";
	for (int i = 0; (size_t)i < fileNames.size(); i++)
	{
		if (fileNames[i] == '/')//以'/'分开，最后一个以长度分开
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

string CompressExtract::wstring2string(const wstring &wstr)
{
	string result;
	//获取缓冲区大小，并申请空间，缓冲区大小事按字节计算的  
	int len = WideCharToMultiByte(CP_ACP, 0, wstr.c_str(), wstr.size(), NULL, 0, NULL, NULL);
	char* buffer = new char[len + 1];
	//宽字节编码转换成多字节编码  
	WideCharToMultiByte(CP_ACP, 0, wstr.c_str(), wstr.size(), buffer, len, NULL, NULL);
	buffer[len] = '\0';
	//删除缓冲区并返回值  
	result.append(buffer);
	delete[] buffer;
	return result;
}

wstring CompressExtract::string2wstring(const string &str)
{
	wstring result;
	//获取缓冲区大小，并申请空间，缓冲区大小按字符计算  
	int len = MultiByteToWideChar(CP_ACP, 0, str.c_str(), str.size(), NULL, 0);
	TCHAR* buffer = new TCHAR[len + 1];
	//多字节编码转换成宽字节编码  
	MultiByteToWideChar(CP_ACP, 0, str.c_str(), str.size(), buffer, len);
	buffer[len] = '\0';             //添加字符串结尾  
	//删除缓冲区并返回值  
	result.append(buffer);
	delete[] buffer;
	return result;
}

bool CompressExtract::filePathExist(const wstring &wsfileName, vector<wstring>& filesList)//wsfileName可以用通配符(可以是某一类文件)
{
	if (wsfileName.empty())
		return false;
	struct _finddata_t fileinfo;
	string sfileName = wstring2string(wsfileName);
	long hFile = _findfirst(sfileName.c_str(), &fileinfo);
	if (hFile == -1)
	{
		cout << "file not existed" << endl;
		return false;
	}
	else
	{
		do
		{
			wstring wstemp = string2wstring(sfileName);
			filesList.push_back(wstemp);
		} while (_findnext(hFile, &fileinfo) == 0);
	}
	return true;
}

bool CompressExtract::DirectoryPathExit(const wstring &wsDirName, vector<wstring>&filesList)
{
	if (wsDirName.empty())
		return false;
	struct _finddata_t fileinfo;
	string sDirName = wstring2string(wsDirName);
	string sDirName_append = sDirName + "\\*";
	long hFile = _findfirst(sDirName_append.c_str(), &fileinfo);
	if (hFile == -1)
	{
		cout << "can't match path" << endl;
		return false;
	}
	do{
		string stemp = sDirName + "\\" + fileinfo.name;
		wstring newPath = string2wstring(stemp);

		if (fileinfo.attrib & _A_SUBDIR)//目录
		{
			if ((strcmp(fileinfo.name, ".") != 0) && (strcmp(fileinfo.name, "..") != 0))
			{
				DirectoryPathExit(newPath, filesList);
			}
		}
		else
		{
			filesList.push_back(newPath);
		}
	} while (_findnext(hFile, &fileinfo) == 0);

	if (strcmp(fileinfo.name, "..") == 0)//文件夹为空
	{
		filesList.push_back(wsDirName);
	}
	return true;
}

bool CompressExtract::GetAllFiles()
{
	for (int i = 0; (size_t)i< _filename.size(); i++)
	{
		if (_filename[i].empty())
			return false;
		struct _finddata_t fileinfo;
		string sallNames = wstring2string(_filename[i]);
		long hFile = _findfirst(sallNames.c_str(), &fileinfo);
		if (fileinfo.attrib &_A_SUBDIR)
			 DirectoryPathExit(_filename[i], _allfileLise);
		else
			 filePathExist(_filename[i], _allfileLise);
	}
	return true;
}


bool CompressExtract::CompressFile(const wstring &archiveFileName, const wstring &fileNames)
{
	if (!Load7zDLL())
		return false;
	if (archiveFileName.empty() || fileNames.empty() || !createObjectInit())
		return false;
	FString archiveName = StringToFString(archiveFileName.c_str());
	FileStringToChar(fileNames);
	if (!GetAllFiles())
	{
		cout << "file get failed" << endl;
		return false;
	}
	CObjectVector<CDirItem> dirItems;
	{
		size_t i;
		for (i = 0; i < _allfileLise.size(); i++)
		{
			CDirItem di;
			FString name = StringToFString(_allfileLise[i].c_str());//文件名

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
			di.Name = fs2us(name);
			di.FullPath = name;
			dirItems.Add(di);
		}
	}

	COutFileStream *outFileStreamSpec = new COutFileStream;
	CMyComPtr<IOutStream> outFileStream = outFileStreamSpec;
	if (!outFileStreamSpec->Create(archiveName, false))
	{
		cout << "can't create archive file" << endl;
		return false;
	}

	CMyComPtr<IOutArchive> outArchive;
	if (_createObjectFunc(&CLSID_Format, &IID_IOutArchive, (void **)&outArchive) != S_OK)
	{
		cout << "Can not get class object" << endl;
		return false;
	}

	CArchiveUpdateCallback *updateCallbackSpec = new CArchiveUpdateCallback;
	CMyComPtr<IArchiveUpdateCallback2> updateCallback(updateCallbackSpec);
	updateCallbackSpec->Init(&dirItems);

	HRESULT result = outArchive->UpdateItems(outFileStream, dirItems.Size(), updateCallback);
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