// PillowBasics Copyright (c) 2025, Icey Pillow. BSD 3-Clause License. Do not remove, obscure, or alter this notice.
#include "Renderer.h"
#include <ranges>
#include <algorithm>

using namespace Pillow;
using namespace Pillow::Graphics;
using namespace DirectX;

namespace
{
   std::unique_ptr<IRenderer> rendererInstance;
   std::vector<GenericRendererCommand> cmdListBusy;

   // Example: NameID = "HelloWorld@Stages=VS,PS@Depth=0@Blend=0@ASSERT_ON@Quality=2"
   std::string MakePipelineStateID(const path& originalName, const std::vector<KeyValuePair>& macros, const PipelineInfo::Configuration& config)
   {
      constexpr char separator = ',';
      constexpr char prefixMacro = '@';
      constexpr char prefixValue = '=';
      string name = GetU8StringfromPath(originalName);
      name += "@Stages=";
      using ShaderType = PipelineInfo::ShaderType;
      for (uint16_t i = 1; i <= uint16_t(ShaderType::Count); i++)
      {
         if (i != 1)
         {
            name += separator;
         }
         if (PipelineInfo::CheckShaderMask(config.ShaderMask, ShaderType(i)))
         {
            name += PipelineInfo::ShaderAcronyms[i];
         }
      }
      name += std::format("@Depth={}@Blend={}", int32_t(config.Depth), int32_t(config.Blend));
      for (const auto& pair : macros)
      {
         name += prefixMacro + pair.GetKey();
         if (!pair.EmptyValue())
         {
            name += prefixValue + pair.GetValueRaw();
         }
      }
      return name;
   }
}

namespace Pillow::Graphics
{
   std::vector<GenericRendererCommand> cmdListIdle;
}

PipelineInfo::PipelineInfo(path shortPath, std::vector<KeyValuePair>& macros, Configuration& config) :
   ID(MakePipelineStateID(shortPath, macros, config)),
   ShortPath(std::move(shortPath)),
   Macros(macros),
   Config(config)
{
   // Empty body
}

bool PipelineInfo::EqualTo(const PipelineInfo& right) const
{
   return this->ID == right.ID;
}

void IRenderer::Initialize(uint32_t threadCount, XMINT2 renderBufferSize, int32_t refreshRate, void* parameter)
{
   if (rendererInstance) throw std::runtime_error("Renderer has already been initialized.");
#if defined(_WIN64)
   rendererInstance = std::make_unique<Graphics::D3D12Renderer>(parameter, threadCount, renderBufferSize, refreshRate);
#elif defined(__ANDROID__)
   //RendererInstance = std::make_unique<Pillow::Vulkan12Renderer>(Hwnd, 2);
#endif
}

IRenderer* IRenderer::GetInstance()
{
#ifdef PILLOW_DEBUG
   if (!rendererInstance) throw std::runtime_error("Renderer instance is not initialized.");
#endif
   return rendererInstance.get();
}

IRenderer::IRenderer(uint32_t threadCount, std::string name, XMINT2 backBufferSize, int32_t refreshRate) :
   f_RendererName(name),
   f_ThreadCount(threadCount),
   f_BackBufferSize(backBufferSize),
   f_RefreshRate(refreshRate),
   f_VSyncBlanks(1),
   threadPool(threadCount, this, &IRenderer::Worker, &IRenderer::BasePioneer, &IRenderer::Assembler)
{
   cmdListIdle.reserve(ReservedCommandCount);
   cmdListBusy.reserve(ReservedCommandCount);
}

IRenderer::~IRenderer()
{
   cmdListIdle.clear();
   cmdListIdle.shrink_to_fit();
   cmdListBusy.clear();
   cmdListBusy.shrink_to_fit();
}


const std::vector<GenericRendererCommand>* IRenderer::GetBusyCmdList()
{
   if (!rendererInstance) throw std::runtime_error("Renderer instance is not initialized.");
   return &cmdListBusy;
}

void IRenderer::BasePioneer()
{
   cmdListIdle.swap(cmdListBusy);
   cmdListIdle.clear();
   // Pre-process before the worker threads.
   Pioneer();
}