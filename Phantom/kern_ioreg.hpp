//
//  kern_ioreg.hpp
//  Phantom
//
//  Created by RoyalGraphX on 6/7/25.
//

#ifndef kern_ioreg_hpp
#define kern_ioreg_hpp

// Include Parent Module
#include "kern_start.hpp"
#include <IOKit/IORegistryEntry.h>
#include <libkern/c++/OSSymbol.h>
#include <libkern/c++/OSString.h>
#include <IOKit/IOLib.h>

// Logging Defs
#define MODULE_IOR "IOR"
#define MODULE_RRIOR "RRIOR"

// Forward declarations for IOKit classes to keep this header light
class IORegistryEntry;
class IORegistryPlane;
class IOIterator;
class OSObject;
class KernelPatcher;
class OSSymbol;

// IORegistry Patcher Class
class IOR {
public:
	
	/**
     * @brief Initializes the IORegistry module.
     * Will be called by the orchestrator in PHTM after KernelPatcher is ready.
     * @param Patcher A reference to the initialized KernelPatcher instance.
     */
	static void init(KernelPatcher &Patcher);
	
	// Declaration for the array of processes to filter
    static const PHTM::DetectedProcess filteredProcs[];
	
	// Function pointer types for the original kernel functions
    // Note: The methods we are hooking are const, so the 'this' pointer is const IORegistryEntry*
    using _IORegistryEntry_getProperty_t = OSObject * (*)(const IORegistryEntry *that, const OSSymbol *aKey);
    using _IORegistryEntry_getProperty_cstring_t = OSObject * (*)(const IORegistryEntry *that, const char *aKey);
    using _IORegistryEntry_getName_t = const char * (*)(const IORegistryEntry *that, const IORegistryPlane *plane);
    using _IOIterator_getNextObject_t = OSObject * (*)(OSCollectionIterator *that);
	
private:
	
	// No private members at the moment
	
};


#endif /* kern_ioreg_hpp */
