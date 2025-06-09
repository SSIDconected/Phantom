//
//  kern_securelevel.hpp
//  Phantom
//
//  Created by RoyalGraphX on 6/5/25.
//

#ifndef kern_securelevel_hpp
#define kern_securelevel_hpp

// Include Parent Module
#include "kern_start.hpp"

// Logging Defs
#define MODULE_SLP "SLP"
#define MODULE_KSL "KSL"
#define MODULE_RRSL "RRSL"

// SIP Patcher Class
class SLP {
public:
	
	// Declaration for the init function
	static void init(KernelPatcher &Patcher);
	
	// Store the original handler
	static sysctl_handler_t originalSecureLevelHandler;
	
private:
	
	// None at the moment
	
};

#endif /* kern_securelevel_hpp */
