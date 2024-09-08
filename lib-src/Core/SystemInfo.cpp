
#include "SystemInfo.h"

#include "WindowsUtils.h"

#include <intrin.h>


namespace ui {

const char* CPUArchitectureToString(CPUArchitecture arch)
{
	switch (arch)
	{
	default:
	case CPUArchitecture::Unknown: return "Unknown";
	case CPUArchitecture::x86: return "x86";
	case CPUArchitecture::x64: return "x64";
	case CPUArchitecture::ARM32: return "ARM32";
	case CPUArchitecture::ARM64: return "ARM64";
	}
}

void CPUInfo::Read()
{
	memset(vendor, 0, sizeof(vendor));
	memset(name, 0, sizeof(name));

	i32 cpuinfo[4];

	__cpuid(cpuinfo, 0);
	u32 numIDs = cpuinfo[0];
	std::swap(cpuinfo[2], cpuinfo[3]);
	memcpy(vendor, &cpuinfo[1], 12);

	__cpuid(cpuinfo, 0x80000000);
	u32 numExtIDs = cpuinfo[0];
	if (numExtIDs >= 0x80000004)
	{
		__cpuid(cpuinfo, 0x80000002);
		memcpy(name, cpuinfo, sizeof(cpuinfo));
		__cpuid(cpuinfo, 0x80000003);
		memcpy(name + 16, cpuinfo, sizeof(cpuinfo));
		__cpuid(cpuinfo, 0x80000004);
		memcpy(name + 32, cpuinfo, sizeof(cpuinfo));
	}
}

static CPUArchitecture FromW32Arch(WORD arch)
{
	switch (arch)
	{
	case PROCESSOR_ARCHITECTURE_INTEL: return CPUArchitecture::x86;
	case PROCESSOR_ARCHITECTURE_AMD64: return CPUArchitecture::x64;
	case PROCESSOR_ARCHITECTURE_ARM: return CPUArchitecture::ARM32;
	case 12/*PROCESSOR_ARCHITECTURE_ARM64*/: return CPUArchitecture::ARM64;
	default: return CPUArchitecture::Unknown;
	}
}

void SystemInfo::ReadAll()
{
	SYSTEM_INFO sysinfo;
	GetSystemInfo(&sysinfo);

	numAvailLogicalCPUCores = sysinfo.dwNumberOfProcessors;
	appArch = FromW32Arch(sysinfo.wProcessorArchitecture);
	pageSize = sysinfo.dwPageSize;

	SYSTEM_INFO nsysinfo;
	GetNativeSystemInfo(&nsysinfo);
	osArch = FromW32Arch(nsysinfo.wProcessorArchitecture);

	MEMORYSTATUSEX mstatus = {};
	mstatus.dwLength = sizeof(mstatus);
	GlobalMemoryStatusEx(&mstatus);

	totalPhysicalMemory = mstatus.ullTotalPhys;
	totalVirtualMemory = mstatus.ullTotalVirtual;
	totalPageFile = mstatus.ullTotalPageFile;
}

} // ui
