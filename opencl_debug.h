//
//  opencl_debug.h
//  Oculus
//
//  Created by Manuel Broncano Rodriguez on 6/12/13.
//  Copyright (c) 2013 Manuel Broncano Rodriguez. All rights reserved.
//

#ifndef Oculus_opencl_debug_h
#define Oculus_opencl_debug_h

#include <map>

using std::cout;
using std::cerr;
using std::string;
using std::endl;
using std::map;

#define HAS_MASK(a, mask) if ((a & mask) == mask)

static map<int, const char *> errorCode = {
    {CL_SUCCESS, "success"},
    {CL_DEVICE_NOT_FOUND, "device not found"},
    {CL_DEVICE_NOT_AVAILABLE, "device not available"},
    {CL_COMPILER_NOT_AVAILABLE, "compiler not available"},
    {CL_MEM_OBJECT_ALLOCATION_FAILURE, "mem object allocation failure"},
    {CL_OUT_OF_RESOURCES, "out of resources"},
    {CL_OUT_OF_HOST_MEMORY, "out of host memory"},
    {CL_PROFILING_INFO_NOT_AVAILABLE, "profiling info not available"},
    {CL_MEM_COPY_OVERLAP, "mem copy overlap"},
    {CL_IMAGE_FORMAT_MISMATCH, "image format mismatch"},
    {CL_IMAGE_FORMAT_NOT_SUPPORTED, "image format not supported"},
    {CL_BUILD_PROGRAM_FAILURE, "build program failure"},
    {CL_MAP_FAILURE, "map failure"},
    {CL_MISALIGNED_SUB_BUFFER_OFFSET, "misaligned sub buffer offset"},
    {CL_EXEC_STATUS_ERROR_FOR_EVENTS_IN_WAIT_LIST, "exec status error for events in wait list"},
    {CL_COMPILE_PROGRAM_FAILURE, "compile program failure"},
    {CL_LINKER_NOT_AVAILABLE, "linker not available"},
    {CL_LINK_PROGRAM_FAILURE, "link program failure"},
    {CL_DEVICE_PARTITION_FAILED, "device partition failed"},
    {CL_KERNEL_ARG_INFO_NOT_AVAILABLE, "kernel arg info not available"},
    {CL_INVALID_VALUE ,"invalid value"},
    {CL_INVALID_DEVICE_TYPE ,"invalid device type"},
    {CL_INVALID_PLATFORM ,"invalid platform"},
    {CL_INVALID_DEVICE ,"invalid device"},
    {CL_INVALID_CONTEXT ,"invalid context"},
    {CL_INVALID_QUEUE_PROPERTIES ,"invalid queue properties"},
    {CL_INVALID_COMMAND_QUEUE ,"invalid command queue"},
    {CL_INVALID_HOST_PTR ,"invalid host ptr"},
    {CL_INVALID_MEM_OBJECT ,"invalid mem object"},
    {CL_INVALID_IMAGE_FORMAT_DESCRIPTOR ,"invalid image format descriptor"},
    {CL_INVALID_IMAGE_SIZE ,"invalid image size"},
    {CL_INVALID_SAMPLER ,"invalid sampler"},
    {CL_INVALID_BINARY ,"invalid binary"},
    {CL_INVALID_BUILD_OPTIONS ,"invalid build options"},
    {CL_INVALID_PROGRAM ,"invalid program"},
    {CL_INVALID_PROGRAM_EXECUTABLE ,"invalid program executable"},
    {CL_INVALID_KERNEL_NAME ,"invalid kernel name"},
    {CL_INVALID_KERNEL_DEFINITION ,"invalid kernel definition"},
    {CL_INVALID_KERNEL ,"invalid kernel"},
    {CL_INVALID_ARG_INDEX ,"invalid arg index"},
    {CL_INVALID_ARG_VALUE ,"invalid arg value"},
    {CL_INVALID_ARG_SIZE ,"invalid arg size"},
    {CL_INVALID_KERNEL_ARGS ,"invalid kernel args"},
    {CL_INVALID_WORK_DIMENSION ,"invalid work dimension"},
    {CL_INVALID_WORK_GROUP_SIZE ,"invalid work group size"},
    {CL_INVALID_WORK_ITEM_SIZE ,"invalid work item size"},
    {CL_INVALID_GLOBAL_OFFSET ,"invalid global offset"},
    {CL_INVALID_EVENT_WAIT_LIST ,"invalid event wait list"},
    {CL_INVALID_EVENT ,"invalid event"},
    {CL_INVALID_OPERATION ,"invalid operation"},
    {CL_INVALID_GL_OBJECT ,"invalid gl object"},
    {CL_INVALID_BUFFER_SIZE ,"invalid buffer size"},
    {CL_INVALID_MIP_LEVEL ,"invalid mip level"},
    {CL_INVALID_GLOBAL_WORK_SIZE ,"invalid global work size"},
    {CL_INVALID_PROPERTY ,"invalid property"},
    {CL_INVALID_IMAGE_DESCRIPTOR ,"invalid image descriptor"},
    {CL_INVALID_COMPILER_OPTIONS ,"invalid compiler options"},
    {CL_INVALID_LINKER_OPTIONS ,"invalid linker options"},
    {CL_INVALID_DEVICE_PARTITION_COUNT ,"invalid device partition count"}
};

static map<unsigned int, const char *> localMemType = {
    {CL_LOCAL, "local"},
    {CL_GLOBAL, "global"}
};

static map<int, const char *> buildStatus = {
    {CL_BUILD_SUCCESS, "success"},
    {CL_BUILD_NONE, "none"},
    {CL_BUILD_ERROR, "error"},
    {CL_BUILD_IN_PROGRESS, "in progress"}
};

static map<int, const char *> kernelArgAddress = {
    {CL_KERNEL_ARG_ADDRESS_GLOBAL, "global"},
    {CL_KERNEL_ARG_ADDRESS_LOCAL, "local"},
    {CL_KERNEL_ARG_ADDRESS_CONSTANT, "constant"},
    {CL_KERNEL_ARG_ADDRESS_PRIVATE, "private "}
};

static map<int, const char *> kernelArgAccess = {
    {CL_KERNEL_ARG_ACCESS_READ_ONLY, "read"},
    {CL_KERNEL_ARG_ACCESS_WRITE_ONLY, "write"},
    {CL_KERNEL_ARG_ACCESS_READ_WRITE, "read/write"},
    {CL_KERNEL_ARG_ACCESS_NONE, "none"}
};

static const char *kernelArgType(unsigned long long a) {
    string ret;
    
    HAS_MASK(a, CL_KERNEL_ARG_TYPE_NONE) ret += "none";
    HAS_MASK(a, CL_KERNEL_ARG_TYPE_CONST) ret += "const";
    HAS_MASK(a, CL_KERNEL_ARG_TYPE_RESTRICT) ret += "restrict ";
    HAS_MASK(a, CL_KERNEL_ARG_TYPE_VOLATILE) ret += "volatile ";
    
    return ret.c_str();
}

static void platformDump(Platform *p) {
    cout
    << "[CL]  Platform: " << p->getInfo<CL_PLATFORM_NAME>() << endl
    << "[CL]  * Vendor: " << p->getInfo<CL_PLATFORM_VENDOR>() << endl
    << "[CL]  * Version: " << p->getInfo<CL_PLATFORM_VERSION>() << endl
    << "[CL]  * Profile: " << p->getInfo<CL_PLATFORM_PROFILE>() << endl
    << "[CL]  * Extensions: " << p->getInfo<CL_PLATFORM_EXTENSIONS>() << endl;
}

static void deviceDump(Device *d) {
    cout
    << "[CL]  Device: " << d->getInfo<CL_DEVICE_NAME>() << endl
    << "[CL]  * Type: " << d->getInfo<CL_DEVICE_TYPE>() << endl
    << "[CL]  * Version: " << d->getInfo<CL_DEVICE_VERSION>() << endl
    << "[CL]  * Extensions: " << d->getInfo<CL_DEVICE_EXTENSIONS>() << endl
    << "[CL]  * Compute units: " << d->getInfo<CL_DEVICE_MAX_COMPUTE_UNITS>() << endl
    << "[CL]  * Global mem size: " << d->getInfo<CL_DEVICE_GLOBAL_MEM_SIZE>() << endl
    << "[CL]  * Local mem size: " << d->getInfo<CL_DEVICE_LOCAL_MEM_SIZE>() << endl
    << "[CL]  * Local mem type: " << localMemType[d->getInfo<CL_DEVICE_LOCAL_MEM_TYPE>()] << endl
    << "[CL]  * Max const buffer size: " << d->getInfo<CL_DEVICE_MAX_CONSTANT_BUFFER_SIZE>() << endl
    << "[CL]  * Max const args: " << d->getInfo<CL_DEVICE_MAX_CONSTANT_ARGS>() << endl
    << "[CL]  * Work groups: " << d->getInfo<CL_DEVICE_MAX_WORK_GROUP_SIZE>() << endl
    << "[CL]  * (float) pref vec width: " << d->getInfo<CL_DEVICE_PREFERRED_VECTOR_WIDTH_FLOAT>() << endl
    << "[CL]  * (int)   pref vec width: " << d->getInfo<CL_DEVICE_PREFERRED_VECTOR_WIDTH_INT>() << endl
    << "[CL]  * (char)  pref vec width: " << d->getInfo<CL_DEVICE_PREFERRED_VECTOR_WIDTH_CHAR>() << endl
    << "[CL]  * Image support: " << d->getInfo<CL_DEVICE_IMAGE_SUPPORT>() << endl;
}

static void kernelArgDump(Kernel *k, int i) {
    cout
    << "[CL]  = Name: " << k->getArgInfo<CL_KERNEL_ARG_NAME>(i) << endl
    << "[CL]    = Type: " << k->getArgInfo<CL_KERNEL_ARG_TYPE_NAME>(i) << endl
    << "[CL]    = Type qualifier: " << kernelArgType(k->getArgInfo<CL_KERNEL_ARG_TYPE_QUALIFIER>(i)) << endl
    << "[CL]    = Access qualifier: " << kernelArgAccess[k->getArgInfo<CL_KERNEL_ARG_ACCESS_QUALIFIER>(i)] << endl
    << "[CL]    = Address qualifier: " << kernelArgAddress[k->getArgInfo<CL_KERNEL_ARG_ADDRESS_QUALIFIER>(i)] << endl;
}

static void kernelDump(Kernel *k) {
    cout
    << "[CL]  Kernel: " << k->getInfo<CL_KERNEL_FUNCTION_NAME>() << endl
    << "[CL]  * Args: " << k->getInfo<CL_KERNEL_NUM_ARGS>() << endl;
    return;
    for (int i = 0; i < k->getInfo<CL_KERNEL_NUM_ARGS>(); i ++) {
        kernelArgDump(k, i);
    }
}

static void programBuildDump(Program *program, Device *d) {
    cerr << "[CL]  Build Status: " << buildStatus[program->getBuildInfo<CL_PROGRAM_BUILD_STATUS>(*d)] << endl;
    cerr << "[CL]  Build Options: " << program->getBuildInfo<CL_PROGRAM_BUILD_OPTIONS>(*d) << endl;
    string log = program->getBuildInfo<CL_PROGRAM_BUILD_LOG>(*d);
    if (log.size() > 0)
        cerr << "[CL]  Build Log\n\n" << program->getBuildInfo<CL_PROGRAM_BUILD_LOG>(*d) << endl;
}

static void errorDump(Error err) {
    cerr << "[CL]  Error: " << err.what() << "(" << err.err() << ": " << errorCode[err.err()] << ")" << endl;
}


#endif
