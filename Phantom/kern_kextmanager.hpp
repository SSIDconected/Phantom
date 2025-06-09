//
//  kern_kextmanager.hpp
//  Phantom
//
//  Created by RoyalGraphX on 6/3/25.
//

#ifndef kern_kextmanager_hpp
#define kern_kextmanager_hpp

// Include Parent Module
#include "kern_start.hpp"

// Logging Defs
#define MODULE_KMP "KMP"
#define MODULE_RRKM "RRKM"
#define MODULE_CLKI "CLKI"

// KM Patcher Class
class KMP {
public:
	
	// Declaration for the init function
	static void init(KernelPatcher &Patcher);

	// Function pointer type for the original kernel functions
    using _OSKext_copyLoadedKextInfo_t = OSDictionary *(*)(OSArray *kextIdentifiers, OSArray *bundlePaths);
	
private:
	
	// None at the moment
	
};

#endif /* kern_kextmanager_hpp */
