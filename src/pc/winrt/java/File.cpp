#include "java/File.h"

#define WIN32_LEAN_AND_MEAN
#define WINRT_LEAN_AND_MEAN
#include <Windows.h>
#include <fileapi.h>

#include "SDL_system.h"

#include <queue>
#include <string>
#include <cstdio>
#include <iostream>
#include <fstream>
#include <memory>
#include <stdexcept>

#include "util/Memory.h"

static jstring FromWPath(const std::wstring &wstr)
{
	if (wstr.empty())
		return u"";

	std::u16string u16str(wstr.begin(), wstr.end());
	return u16str;
}

static std::wstring ToWPath(const jstring &path)
{
	if (path.empty())
		return L"";

	// Convert u16string to wstring — UWP paths are used as-is (no \\?\ prefix).
	std::wstring wpath(path.begin(), path.end());

	// Normalize separators
	for (auto &ch : wpath)
	{
		if (ch == L'/')
			ch = L'\\';
	}

	// Remove trailing slashes
	while (!wpath.empty() && wpath.back() == L'\\')
		wpath.pop_back();

	return wpath;
}

// UWP-compatible wrapper around CreateFile2.
static HANDLE UWP_CreateFile(const wchar_t *path, DWORD access, DWORD share, DWORD disposition)
{
	CREATEFILE2_EXTENDED_PARAMETERS params = {};
	params.dwSize = sizeof(params);
	params.dwFileAttributes = FILE_ATTRIBUTE_NORMAL;
	return CreateFile2(path, access, share, disposition, &params);
}

class File_Impl : public File
{
private:
	std::wstring wpath;

public:
	File_Impl(const jstring &path)
	{
		wpath = ToWPath(path);
		this->path = FromWPath(wpath);
	}

	virtual ~File_Impl()
	{
	}

	virtual bool createNewFile() const override
	{
		HANDLE hfile = UWP_CreateFile(wpath.c_str(), GENERIC_WRITE, 0, CREATE_NEW);
		if (hfile == INVALID_HANDLE_VALUE)
			return false;
		CloseHandle(hfile);
		return true;
	}

	bool remove() const override
	{
		if (isDirectory())
			return RemoveDirectoryW(wpath.c_str()) != 0;
		else
			return DeleteFileW(wpath.c_str()) != 0;
	}

	bool renameTo(const File &dest) const override
	{
		const File_Impl &dest_impl = reinterpret_cast<const File_Impl&>(dest);
		// MoveFileExW with MOVEFILE_REPLACE_EXISTING is in the UWP API set.
		if (MoveFileExW(wpath.c_str(), dest_impl.wpath.c_str(), MOVEFILE_REPLACE_EXISTING) == 0)
			return false;
		return true;
	}

	bool exists() const override
	{
		WIN32_FILE_ATTRIBUTE_DATA data;
		return GetFileAttributesExW(wpath.c_str(), GetFileExInfoStandard, &data) != 0;
	}

	bool isDirectory() const override
	{
		WIN32_FILE_ATTRIBUTE_DATA data;
		if (!GetFileAttributesExW(wpath.c_str(), GetFileExInfoStandard, &data))
			return false;
		return (data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0;
	}

	bool isFile() const override
	{
		WIN32_FILE_ATTRIBUTE_DATA data;
		if (!GetFileAttributesExW(wpath.c_str(), GetFileExInfoStandard, &data))
			return false;
		return (data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) == 0;
	}

	long_t lastModified() const override
	{
		HANDLE hfile = UWP_CreateFile(wpath.c_str(), GENERIC_READ, FILE_SHARE_READ, OPEN_EXISTING);
		if (hfile == INVALID_HANDLE_VALUE)
			return 0;

		FILETIME ftwrite;
		if (!GetFileTime(hfile, nullptr, nullptr, &ftwrite))
		{
			CloseHandle(hfile);
			return 0;
		}

		CloseHandle(hfile);

		ULARGE_INTEGER uli;
		uli.HighPart = ftwrite.dwHighDateTime;
		uli.LowPart = ftwrite.dwLowDateTime;

		ULONGLONG ms = uli.QuadPart / 10000;
		ULONGLONG epoch = 11644473600000; // 1970-01-01

		if (ms < epoch)
			return 0;
		else
			return ms - epoch;
	}

	long_t length() const override
	{
		HANDLE hfile = UWP_CreateFile(wpath.c_str(), GENERIC_READ, FILE_SHARE_READ, OPEN_EXISTING);
		if (hfile == INVALID_HANDLE_VALUE)
			return 0;

		LARGE_INTEGER size;
		if (!GetFileSizeEx(hfile, &size))
		{
			CloseHandle(hfile);
			return 0;
		}

		CloseHandle(hfile);
		return size.QuadPart;
	}

	std::vector<std::unique_ptr<File>> listFiles() const override
	{
		std::vector<std::unique_ptr<File>> files;

		if (!isDirectory())
			return files;

		// FindFirstFileExW is available on UWP (WINAPI_PARTITION_APP).
		WIN32_FIND_DATAW find_data;
		std::wstring pattern = wpath + L"\\*";
		HANDLE hfind = FindFirstFileExW(
			pattern.c_str(), FindExInfoBasic, &find_data,
			FindExSearchNameMatch, nullptr, 0);
		if (hfind == INVALID_HANDLE_VALUE)
			return files;

		do
		{
			if (wcscmp(find_data.cFileName, L".") == 0 || wcscmp(find_data.cFileName, L"..") == 0)
				continue;

			std::wstring child_path = wpath + L'\\' + find_data.cFileName;
			files.push_back(Util::make_unique<File_Impl>(FromWPath(child_path)));
		} while (FindNextFileW(hfind, &find_data) != 0);

		FindClose(hfind);
		return files;
	}

	jstring getName() const override
	{
		size_t wpos = wpath.find_last_of(L"/\\");
		if (wpos == std::wstring::npos)
			return FromWPath(wpath);

		if (wpos + 1 >= wpath.length())
			return u"";
			
		return FromWPath(wpath.substr(wpos + 1));
	}

	File *getParentFile() const override
	{
		size_t npos = path.find_last_of(u"/\\");
		if (npos != std::string::npos)
			return new File_Impl(path.substr(0, npos));
		return new File_Impl(u"");
	}

	bool mkdir() const override
	{
		return CreateDirectoryW(wpath.c_str(), nullptr) != 0;
	}

	std::istream *toStreamIn() const override
	{
		auto is = Util::make_unique<std::ifstream>(wpath, std::ios::binary);
		if (!is->is_open() || !is->good())
			return nullptr;
		return is.release();
	}

	std::ostream *toStreamOut() const override
	{
		auto os = Util::make_unique<std::ofstream>(wpath, std::ios::binary);
		if (!os->is_open() || !os->good())
			return nullptr;
		return os.release();
	}

	friend File *File::open(const jstring &path);
	friend File *File::open(const File &parent, const jstring &child);
};

File *File::open(const jstring &path)
{
	return new File_Impl(path);
}

File *File::open(const File &parent, const jstring &child)
{
	jstring new_path = parent.path + u'\\' + child;
	return new File_Impl(new_path);
}

File *File::openResourceDirectory()
{
	// Get the path to the executable — GetModuleFileNameW is available on UWP.
	std::wstring path(MAX_PATH, 0);
	while (1)
	{
		DWORD length = GetModuleFileNameW(nullptr, &path.front(), static_cast<DWORD>(path.size()));
		if (length < path.size())
		{
			path.resize(length);
			break;
		}
		path.resize(path.size() * 2);
	}

	jstring u16str = FromWPath(path);

	size_t pos = u16str.find_last_of(u"/\\");
	if (pos == std::string::npos)
		return new File_Impl(u"");

	return new File_Impl(u16str.substr(0, pos) + u"\\resource");
}

File *File::openWorkingDirectory(const jstring &name)
{
	const wchar_t *localPath = SDL_WinRTGetFSPathUNICODE(SDL_WINRT_PATH_LOCAL_FOLDER);
	if (localPath == nullptr)
		return new File_Impl(u"");

	jstring u16str = FromWPath(std::wstring(localPath));
	return new File_Impl(u16str + u"\\" + name);
}
