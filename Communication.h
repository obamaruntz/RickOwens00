#pragma once
#include <Windows.h>
#include <tlhelp32.h>
#include <cstdint>
#include <string>

#include <Framework.h>

#define PRW_CODE CTL_CODE(FILE_DEVICE_UNKNOWN, 0x2ec33, METHOD_BUFFERED, FILE_SPECIAL_ACCESS)
#define VRW_ATTACH_CODE CTL_CODE(FILE_DEVICE_UNKNOWN, 0x2ec34, METHOD_BUFFERED, FILE_SPECIAL_ACCESS)
#define VRW_CODE CTL_CODE(FILE_DEVICE_UNKNOWN, 0x2ec35, METHOD_BUFFERED, FILE_SPECIAL_ACCESS)
#define BA_CODE CTL_CODE(FILE_DEVICE_UNKNOWN, 0x2ec36, METHOD_BUFFERED, FILE_SPECIAL_ACCESS)  
#define SECURITY_CODE 0x94c9e4bc3

struct _PRW {
	u64 SecurityCode;

	i32 ProcessID;
	void* Address;
	void* Buffer;

	u64 Size;
	u64 ReturnSize;

	bool Type;
};

struct _VRW {
	u64 SecurityCode;

	HANDLE ProcessHandle;
	void* Address;
	void* Buffer;

	u64 Size;
	u64 ReturnSize;

	bool Type;
};

struct _BA {
	u64 SecurityCode;

	i32 ProcessID;
	u64* Address;
};

class Communication {
public:
	Communication();
	~Communication();

	bool IsConnected();

	bool VAttach(i32 ProcessID);

	i32 FindProcess(const i8* ProcessName); // UM
	u64 FindImage();

	template <typename T>
	T VRead(u64 Address);
	template <typename T>
	void VWrite(u64 Address, T& Value);

	template <typename T>
	T PRead(u64 Address);
	template <typename T>
	void PWrite(u64 Address, T& Value);

	std::string ReadStr(u64 Address);

	u64 ImageAddress = 0;
	i32 ProcessID = 0;
private:
	HANDLE DriverHandle = INVALID_HANDLE_VALUE;
};

// C++ Sucks

template <typename T>
T Communication::VRead(u64 Address) {
	T Temp = {};

	_VRW Arguments;
	Arguments.SecurityCode = SECURITY_CODE;
	Arguments.Address = reinterpret_cast<void*>(Address);
	Arguments.Buffer = &Temp;
	Arguments.Size = sizeof(T);
	Arguments.Type = false;

	DeviceIoControl(DriverHandle, VRW_CODE, &Arguments, sizeof(Arguments), &Arguments, sizeof(Arguments), nullptr, nullptr);
	return Temp;
}

template <typename T>
void Communication::VWrite(u64 Address, T& Value) {
	_VRW Arguments;
	Arguments.SecurityCode = SECURITY_CODE;
	Arguments.Address = reinterpret_cast<void*>(Address);
	Arguments.Buffer = (void*)&Value;
	Arguments.Size = sizeof(T);
	Arguments.Type = true;

	DeviceIoControl(DriverHandle, VRW_CODE, &Arguments, sizeof(Arguments), &Arguments, sizeof(Arguments), nullptr, nullptr);
}

template <typename T>
T Communication::PRead(u64 Address) {
	T Temp = {};

	_PRW Arguments = {};
	Arguments.SecurityCode = SECURITY_CODE;
	Arguments.Address = reinterpret_cast<void*>(Address);
	Arguments.Buffer = &Temp;
	Arguments.Size = sizeof(T);
	Arguments.ProcessID = ProcessID;
	Arguments.Type = false;

	DeviceIoControl(DriverHandle, PRW_CODE, &Arguments, sizeof(Arguments), nullptr, NULL, NULL, NULL);

	return Temp;
}

template <typename T>
void Communication::PWrite(u64 Address, T& Value) {
	_PRW Arguments = {};
	Arguments.SecurityCode = SECURITY_CODE;
	Arguments.Address = reinterpret_cast<void*>(Address);
	Arguments.Buffer = (void*)&Value;
	Arguments.Size = sizeof(T);
	Arguments.ProcessID = ProcessID;
	Arguments.Type = true;

	DeviceIoControl(DriverHandle, PRW_CODE, &Arguments, sizeof(Arguments), nullptr, NULL, NULL, NULL);
}