/******************************************************************************
* Copyright (c) 2021 Alan Witkowski
*
* This software is provided 'as-is', without any express or implied
* warranty. In no event will the authors be held liable for any damages
* arising from the use of this software.
*
* Permission is granted to anyone to use this software for any purpose,
* including commercial applications, and to alter it and redistribute it
* freely, subject to the following restrictions:
*
* 1. The origin of this software must not be misrepresented; you must not
*    claim that you wrote the original software. If you use this software
*    in a product, an acknowledgment in the product documentation would be
*    appreciated but is not required.
* 2. Altered source versions must be plainly marked as such, and must not be
*    misrepresented as being the original software.
* 3. This notice may not be removed or altered from any source distribution.
*******************************************************************************/
#include <ae/files.h>
#include <fstream>
#include <cstring>
#include <algorithm>

namespace ae {

#ifdef _WIN32
	#define WIN32_LEAN_AND_MEAN
	#include <windows.h>
#else
	#include <sys/stat.h>
	#include <dirent.h>
#endif

// Constructor
_Files::_Files(const std::string &Path) : Path(Path) {
	Load(Path);
}

// Get a list of files in a directory
void _Files::Load(const std::string &Path, bool PrependPath) {

#ifdef _WIN32

	// Get file handle
	WIN32_FIND_DATA FindFileData;
	HANDLE FindHandle = FindFirstFile((Path + "*").c_str(), &FindFileData);
	if(FindHandle == INVALID_HANDLE_VALUE) {
		return;
	}

	// Add first value
	if(!(FindFileData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)) {
		if(PrependPath)
			Nodes.push_back(Path + FindFileData.cFileName);
		else
			Nodes.push_back(FindFileData.cFileName);
	}

	// Get the other files
	while(FindNextFile(FindHandle, &FindFileData)) {
		if(!(FindFileData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)) {
			if(PrependPath)
				Nodes.push_back(Path + FindFileData.cFileName);
			else
				Nodes.push_back(FindFileData.cFileName);
		}
	}

	// Close
	FindClose(FindHandle);
#else

	// Read directory
	DIR *Directory;
	struct dirent *Entry;
	Directory = opendir(Path.c_str());
	if(Directory) {

		// Iterate through files
		while((Entry = readdir(Directory)) != nullptr) {
			if(Entry->d_type == DT_REG) {
				if(PrependPath)
					Nodes.push_back(Path + Entry->d_name);
				else
					Nodes.push_back(Entry->d_name);
			}
		}

		// Close handle
		closedir(Directory);
	}
#endif

	// Sort files
	std::sort(Nodes.begin(), Nodes.end());
}

// Constructor
_FilePack::_FilePack(const std::string &Path) {
	Load(Path);
}

// Read list of files in pack
void _FilePack::Load(const std::string &Path) {
	this->Path = Path;

	// Open file
	std::ifstream Input(Path.c_str(), std::ios::binary);

	// Get file count
	int FileCount = 0;
	Input.read((char *)&FileCount, 4);

	// Load header
	char Buffer[256];
	int Offset = 0;
	for(int i = 0; i < FileCount; i++) {

		// Read file name
		int NameSize = Input.get();
		Input.read(Buffer, NameSize);
		Buffer[NameSize] = 0;

		// Get base name
		char *BaseName = strrchr(Buffer, '/');
		if(BaseName)
			BaseName++;
		else
			BaseName = Buffer;

		// Read file size
		int Size;
		Input.read((char *)&Size, 4);
		Data[Buffer] = _File(BaseName, Size, Offset);

		Offset += Size;
	}

	// Save start of body
	BodyOffset = Input.tellg();
}

}
