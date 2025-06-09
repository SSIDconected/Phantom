//
//  kern_ioreg.cpp
//  Phantom
//
//  Created by RoyalGraphX on 6/7/25.
//

#include "kern_ioreg.hpp"

// Module-specific Filtered Process List
const PHTM::DetectedProcess IOR::filteredProcs[] = {
    { "Terminal", 0 },
    { "ioreg", 0 },
    { "LeagueClient", 0 },
    { "LeagueofLegends", 0 },
    { "LeagueClientUx H", 0 },
    { "RiotClientServic", 0 }
};

// --- Helper Function ---
static bool isProcFiltered(const char *procName) {
    if (!procName) {
        return false;
    }
    const size_t num_filtered = sizeof(IOR::filteredProcs) / sizeof(IOR::filteredProcs[0]);
    for (size_t i = 0; i < num_filtered; ++i) {
        if (strcmp(procName, IOR::filteredProcs[i].name) == 0) {
            return true;
        }
    }
    return false;
}

// Static pointers to hold the original function addresses
static IOR::_IORegistryEntry_getProperty_t original_IORegistryEntry_getProperty_os_symbol = nullptr;
static IOR::_IORegistryEntry_getProperty_cstring_t original_IORegistryEntry_getProperty_cstring = nullptr;
static IOR::_IORegistryEntry_getName_t original_IORegistryEntry_getName = nullptr;
static IOR::_IOIterator_getNextObject_t original_IOIterator_getNextObject = nullptr;

// Forward declaration for our main hook
OSObject *phtm_IORegistryEntry_getProperty_os_symbol(const IORegistryEntry *that, const OSSymbol *aKey);

// This is the new hook for the getProperty(const char*) variant.
OSObject *phtm_IORegistryEntry_getProperty_cstring(const IORegistryEntry *that, const char *aKey) {
    const OSSymbol *symbolKey = OSSymbol::withCString(aKey);
    OSObject *result = phtm_IORegistryEntry_getProperty_os_symbol(that, symbolKey);
    
    if (symbolKey) {
        const_cast<OSSymbol *>(symbolKey)->release();
    }
    return result;
}

// This is our main, updated hook for getProperty(const OSSymbol*).
OSObject *phtm_IORegistryEntry_getProperty_os_symbol(const IORegistryEntry *that, const OSSymbol *aKey) {
	
    // Always call the original function first to get the real property.
    OSObject* original_property = nullptr;
    if (original_IORegistryEntry_getProperty_os_symbol) {
        original_property = original_IORegistryEntry_getProperty_os_symbol(that, aKey);
    }
    
    proc_t p = current_proc();
    pid_t pid = proc_pid(p);
    char procName[MAX_PROC_NAME_LEN];
    proc_name(pid, procName, sizeof(procName));

    // Check if the process is one we want to target using our new helper function.
    if (isProcFiltered(procName))
    {
        const char *keyName = aKey->getCStringNoCopy();

        // If the key is "manufacturer", spoof it regardless of the class.
        if (keyName && strcmp(keyName, "manufacturer") == 0)
        {
            const char* entryClassName = that->getMetaClass()->getClassName();
            DBGLOG(MODULE_IOR, "'manufacturer' key on class '%s' called by '%s (PID: %d). Returning 'Apple Inc.'.",
                   entryClassName, procName, pid);
            
            // Return our new, spoofed OSString.
            return OSString::withCString("Apple Inc.");
        }
        else // If its not manufact, then lets go ahead and log what was being asked and what was returned
        {
            const char* entryClassName = that->getMetaClass()->getClassName();
            OSString* str_prop = OSDynamicCast(OSString, original_property);
            OSData* data_prop = OSDynamicCast(OSData, original_property);

            if (str_prop) {
                 DBGLOG(MODULE_IOR, "getProperty called by '%s (PID: %d) on class '%s' for key '%s' returned OSString: '%s'",
                       procName, pid, entryClassName, keyName, str_prop->getCStringNoCopy());
            } else if (data_prop) {
                 // Often, strings are stored in OSData. We can try to print it if it's null-terminated.
                 DBGLOG(MODULE_IOR, "getProperty called by '%s (PID: %d) on class '%s' for key '%s' returned OSData: '%.*s' (size: %d)",
                       procName, pid, entryClassName, keyName, (int)data_prop->getLength(), (const char*)data_prop->getBytesNoCopy(), data_prop->getLength());
            } else {
                 DBGLOG(MODULE_IOR, "getProperty called by '%s (PID: %d) on class '%s' for key '%s' returned object of class '%s'",
                       procName, pid, entryClassName, keyName,
                       original_property ? original_property->getMetaClass()->getClassName() : "nullptr");
            }
        }
    }

    // For all other cases, just return the original property without logging.
    return original_property;
}

// Logs only for specified processes with more detail.
const char *phtm_IORegistryEntry_getName(const IORegistryEntry *that, const IORegistryPlane *plane) {
    const char *entryName = nullptr;
    if (original_IORegistryEntry_getName) {
        entryName = original_IORegistryEntry_getName(that, plane);
    }

    proc_t p = current_proc();
    pid_t pid = proc_pid(p);
    char procName[MAX_PROC_NAME_LEN];
    proc_name(pid, procName, sizeof(procName));

    // Only log if the process name matches one on our allow list, using the helper.
    if (isProcFiltered(procName))
    {
        // Get the class name for a more descriptive log message.
        const char* entryClassName = that->getMetaClass()->getClassName();
        DBGLOG(MODULE_IOR, "getName called by '%s (PID: %d) on class '%s', returning name: '%s'",
               procName,
               pid,
               entryClassName ? entryClassName : "Unknown",
               entryName ? entryName : "Unknown");
    }

    return entryName;
}

// Custom getNextObject: Logs only for specified processes.
OSObject *phtm_IOIterator_getNextObject(OSCollectionIterator *that) {
    OSObject *obj = nullptr;
    if (original_IOIterator_getNextObject) {
        obj = original_IOIterator_getNextObject(that);
    }

    proc_t p = current_proc();
    pid_t pid = proc_pid(p);
    char procName[MAX_PROC_NAME_LEN];
    proc_name(pid, procName, sizeof(procName));
	
    // Only log if the process name matches one on our allow list, using the helper.
    if (isProcFiltered(procName))
    {
        DBGLOG(MODULE_IOR, "getNextObject called by '%s' (PID: %d) on an IOIterator, returning object of class '%s'",
               procName,
               pid,
               obj ? obj->getMetaClass()->getClassName() : "nullptr");
    }
    
    return obj;
}

// This function will be called by our onKextLoad callback when IOACPIFamily is loaded
// NOTE: This is fucking horrible, theres a real reason why we do NOT use Apple Inc. globally, filtering is our only option
//static void modify_ioregistry_properties(void *user, KernelPatcher &patcher, size_t id, mach_vm_address_t, size_t) {
//    DBGLOG(MODULE_IOR, "IOACPIFamily loaded, now attempting to modify IORegistry properties.");
//
//    // The IOPlatformExpertDevice is at the root of the device tree plane.
//    IORegistryEntry *root = IORegistryEntry::fromPath("/", &gIODTPlane);
//    if (root) {
//        DBGLOG(MODULE_IOR, "Found IORegistry root: %s", root->getName());
//        
//        // Overwrite the 'manufacturer' property. This is more reliable than hooking getProperty.
//        if (root->setProperty("manufacturer", "Apple Inc.")) {
//            DBGLOG(MODULE_IOR, "Successfully set 'manufacturer' to 'Apple Inc.' on '%s'.", root->getName());
//        } else {
//            DBGLOG(MODULE_ERROR, "Failed to set 'manufacturer' property on '%s'.", root->getName());
//        }
//        root->release();
//    } else {
//        DBGLOG(MODULE_ERROR, "Failed to get IORegistry root even after IOACPIFamily was loaded.");
//    }
//}

// Generic rerouting function for simplicity
static bool reRoute(KernelPatcher &patcher, const char *mangledName, mach_vm_address_t replacement, void **original) {
    mach_vm_address_t address = patcher.solveSymbol(KernelPatcher::KernelID, mangledName);
    if (address) {
        DBGLOG(MODULE_RRIOR, "Resolved %s at 0x%llx", mangledName, address);
        *original = reinterpret_cast<void*>(patcher.routeFunction(address, replacement, true));

        if (patcher.getError() == KernelPatcher::Error::NoError && *original) {
            DBGLOG(MODULE_RRIOR, "Successfully routed %s", mangledName);
            return true;
        } else {
            DBGLOG(MODULE_ERROR, "Failed to route %s. Lilu error: %d", mangledName, patcher.getError());
            patcher.clearError();
        }
    } else {
        DBGLOG(MODULE_ERROR, "Failed to resolve symbol %s. Lilu error: %d", mangledName, patcher.getError());
        patcher.clearError();
    }
    return false;
}

// IORegistry Module Initialization
void IOR::init(KernelPatcher &Patcher) {
    DBGLOG(MODULE_IOR, "IOR::init(Patcher) called. IORegistry module is starting.");
    
    // Reroute functions, Seq specific, it's 4am give me a break, ts free and oss
    reRoute(Patcher, "__ZNK15IORegistryEntry11getPropertyEPK8OSSymbol",
            reinterpret_cast<mach_vm_address_t>(phtm_IORegistryEntry_getProperty_os_symbol),
            reinterpret_cast<void**>(&original_IORegistryEntry_getProperty_os_symbol));
    
    reRoute(Patcher, "__ZNK15IORegistryEntry11getPropertyEPKc",
            reinterpret_cast<mach_vm_address_t>(phtm_IORegistryEntry_getProperty_cstring),
            reinterpret_cast<void**>(&original_IORegistryEntry_getProperty_cstring));
            
    reRoute(Patcher, "__ZNK15IORegistryEntry7getNameEPK15IORegistryPlane",
            reinterpret_cast<mach_vm_address_t>(phtm_IORegistryEntry_getName),
            reinterpret_cast<void**>(&original_IORegistryEntry_getName));

    reRoute(Patcher, "__ZN20OSCollectionIterator13getNextObjectEv",
            reinterpret_cast<mach_vm_address_t>(phtm_IOIterator_getNextObject),
            reinterpret_cast<void**>(&original_IOIterator_getNextObject));
	
    DBGLOG(MODULE_IOR, "IOR::init(Patcher) finished.");
}
