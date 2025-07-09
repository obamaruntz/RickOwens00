#pragma once

#include "winver.h"

// 名称常量
#define DRIVER_NAME   L"\\Driver\\RickOwens00"
#define DEVICE_NAME   L"\\Device\\RickOwens00"
#define SYMBOLIC_LINK L"\\DosDevices\\RickOwens00"

// 代码
#define PRW_CODE CTL_CODE(FILE_DEVICE_UNKNOWN, 0x2ec33, METHOD_BUFFERED, FILE_SPECIAL_ACCESS) // 物理读写
#define VRW_ATTACH_CODE CTL_CODE(FILE_DEVICE_UNKNOWN, 0x2ec34, METHOD_BUFFERED, FILE_SPECIAL_ACCESS) // 虚拟读写附加代码
#define VRW_CODE CTL_CODE(FILE_DEVICE_UNKNOWN, 0x2ec35, METHOD_BUFFERED, FILE_SPECIAL_ACCESS) // 虚拟读写
#define BA_CODE CTL_CODE(FILE_DEVICE_UNKNOWN, 0x2ec36, METHOD_BUFFERED, FILE_SPECIAL_ACCESS)  // 基址
#define GR_CODE CTL_CODE(FILE_DEVICE_UNKNOWN, 0x2ec37, METHOD_BUFFERED, FILE_SPECIAL_ACCESS)  // 获取守卫区域
#define HF_CODE CTL_CODE(FILE_DEVICE_UNKNOWN, 0x2ec38, METHOD_BUFFERED, FILE_SPECIAL_ACCESS)  // TODO
#define SECURITY_CODE 0x94c9e4bc3 // 安全代码可防止未经授权的访问

// PMASK 例子 '0xFFFFFFFFFFFFFF00'
static const UINT64 PMASK = (~0xfull << 8) & 0xfffffffffull;

// 请求结构
typedef struct _PRW {
	// 安全码
	UINT64 SecurityCode;

	// 实际数据
	UINT32 ProcessID;
	UINT64 Address;
	UINT64 Buffer;
	SIZE_T Size;
	SIZE_T ReturnSize;

	BOOLEAN Type;

	// 变量和指针声明
} physical_rw, * p_physical_rw;

typedef struct _VRW {
	// 安全码
	UINT64 SecurityCode;

	// 实际数据
	HANDLE ProcessHandle;
	PVOID Address;
	PVOID Buffer;
	SIZE_T Size;
	SIZE_T return_size;

	BOOLEAN Type;

	// 变量和指针声明
} virtual_rw, * p_virtual_rw;

typedef struct _BA {
	// 安全码
	UINT64 SecurityCode;

	// 实际数据
	INT32 process_id;
	ULONGLONG* Address;    // (out)

	// 变量和指针声明
} base_address, * p_base_address;

typedef struct _GR {
	// 安全码
	UINT64 SecurityCode;

	// 实际数据
	ULONGLONG* Address;    // (out)

	// 变量和指针声明
} guarded_region, * p_guarded_region;

typedef struct _hf {
	// 安全码
	UINT64 security_code;

	// 实际数据
	INT32 process_id;

	// 变量和指针声明
} hide_file, * p_hide_file;

// 未记录的 Win32 API 函数定义
typedef struct _SYSTEM_BIGPOOL_ENTRY {
	PVOID VirtualAddress;
	ULONG_PTR NonPaged : 1;
	ULONG_PTR SizeInBytes;
	UCHAR Tag[4];
} SYSTEM_BIGPOOL_ENTRY, * PSYSTEM_BIGPOOL_ENTRY;

typedef struct _SYSTEM_BIGPOOL_INFORMATION {
	ULONG Count;
	SYSTEM_BIGPOOL_ENTRY AllocatedInfo[1];
} SYSTEM_BIGPOOL_INFORMATION, * PSYSTEM_BIGPOOL_INFORMATION;

typedef enum _SYSTEM_INFORMATION_CLASS {
	SystemBigPoolInformation = 0x42,
} SYSTEM_INFORMATION_CLASS;

extern "C" {
	NTKERNELAPI NTSTATUS IoCreateDriver(
		PUNICODE_STRING DriverName,
		PDRIVER_INITIALIZE InitializationFunction
	);

	NTKERNELAPI NTSTATUS MmCopyVirtualMemory(
		PEPROCESS SourceProcess,
		PVOID SourceAddress,
		PEPROCESS TargetProcess,
		PVOID TargetAddress,
		SIZE_T BufferSize,
		KPROCESSOR_MODE PreviousMode,
		PSIZE_T ReturnSize
	);

	PVOID NTAPI PsGetProcessSectionBaseAddress(
		PEPROCESS Process
	);

	NTSTATUS NTAPI ZwQuerySystemInformation(
		SYSTEM_INFORMATION_CLASS SystemInformationClass,
		PVOID SystemInformation,
		ULONG SystemInformationLength,
		PULONG ReturnLength
	);
}

namespace driver_functions {
	// 开始 (Physical Functions)
	NTSTATUS read_physical(PVOID Address, PVOID buffer, SIZE_T size, SIZE_T* bytes_read) {
		MM_COPY_ADDRESS readable = { 0 };
		readable.PhysicalAddress.QuadPart = (LONGLONG)Address;

		return MmCopyMemory(buffer, readable, size, MM_COPY_MEMORY_PHYSICAL, bytes_read);
	}

	NTSTATUS write_physical(PVOID Address, PVOID buffer, SIZE_T size, SIZE_T* bytes_read) {
		if (!Address) return STATUS_UNSUCCESSFUL;

		PHYSICAL_ADDRESS writable = { 0 };
		writable.QuadPart = LONGLONG(Address);

		PVOID mapped_memory = MmMapIoSpaceEx(writable, size, PAGE_READWRITE);
		if (!mapped_memory) return STATUS_UNSUCCESSFUL;

		memcpy(mapped_memory, buffer, size);
		*bytes_read = size;

		MmUnmapIoSpace(mapped_memory, size);

		return STATUS_SUCCESS;
	}
	// 结尾
}

namespace utility_functions {
	// 获取进程的 CR3（页表基址）
	UINT64 get_process_cr3(PEPROCESS process) {
		PUCHAR process_byte = (PUCHAR)process;
		ULONG_PTR process_directory_base = *(PULONG_PTR)(process_byte + 0x28);

		if (process_directory_base == 0) {
			INT32 directory_table_base = get_windows_version();

			ULONG_PTR process_directory_table_base = *(PULONG_PTR)(process_byte + directory_table_base);
			return process_directory_table_base;
		}

		return process_directory_base;
	}

	UINT64 translate_virtual_address(UINT64 directory_table_base, UINT64 virtual_address) {
		directory_table_base &= ~0xf; // 对齐目录表

		UINT64 page_offset = virtual_address & ~(~0ul << 12); // 12 = 页面偏移大小

		UINT64 pte = ((virtual_address >> 12) & (0x1ffll));   // 'page table entry'
		UINT64 pt = ((virtual_address >> 21) & (0x1ffll));    // 'page table'
		UINT64 pd = ((virtual_address >> 30) & (0x1ffll));    // 'page directory'
		UINT64 pdp = ((virtual_address >> 39) & (0x1ffll));   // 'page directory pointer'

		SIZE_T buffer = 0;

		UINT64 _pdp = 0;
		driver_functions::read_physical(PVOID(directory_table_base + 8 * pdp), &_pdp, sizeof(_pdp), &buffer);
		if (~_pdp & 1) return 0;

		UINT64 pde = 0;
		driver_functions::read_physical(PVOID((_pdp & PMASK) + 8 * pd), &pde, sizeof(pde), &buffer);
		if (~pde & 1) return 0;

		if (pde & 0x80) // 处理 1GB 页面
			return (pde & (~0ull << 42 >> 12)) + (virtual_address & ~(~0ull << 30));

		UINT64 _pte = 0;
		driver_functions::read_physical(PVOID((pde & PMASK) + 8 * pt), &_pte, sizeof(_pte), &buffer);
		if (~_pte & 1) return 0;

		if (_pte & 0x80) // 通过 PTE 处理 2MB 页面
			return (_pte & PMASK) + (virtual_address & ~(~0ull << 21));

		// 正常 4KB 页面 — 读取最终页表条目
		virtual_address = 0;
		driver_functions::read_physical(PVOID((_pte & PMASK) + 8 * pte), &virtual_address, sizeof(virtual_address), &buffer);
		virtual_address &= PMASK;

		if (!virtual_address)
			return 0;

		return virtual_address + page_offset;
	}
}

namespace io_handlers {
	NTSTATUS handle_physical_request(p_physical_rw request) {
		if (request->SecurityCode != SECURITY_CODE) return STATUS_UNSUCCESSFUL;
		if (!request->ProcessID) return STATUS_UNSUCCESSFUL;

		PEPROCESS process = NULL;
		PsLookupProcessByProcessId((HANDLE)request->ProcessID, &process); // 根据进程ID查找进程
		if (!process) return STATUS_UNSUCCESSFUL;

		ULONGLONG process_base = utility_functions::get_process_cr3(process);
		ObDereferenceObject(process);

		SIZE_T this_offset = NULL;
		SIZE_T total_size = request->Size;

		// 使用进程的 CR3 将虚拟地址转换为物理地址
		UINT64 physical_address = utility_functions::translate_virtual_address(process_base, (ULONG64)request->Address + this_offset);
		if (!physical_address)
			return STATUS_UNSUCCESSFUL;

		// 计算最终读取或写入的大小，确保不超过页面边界
		ULONG64 final_size = (
			((PAGE_SIZE - (physical_address & 0xFFF)) < (SIZE_T)total_size) ?
			(PAGE_SIZE - (physical_address & 0xFFF)) : (SIZE_T)total_size
		);

		if (request->Type) // 如果类型等于 'true'，则改为写
			driver_functions::write_physical(PVOID(physical_address), (PVOID)((ULONG64)request->Buffer + this_offset), final_size, &request->ReturnSize);
		else
			driver_functions::read_physical(PVOID(physical_address), (PVOID)((ULONG64)request->Buffer + this_offset), final_size, &request->ReturnSize);

		return STATUS_SUCCESS;
	}

	NTSTATUS handle_base_address_request(p_base_address request) {
		if (request->SecurityCode != SECURITY_CODE) return STATUS_UNSUCCESSFUL;
		if (!request->process_id) return STATUS_UNSUCCESSFUL;

		PEPROCESS process = NULL;
		NTSTATUS status = PsLookupProcessByProcessId((HANDLE)request->process_id, &process);

		if (!NT_SUCCESS(status))
			return status;


		// 获取进程映像的基地址（即其可执行文件在内存中的加载位置）
		ULONGLONG image_base = (ULONGLONG)PsGetProcessSectionBaseAddress(process);
		if (!image_base) return STATUS_UNSUCCESSFUL;

		// 将镜像基地址复制到请求结构中的“地址”字段
		RtlCopyMemory(request->Address, &image_base, sizeof(image_base));
		ObDereferenceObject(process);

		return STATUS_SUCCESS;
	}

	NTSTATUS get_gaurded_region(p_guarded_region request) {
		if (request->SecurityCode != SECURITY_CODE) return STATUS_UNSUCCESSFUL;

		ULONG info_size = 0;
		NTSTATUS status = ZwQuerySystemInformation(SystemBigPoolInformation, &info_size, 0, &info_size);

		if (!NT_SUCCESS(status))
			return status;

		// 为系统池信息分配内存
		PSYSTEM_BIGPOOL_INFORMATION pool_info = 0;
		while (status == STATUS_INFO_LENGTH_MISMATCH) {
			if (pool_info) ExFreePool(pool_info);
			pool_info = (PSYSTEM_BIGPOOL_INFORMATION)ExAllocatePool(NonPagedPool, info_size);
			status = ZwQuerySystemInformation(SystemBigPoolInformation, pool_info, info_size, &info_size);
		}

		// 迭代池条目
		if (pool_info) {
			for (unsigned int i = 0; i < pool_info->Count; i++) {
				SYSTEM_BIGPOOL_ENTRY* entry = &pool_info->AllocatedInfo[i];
				PVOID virtual_address;
				virtual_address = (PVOID)((uintptr_t)entry->VirtualAddress & ~1ull);
				SIZE_T size_bytes = entry->SizeInBytes;
				BOOLEAN is_non_paged = entry->NonPaged;

				// 检查非分页、特定大小和标签
				if (entry->NonPaged && entry->SizeInBytes == 0x200000) {
					UCHAR expected_tag[] = "TnoC"; // 标签应该是一个字符串，而不是无符号长整型
					if (memcmp(entry->Tag, expected_tag, sizeof(expected_tag)) == 0) {
						RtlCopyMemory((void*)request->Address, &entry->VirtualAddress, sizeof(entry->VirtualAddress));
						return STATUS_SUCCESS;
					}
				}
			}

			ExFreePool(pool_info);
		}

		return STATUS_SUCCESS;
	}

	NTSTATUS handle_hide_file(p_hide_file request) {
		if (request->security_code != SECURITY_CODE) return STATUS_UNSUCCESSFUL;
		if (!request->process_id) return STATUS_UNSUCCESSFUL;

		PEPROCESS process = NULL;
		NTSTATUS status = PsLookupProcessByProcessId((HANDLE)request->process_id, &process);

		if (!NT_SUCCESS(status))
			return status;

		PLIST_ENTRY active_process_links = ((PLIST_ENTRY)((PCHAR)process + 0x448));
		active_process_links->Blink->Flink = active_process_links->Flink;
		active_process_links->Flink->Blink = active_process_links->Blink;

		return STATUS_SUCCESS;
	}
}

namespace major_functions {
	// MJ_CREATE 和 MJ_CLOSE 的默认调度程序
	NTSTATUS dispatcher(PDEVICE_OBJECT device_object, PIRP irp) {
		UNREFERENCED_PARAMETER(device_object);

		IoCompleteRequest(irp, IO_NO_INCREMENT);

		return irp->IoStatus.Status;
	}

	// 主 I/O 处理程序用于处理我们的虚拟/物理/基地址/鼠标请求
	NTSTATUS io_controller(PDEVICE_OBJECT device_object, PIRP irp) {
		UNREFERENCED_PARAMETER(device_object);

		if (KeGetCurrentIrql() != PASSIVE_LEVEL) { // crash fix
			irp->IoStatus.Status = STATUS_UNSUCCESSFUL;
			irp->IoStatus.Information = 0;
			IoCompleteRequest(irp, IO_NO_INCREMENT);
			return STATUS_UNSUCCESSFUL;
		}

		NTSTATUS status = STATUS_UNSUCCESSFUL;
		ULONG bytes = {};

		PIO_STACK_LOCATION stack = IoGetCurrentIrpStackLocation(irp);
		ULONG code = stack->Parameters.DeviceIoControl.IoControlCode;
		ULONG size = stack->Parameters.DeviceIoControl.InputBufferLength;

		// 用于虚拟函数
		static PEPROCESS virtual_target_process = nullptr;

		if (code == PRW_CODE) {
			if (size == sizeof(_PRW)) {
				p_physical_rw request = (p_physical_rw)(irp->AssociatedIrp.SystemBuffer);

				status = io_handlers::handle_physical_request(request);
				bytes = sizeof(_PRW);
			}
			else {
				status = STATUS_INFO_LENGTH_MISMATCH;
				bytes = 0;
			}
		}
		else if (code == VRW_ATTACH_CODE) {
			if (size == sizeof(_VRW)) {
				p_virtual_rw request = (p_virtual_rw)(irp->AssociatedIrp.SystemBuffer);

				status = PsLookupProcessByProcessId(request->ProcessHandle, &virtual_target_process);
				bytes = sizeof(_VRW);
			}
			else {
				status = STATUS_INFO_LENGTH_MISMATCH;
				bytes = 0;
			}
		}
		else if (code == VRW_CODE) {
			if (size == sizeof(_VRW)) {
				p_virtual_rw request = (p_virtual_rw)(irp->AssociatedIrp.SystemBuffer);

				if (request->Type) {
					if (virtual_target_process != nullptr)
						status = MmCopyVirtualMemory(PsGetCurrentProcess(), request->Buffer, virtual_target_process, request->Address, request->Size, KernelMode, &request->return_size);
				}
				else {
					if (virtual_target_process != nullptr)
						status = MmCopyVirtualMemory(virtual_target_process, request->Address, PsGetCurrentProcess(), request->Buffer, request->Size, KernelMode, &request->return_size);
				}

				bytes = sizeof(_VRW);
			}
			else {
				status = STATUS_INFO_LENGTH_MISMATCH;
				bytes = 0;
			}
		}
		else if (code == BA_CODE) {
			if (size == sizeof(_BA)) {
				p_base_address request = (p_base_address)(irp->AssociatedIrp.SystemBuffer);

				status = io_handlers::handle_base_address_request(request);
				bytes = sizeof(_BA);
			}
			else {
				status = STATUS_INFO_LENGTH_MISMATCH;
				bytes = 0;
			}
		}
		else if (code == GR_CODE) {
			if (size == sizeof(_GR)) {
				p_guarded_region request = (p_guarded_region)(irp->AssociatedIrp.SystemBuffer);

				status = io_handlers::get_gaurded_region(request);
				bytes = sizeof(_GR);
			}
			else {
				status = STATUS_INFO_LENGTH_MISMATCH;
				bytes = 0;
			}
		}
		else if (code == HF_CODE) {
			if (size == sizeof(_hf)) {
				p_hide_file request = (p_hide_file)(irp->AssociatedIrp.SystemBuffer);

				status = io_handlers::handle_hide_file(request);
				bytes = sizeof(_hf);
			}
			else {
				status = STATUS_INFO_LENGTH_MISMATCH;
				bytes = 0;
			}
		}
		else {
			status = STATUS_INVALID_DEVICE_REQUEST;
			bytes = 0;
		}

		irp->IoStatus.Status = status;
		irp->IoStatus.Information = bytes;
		IoCompleteRequest(irp, IO_NO_INCREMENT);

		return status;
	}
}
