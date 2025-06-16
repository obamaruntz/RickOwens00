#pragma once
#include <string>
#include <thread>
#include <chrono>

#define PRW_CODE CTL_CODE(FILE_DEVICE_UNKNOWN, 0x2ec33, METHOD_BUFFERED, FILE_SPECIAL_ACCESS)
#define VRW_ATTACH_CODE CTL_CODE(FILE_DEVICE_UNKNOWN, 0x2ec34, METHOD_BUFFERED, FILE_SPECIAL_ACCESS)
#define VRW_CODE CTL_CODE(FILE_DEVICE_UNKNOWN, 0x2ec35, METHOD_BUFFERED, FILE_SPECIAL_ACCESS)
#define BA_CODE CTL_CODE(FILE_DEVICE_UNKNOWN, 0x2ec36, METHOD_BUFFERED, FILE_SPECIAL_ACCESS)  
#define SECURITY_CODE 0x94c9e4bc3

#define sleep_ms(ms) std::this_thread::sleep_for(std::chrono::milliseconds(ms))

typedef char i8;
typedef short i16;
typedef int i32;
typedef long long i64;

typedef unsigned char u8;
typedef unsigned short u16;
typedef unsigned int u32;
typedef unsigned long long u64;

typedef float f32;
typedef double f64;

typedef const char* cstr;
typedef std::string str;

struct _PRW {
	u64 securityCode;

	i32 processId;
	void* address;
	void* buffer;

	u64 size;
	u64 returnSize;

	bool type;
};

struct _VRW {
	u64 securityCode;

	HANDLE processHandle;
	void* address;
	void* buffer;

	u64 size;
	u64 returnSize;

	bool type;
};

struct _BA {
	u64 securityCode;

	i32 processId;
	u64* address;
};
