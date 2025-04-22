#pragma once
#include <Windows.h>
#include <tlhelp32.h>
#include <cstdint>
#include <string>

#include <framework.h>

#define PRW_CODE CTL_CODE(FILE_DEVICE_UNKNOWN, 0x2ec33, METHOD_BUFFERED, FILE_SPECIAL_ACCESS)
#define VRW_ATTACH_CODE CTL_CODE(FILE_DEVICE_UNKNOWN, 0x2ec34, METHOD_BUFFERED, FILE_SPECIAL_ACCESS)
#define VRW_CODE CTL_CODE(FILE_DEVICE_UNKNOWN, 0x2ec35, METHOD_BUFFERED, FILE_SPECIAL_ACCESS)
#define BA_CODE CTL_CODE(FILE_DEVICE_UNKNOWN, 0x2ec36, METHOD_BUFFERED, FILE_SPECIAL_ACCESS)  
#define SECURITY_CODE 0x94c9e4bc3

struct _PRW {
	u64 security_code;

	i32 process_id;
	void* address;
	void* buffer;

	u64 size;
	u64 return_size;

	bool Type;
};

struct _VRW {
	u64 security_code;

	HANDLE process_handle;
	void* address;
	void* buffer;

	u64 size;
	u64 return_size;

	bool Type;
};

struct _BA {
	u64 security_code;

	i32 process_id;
	u64* address;
};

class communication {
public:
	communication();
	~communication();

	bool is_connected();

	bool v_attach(i32 process_id);

	i32 find_process(const i8* ProcessName); // UM
	u64 find_image();

	template <typename T>
	T v_read(u64 address);
	template <typename T>
	void v_write(u64 address, T& Value);

	template <typename T>
	T read(u64 address);
	template <typename T>
	void read(u64 address, T& Value);

	std::string readstr(u64 address);

	u64 image_address = 0;
	i32 process_id = 0;
private:
	HANDLE driver_handle = INVALID_HANDLE_VALUE;
};

template <typename T>
T communication::v_read(u64 address) {
	T temp = {};

	_VRW arguments;
	arguments.security_code = SECURITY_CODE;
	arguments.address = reinterpret_cast<void*>(address);
	arguments.buffer = &temp;
	arguments.size = sizeof(T);
	arguments.Type = false;

	DeviceIoControl(driver_handle, VRW_CODE, &arguments, sizeof(arguments), &arguments, sizeof(arguments), nullptr, nullptr);
	return temp;
}

template <typename T>
void communication::v_write(u64 address, T& value) {
	_VRW arguments;
	arguments.security_code = SECURITY_CODE;
	arguments.address = reinterpret_cast<void*>(address);
	arguments.buffer = (void*)&value;
	arguments.size = sizeof(T);
	arguments.Type = true;

	DeviceIoControl(driver_handle, VRW_CODE, &arguments, sizeof(arguments), &arguments, sizeof(arguments), nullptr, nullptr);
}

template <typename T>
T communication::read(u64 address) {
	T temp = {};

	_PRW arguments = {};
	arguments.security_code = SECURITY_CODE;
	arguments.address = reinterpret_cast<void*>(address);
	arguments.buffer = &temp;
	arguments.size = sizeof(T);
	arguments.process_id = process_id;
	arguments.Type = false;

	DeviceIoControl(driver_handle, PRW_CODE, &arguments, sizeof(arguments), nullptr, NULL, NULL, NULL);

	return temp;
}

template <typename T>
void communication::read(u64 address, T& value) {
	_PRW arguments = {};
	arguments.security_code = SECURITY_CODE;
	arguments.address = reinterpret_cast<void*>(address);
	arguments.buffer = (void*)&value;
	arguments.size = sizeof(T);
	arguments.process_id = process_id;
	arguments.Type = true;

	DeviceIoControl(driver_handle, PRW_CODE, &arguments, sizeof(arguments), nullptr, NULL, NULL, NULL);
}
