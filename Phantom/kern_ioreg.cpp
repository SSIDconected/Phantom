//
//  kern_ioreg.cpp
//  Phantom
//
//  Created by RoyalGraphX on 6/7/25.
//

#include "kern_ioreg.hpp"

// Static pointers to hold the original function addresses
static IOR::_IORegistryEntry_getProperty_t original_IORegistryEntry_getProperty_os_symbol = nullptr;
static IOR::_IORegistryEntry_getProperty_cstring_t original_IORegistryEntry_getProperty_cstring = nullptr;
static IOR::_IORegistryEntry_getName_t original_IORegistryEntry_getName = nullptr;
static IOR::_IOIterator_getNextObject_t original_IOIterator_getNextObject = nullptr;
static IOR::_IOService_getMatchingServices_t original_IOService_getMatchingServices = nullptr;
static IOR::_IOService_getMatchingService_t original_IOService_getMatchingService = nullptr;

// Module-specific Filtered Process List
const PHTM::DetectedProcess IOR::filteredProcs[] = {
    {"LeagueClient", 0},
    {"LeagueofLegends", 0},
    {"LeagueClientUx H", 0},
    {"RiotClientServic", 0}
};

// List of IORegistry class names to hide from the filtered processes.
const char *IOR::filteredClasses[] = {
	"AppleVirtIONetwork",
	"AppleVirtIOPCITransport",
    "AppleVirtIOBlockStorageDevice",
};

// Generic isProcFiltered Helper Function
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

    // Check if the process is one we want to target.
    if (isProcFiltered(procName))
    {
        const char *keyName = aKey->getCStringNoCopy();

        // If the key is "manufacturer", spoof it.
        if (keyName && strcmp(keyName, "manufacturer") == 0)
        {
            const char* entryClassName = that->getMetaClass()->getClassName();
            const char* spoofedValue = "Apple Inc.";
            
            // Create a buffer to hold the original value.
            char originalValue[128];

            // Populate originalValue with a useful description
            if (original_property) {
                OSString* str_prop = OSDynamicCast(OSString, original_property);
                OSData* data_prop = OSDynamicCast(OSData, original_property);
                OSNumber* num_prop = OSDynamicCast(OSNumber, original_property);

                if (data_prop) {
                    snprintf(originalValue, sizeof(originalValue), "'%.*s'", (int)data_prop->getLength(), static_cast<const char*>(data_prop->getBytesNoCopy()));
                } else if (str_prop) {
                    snprintf(originalValue, sizeof(originalValue), "'%s'", str_prop->getCStringNoCopy());
                } else if (num_prop) {
                    snprintf(originalValue, sizeof(originalValue), "Number(%llu)", num_prop->unsigned64BitValue());
                } else {
                    // Fallback for any other type
                    snprintf(originalValue, sizeof(originalValue), "Object<%s>", original_property->getMetaClass()->getClassName());
                }
            } else {
                strlcpy(originalValue, "nullptr", sizeof(originalValue));
			}
            
            // Now, log the complete before-and-after picture with the correct original value.
            DBGLOG(MODULE_IOR, "'%s' (PID: %d) on class '%s' is spoofing 'manufacturer'. Was: %s -> Now: '%s'",
                   procName, pid, entryClassName, originalValue, spoofedValue);
                   
            return OSString::withCString(spoofedValue);
        }
    }

    // For all other cases, just return the original property.
    return original_property;
}

// IORegistry Module Initialization
void IOR::init(KernelPatcher &Patcher) {
    
    DBGLOG(MODULE_IOR, "IOR::init(Patcher) called. IORegistry module is starting.");

    // Route Requests for getProperty
    KernelPatcher::RouteRequest requests[] = {
        { "__ZNK15IORegistryEntry11getPropertyEPK8OSSymbol", phtm_IORegistryEntry_getProperty_os_symbol, original_IORegistryEntry_getProperty_os_symbol },
        { "__ZNK15IORegistryEntry11getPropertyEPKc", phtm_IORegistryEntry_getProperty_cstring, original_IORegistryEntry_getProperty_cstring },
    };

    // By default, we only patch the first, most critical function.
    // Perform Safety Checks Before Attempting to Patch Both.
	size_t request_count = 1;
    
    // Resolve the addresses of BOTH functions first.
    mach_vm_address_t addr_os_symbol = Patcher.solveSymbol(KernelPatcher::KernelID, requests[0].symbol);
    mach_vm_address_t addr_cstring = Patcher.solveSymbol(KernelPatcher::KernelID, requests[1].symbol);

    if (!addr_os_symbol || !addr_cstring) {
        DBGLOG(MODULE_ERROR, "Could not resolve one or both getProperty variants.");
        return;
    }

    // Calculate the distance between the two functions in memory.
    const int64_t min_safe_distance = 32;
    int64_t functions_distance = (addr_os_symbol > addr_cstring) ? (addr_os_symbol - addr_cstring) : (addr_cstring - addr_os_symbol);
    DBGLOG(MODULE_IOR, "Distance between getProperty variants is %lld bytes.", functions_distance);
    
    // --- Decide Whether to Patch One or Both Functions ---
    if (functions_distance < min_safe_distance) {
		DBGLOG(MODULE_WARN, "getProperty variants are too close. Patching only one to avoid multiroute panic.");
    } else {
        DBGLOG(MODULE_IOR, "Conditions are safe to patch both getProperty variants.");
        request_count = 2;
    }

    // Perform the reRouting
    if (!Patcher.routeMultipleLong(KernelPatcher::KernelID, requests, request_count)) {
        DBGLOG(MODULE_ERROR, "Failed to apply IORegistry patches.");
        return;
    }
    
    DBGLOG(MODULE_IOR, "IOR::init(Patcher) finished successfully.");
}
