// blah blah blah This file is confidential blah blah blah
#ifndef TT_INCLUDE_HOOKSUPPORT_H_
#define TT_INCLUDE_HOOKSUPPORT_H_

void WriteMemory(void* lpBaseAddress, const void* lpBuffer, size_t nSize);
void WriteMemory(uint32 lpBaseAddress, const void* lpBuffer, size_t nSize);

void WriteVtable(void* location, void* function);
void WriteVtableForPlatform(void* a, void* b, void* function);

void WriteJump(void *location, void *function);
void WriteJump(uint32 address, void* function);
void WriteJump(uint32 address, uint32 function);
void WriteJump(void *location, void *function, int nop_count);
void WriteJumpForPlatform(void* a, void* b, void* function);

void WriteCall(void *location, void *function);
void WriteCall(void *location, void *function, int count);
void WriteCallForPlatform(void* a, void* b, void* function);
void WriteCallForPlatform2(void* a, void* b, void* function, int count);

void WriteNop(void *location, int count);
void WriteNopForPlatform(void* a, void* b, int count);

#define typedHookAsJump(game, fds, function) ((void (*)(...))WriteJumpForPlatform)(game, fds, function)

#define hookAsJump(a,b,function) __asm \
   {                                       \
      __asm push function                  \
      __asm push b                         \
      __asm push a                         \
      __asm call WriteJumpForPlatform      \
      __asm add esp, 12                    \
   }

#define hookNop(a,b,count) __asm \
   {                                       \
      __asm push count                  \
      __asm push b                         \
      __asm push a                         \
      __asm call WriteNopForPlatform      \
      __asm add esp, 12                    \
   }

#define hookAsCall(a,b,function) __asm \
   {                                       \
      __asm push function                  \
      __asm push b                         \
      __asm push a                         \
      __asm call WriteCallForPlatform      \
      __asm add esp, 12                    \
   }

#define hookAsCall2(a,b,function,count) __asm \
   {                                       \
      __asm push count                     \
      __asm push function                  \
      __asm push b                         \
      __asm push a                         \
      __asm call WriteCallForPlatform2     \
      __asm add esp, 16                    \
   }

#define WriteVtable2(a,f) __asm \
   {                                       \
      __asm push f                         \
      __asm push a                         \
      __asm call WriteVtable               \
      __asm add esp, 8                     \
   }


#define hookVtable(a,b,f) __asm \
   {                                       \
      __asm push f                         \
      __asm push b                         \
      __asm push a                         \
      __asm call WriteVtableForPlatform    \
      __asm add esp, 12                    \
   }

#endif