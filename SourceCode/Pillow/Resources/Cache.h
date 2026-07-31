// PillowBasics Copyright (c) 2025, Icey Pillow. BSD 3-Clause License. Do not remove, obscure, or alter this notice.
#pragma once
#include "Common.h"
#include <filesystem>
#include <fstream>

using namespace std::filesystem;

namespace Pillow
{
   enum struct CacheType
   {
      Texture,
      Mesh,
      Shader
   };

   enum struct AssetBundleType
   {
      Static,
      Increment
   };

   ForceInline void WriteCache(std::vector<uint8_t>& data, path location)
   {
      std::ofstream file(location, std::ios::binary);
      file.write(reinterpret_cast<const char*>(data.data()), data.size());
   }

   ForceInline void ReadCache(std::vector<uint8_t>& data, path location)
   {
      data.clear();
      auto size = std::filesystem::file_size(location);
      data.resize(size);
      std::ifstream file(location, std::ios::binary);
      file.read(reinterpret_cast<char*>(data.data()), size);
   }

   ForceInline void BuildAssetBundle()
   {

   }

   ForceInline void AlterAssetBundle()
   {

   }

   ForceInline void ReadAssetBundle()
   {

   }
}