
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

void SystemInfo::ReadAll()
{
	SYSTEM_INFO sysinfo;
	GetSystemInfo(&sysinfo);

	numAvailLogicalCPUCores = sysinfo.dwNumberOfProcessors;
	switch (sysinfo.wProcessorArchitecture)
	{
	case PROCESSOR_ARCHITECTURE_INTEL: osArch = CPUArchitecture::x86; break;
	case PROCESSOR_ARCHITECTURE_AMD64: osArch = CPUArchitecture::x64; break;
	case PROCESSOR_ARCHITECTURE_ARM: osArch = CPUArchitecture::ARM32; break;
	case 12/*PROCESSOR_ARCHITECTURE_ARM64*/: osArch = CPUArchitecture::ARM64; break;
	default: osArch = CPUArchitecture::Unknown; break;
	}
	pageSize = sysinfo.dwPageSize;

	MEMORYSTATUSEX mstatus = {};
	mstatus.dwLength = sizeof(mstatus);
	GlobalMemoryStatusEx(&mstatus);

	totalPhysicalMemory = mstatus.ullTotalPhys;
	totalVirtualMemory = mstatus.ullTotalVirtual;
	totalPageFile = mstatus.ullTotalPageFile;
}

} // ui
