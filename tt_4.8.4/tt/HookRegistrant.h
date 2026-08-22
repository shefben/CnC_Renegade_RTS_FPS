#include "hooksupport.h"

// Do not look at this file if you don't like hacks.

#define CONCAT(x,y) CONCAT_(x,y)
#define CONCAT_(x,y) x##y

#define JumpHookRegistrant(game, fds, function) static JumpHookRegistrantClass CONCAT(hookRegistrant_, __COUNTER__ )(game, fds, force_cast<uint32>(function))

class JumpHookRegistrantClass
{
public:
    JumpHookRegistrantClass(uint32 game, uint32 fds, uint32 function)
    {
        WriteJumpForPlatform((void*)game, (void*)fds, (void*)function);
    }
};

#define CallHookRegistrant(game, fds, function) static CallHookRegistrantClass CONCAT(hookRegistrant_, __COUNTER__ )(game, fds, force_cast<uint32>(function))

class CallHookRegistrantClass
{
public:
    CallHookRegistrantClass(uint32 game, uint32 fds, uint32 function)
    {
        WriteCallForPlatform((void*)game, (void*)fds, (void*)function);
    }
};


template<typename D, typename S> __forceinline D force_cast(S s) { union { D d; S s; } c; c.s = s; return c.d; } 
