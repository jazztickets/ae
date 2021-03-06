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
#pragma once

#include <unordered_map>
#include <map>
#include <string>

namespace ae {

// Hold information about tile hierarchy and index
class _TileMap {

	public:

		struct _TileData {
			std::string ID;
			uint32_t Index;
			int Hierarchy;
		};

		_TileMap(const std::string &Path);

		std::unordered_map<std::string, _TileData> Data;
		std::map<uint32_t, const _TileData *> Index;

	private:

};

}
