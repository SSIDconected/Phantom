//
//  kern_start.hpp
//  Phantom
//
//  Created by RoyalGraphX on 10/15/24.
//

#ifndef kern_start_h
#define kern_start_h

// Phantom Includes
#include <Headers/plugin_start.hpp>
#include <Headers/kern_patcher.hpp>
#include <Headers/kern_nvram.hpp>
#include <Headers/kern_devinfo.hpp>
#include <Headers/kern_efi.hpp>
#include <Headers/kern_api.hpp>
#include <Headers/kern_util.hpp>
#include <Headers/kern_mach.hpp>
#include <mach/i386/vm_types.h>
#include <libkern/libkern.h>
#include <IOKit/IOLib.h>
#include <sys/sysctl.h>
#include <i386/cpuid.h>

// Logging Defs
#define MODULE_INIT "INIT"
#define MODULE_SHORT "PHTM"
#define MODULE_LONG "Phantom"
#define MODULE_ERROR "ERR"
#define MODULE_WARN "WARN"
#define MODULE_INFO "INFO"
#define MODULE_CUTE "\u2665"

// Function Logging Defs
#define MODULE_PPU "PPU"
#define MODULE_SYSCA "SYSCA"
#define MODULE_SSYSCTL "SSYSCTL"

// PHTM Root/Parent Class
class PHTM {
public:

    /**
     * Maximum number of processes we can track, Maximum length of a process name
     */
    #define MAX_PROCESSES 256
    #define MAX_PROC_NAME_LEN 256
	
    /**
     * Standard Init and deInit functions
     */
    void init();
    void deinit();
	
	/**
	* Publicly accessible internal build flag
	*/
	static const bool IS_INTERNAL;
	
	/**
     * @brief Globally accessible Darwin kernel version numbers.
     * Populated by PHTM::init().
     */
    static int darwinMajor;
    static int darwinMinor;
	
	/**
	* Struct to hold both process name and potential PID
	*/
	struct DetectedProcess {
		const char *name;
    	pid_t pid;
	};
	
    /**
     * @brief Stores the resolved address of the kernel's _sysctl__children list.
     * Populated by PHTM::solveSysCtlChildrenAddr.
     */
    static mach_vm_address_t gSysctlChildrenAddr;
	
    /**
     * @brief Resolves the address of the kernel's _sysctl__children list.
     * This function directly uses the KernelPatcher to find the symbol.
     * @param patcher A reference to the KernelPatcher instance.
     * @return The address of _sysctl__children, or 0 if not found.
     */
    static mach_vm_address_t sysctlChildrenAddr(KernelPatcher &patcher);
	
    /**
     * @brief Callback for Lilu's onPatcherLoad. Solves for and stores _sysctl__children address.
     * This function calls PHTM::sysctlChildrenAddr and stores the result inPHTMH::gSysctlChildrenAddr.
     * @param user User-defined pointer (unused).
     * @param Patcher A reference to the KernelPatcher instance.
     */
    static void solveSysCtlChildrenAddr(void *user, KernelPatcher &Patcher);
	
private:

    /**
     *  Private self instance for callbacks
     */
    static PHTM *callbackPHTM;

};

#endif /* kern_start_hpp */

#ifndef PHTM_VERSION /*PHTMH_VERSION Macro */
#define PHTM_VERSION "Unknown"

#endif /* PHTM_VERSION Macro */
