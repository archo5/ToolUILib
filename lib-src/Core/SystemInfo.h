
#pragma once

#include "Platform.h"


namespace ui {

enum class CPUArchitecture : u8
{
	Unknown = 0,
	x86,
	x64,
	ARM32,
	ARM64,
};
const char* CPUArchitectureToString(CPUArchitecture arch);

struct CPUInfo
{
	char vendor[13];
	char name[49];

	void Read();
};

struct SystemInfo
{
	CPUArchitecture osArch;
	CPUArchitecture appArch;
	u32 numAvailLogicalCPUCores;
	u32 pageSize;
	u64 totalPhysicalMemory;
	u64 totalVirtualMemory;
	u64 totalPageFile;

	void ReadAll();
};

} // ui
