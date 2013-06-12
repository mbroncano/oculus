//
//  opencl_debug.h
//  Oculus
//
//  Created by Manuel Broncano Rodriguez on 6/12/13.
//  Copyright (c) 2013 Manuel Broncano Rodriguez. All rights reserved.
//

#ifndef Oculus_opencl_debug_h
#define Oculus_opencl_debug_h

using std::cout;
using std::cerr;
using std::string;
using std::endl;

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
    << "[CL]  * Work groups: " << d->getInfo<CL_DEVICE_MAX_WORK_GROUP_SIZE>() << endl
    << "[CL]  * Image support: " << d->getInfo<CL_DEVICE_IMAGE_SUPPORT>() << endl;
}

#define HAS_MASK(a, mask) if ((a & mask) == mask)

static const char *kernelArgAddress(unsigned int a) {
    string ret;
    
    HAS_MASK(a, CL_KERNEL_ARG_ADDRESS_GLOBAL) ret += "global ";
    HAS_MASK(a, CL_KERNEL_ARG_ADDRESS_LOCAL) ret += "local ";
    HAS_MASK(a, CL_KERNEL_ARG_ADDRESS_CONSTANT) ret += "constant ";
    HAS_MASK(a, CL_KERNEL_ARG_ADDRESS_PRIVATE) ret += "private ";
    
    return ret.c_str();
}

static const char *kernelArgAccess(unsigned int a) {
    string ret;
    
    HAS_MASK(a, CL_KERNEL_ARG_ACCESS_READ_ONLY) ret += "read ";
    HAS_MASK(a, CL_KERNEL_ARG_ACCESS_WRITE_ONLY) ret += "write ";
    HAS_MASK(a, CL_KERNEL_ARG_ACCESS_READ_WRITE) ret += "read/write ";
    HAS_MASK(a, CL_KERNEL_ARG_ACCESS_NONE) ret += "none ";
    
    return ret.c_str();
}

static const char *kernelArgType(unsigned long long a) {
    string ret;
    
    HAS_MASK(a, CL_KERNEL_ARG_TYPE_NONE) ret += "none ";
    HAS_MASK(a, CL_KERNEL_ARG_TYPE_CONST) ret += "const ";
    HAS_MASK(a, CL_KERNEL_ARG_TYPE_RESTRICT) ret += "restrict ";
    HAS_MASK(a, CL_KERNEL_ARG_TYPE_VOLATILE) ret += "volatile ";
    
    return ret.c_str();
}

static void kernelArgDump(Kernel *k, int i) {
    cout
    << "[CL]  = Name: " << k->getArgInfo<CL_KERNEL_ARG_NAME>(i) << endl
    << "[CL]    = Type: " << k->getArgInfo<CL_KERNEL_ARG_TYPE_NAME>(i) << endl
    << "[CL]    = Type qualifier: " << kernelArgType(k->getArgInfo<CL_KERNEL_ARG_TYPE_QUALIFIER>(i)) << endl
    << "[CL]    = Access qualifier: " << kernelArgAccess(k->getArgInfo<CL_KERNEL_ARG_ACCESS_QUALIFIER>(i)) << endl
    << "[CL]    = Address qualifier: " << kernelArgAddress(k->getArgInfo<CL_KERNEL_ARG_ADDRESS_QUALIFIER>(i)) << endl;
}

static void kernelDump(Kernel *k) {
    cout
    << "[CL]  Kernel: " << k->getInfo<CL_KERNEL_FUNCTION_NAME>() << endl
    << "[CL]  * Args: " << k->getInfo<CL_KERNEL_NUM_ARGS>() << endl;
    // it doesn't work properly
    /*
     for (int i = 0; i < k->getInfo<CL_KERNEL_NUM_ARGS>(); i ++) {
     kernelArgDump(k, i);
     }
     */
}

static const char *programBuildStatus(int s) {
    if (s == 0) return "success";
    if (s == -1) return "none";
    if (s == -2) return "error";
    if (s == -3) return "in progress";
    return "unknown";
}

static void programBuildDump(Program *program, Device *d) {
    cerr << "[CL]  Build Status: " << programBuildStatus(program->getBuildInfo<CL_PROGRAM_BUILD_STATUS>(*d)) << endl;
    cerr << "[CL]  Build Options: " << program->getBuildInfo<CL_PROGRAM_BUILD_OPTIONS>(*d) << endl;
    string log = program->getBuildInfo<CL_PROGRAM_BUILD_LOG>(*d);
    if (log.size() > 0)
        cerr << "[CL]  Build Log\n\n" << program->getBuildInfo<CL_PROGRAM_BUILD_LOG>(*d) << endl;
}

static void errorDump(Error err) {
    cerr << "[CL]  Error: " << err.what() << "(" << err.err() << ")" << endl;
}


#endif
