#ifndef TT_INCLUDE_MSS32_H
#define TT_INCLUDE_MSS32_H
extern "C" {
long WINAPI AIL_startup(void);
void WINAPI AIL_lock(void);
void WINAPI AIL_unlock(void);
long WINAPI AIL_enumerate_3D_providers(long *next, long *dest, char **name);
long WINAPI AIL_open_3D_provider(long provider);
void WINAPI AIL_close_3D_provider(long provider);
void WINAPI AIL_shutdown(void);
char *WINAPI AIL_last_error(void);
long WINAPI AIL_set_preference(long l1,long l2);
void WINAPI AIL_waveOutClose(char *handle);
long WINAPI AIL_waveOutOpen(char **handle,long *l1,long l2,LPWAVEFORMAT format);
};
#endif
