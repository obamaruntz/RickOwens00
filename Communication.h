#pragma once
#include <Windows.h>
#include <tlhelp32.h>
#include <cstdint>
#include <string>
#include <vector>

#include "Definitions.h"

class Communication {
public:
	Communication();
	~Communication();

	bool isConnected();

	bool virtualAttach(i32 processId);

	i32 findProcess(const i8* processName); // UM
	u64 findImage();

	template <typename T>
	T virtualRead(u64 address);
	template <typename T>
	void virtualWrite(u64 address, T& value);

	template <typename T>
	T read(u64 address);
	template <typename T>
	void write(u64 address, T& value);

	std::string readString(u64 address);

	HANDLE getDriverHandle();
	i32 getProcessId();
	u64 getImageAddress();
private:
	HANDLE driverHandle = INVALID_HANDLE_VALUE;
	i32 processId = 0;
	u64 imageAddress = 0;
};

template <typename T>
T Communication::virtualRead(u64 address) {
	T temp = {};

	_VRW arguments;
	arguments.securityCode = SECURITY_CODE;
	arguments.address = reinterpret_cast<void*>(address);
	arguments.buffer = &temp;
	arguments.size = sizeof(T);
	arguments.type = false;

	DeviceIoControl(driverHandle, VRW_CODE, &arguments, sizeof(arguments), &arguments, sizeof(arguments), nullptr, nullptr);
	return temp;
}

template <typename T>
void Communication::virtualWrite(u64 address, T& value) {
	_VRW arguments;
	arguments.securityCode = SECURITY_CODE;
	arguments.address = reinterpret_cast<void*>(address);
	arguments.buffer = (void*)&value;
	arguments.size = sizeof(T);
	arguments.type = true;

	DeviceIoControl(driverHandle, VRW_CODE, &arguments, sizeof(arguments), &arguments, sizeof(arguments), nullptr, nullptr);
}

template <typename T>
T Communication::read(u64 address) {
	T temp = {};

	_PRW arguments = {};
	arguments.securityCode = SECURITY_CODE;
	arguments.address = reinterpret_cast<void*>(address);
	arguments.buffer = &temp;
	arguments.size = sizeof(T);
	arguments.processId = processId;
	arguments.type = false;

	DeviceIoControl(driverHandle, PRW_CODE, &arguments, sizeof(arguments), nullptr, NULL, NULL, NULL);

	return temp;
}

template <typename T>
void Communication::write(u64 address, T& value) {
	_PRW arguments = {};
	arguments.securityCode = SECURITY_CODE;
	arguments.address = reinterpret_cast<void*>(address);
	arguments.buffer = (void*)&value;
	arguments.size = sizeof(T);
	arguments.processId = processId;
	arguments.type = true;

	DeviceIoControl(driverHandle, PRW_CODE, &arguments, sizeof(arguments), nullptr, NULL, NULL, NULL);
}
