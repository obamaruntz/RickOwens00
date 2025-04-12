#include <ntifs.h>
#include "definitions.h"

NTSTATUS driver_initialize(PDRIVER_OBJECT driver_object, PUNICODE_STRING registry_path) {
    UNREFERENCED_PARAMETER(registry_path);

    NTSTATUS status = STATUS_UNSUCCESSFUL;

    UNICODE_STRING device_name = {};
    RtlInitUnicodeString(&device_name, DEVICE_NAME);

    PDEVICE_OBJECT device_object = nullptr;

    status = IoCreateDevice(driver_object, NULL, &device_name, FILE_DEVICE_UNKNOWN, FILE_DEVICE_SECURE_OPEN, FALSE, &device_object);
    if (!NT_SUCCESS(status)) return status;

    UNICODE_STRING symbolic_link = {};
    RtlInitUnicodeString(&symbolic_link, SYMBOLIC_LINK);

    status = IoCreateSymbolicLink(&symbolic_link, &device_name);
    if (!NT_SUCCESS(status)) return status;

    SetFlag(device_object->Flags, DO_BUFFERED_IO);

    driver_object->MajorFunction[IRP_MJ_CREATE] = major_functions::dispatcher;
    driver_object->MajorFunction[IRP_MJ_CLOSE] = major_functions::dispatcher;
    driver_object->MajorFunction[IRP_MJ_DEVICE_CONTROL] = major_functions::io_controller;
    driver_object->DriverUnload = nullptr;

    ClearFlag(device_object->Flags, DO_DIRECT_IO);
    ClearFlag(device_object->Flags, DO_DEVICE_INITIALIZING);

    return STATUS_SUCCESS;
}

NTSTATUS DriverEntry() {
    UNICODE_STRING driver_name = {};
    RtlInitUnicodeString(&driver_name, DRIVER_NAME);

    return IoCreateDriver(&driver_name, &driver_initialize);
}