#include "General.h"
#include "WWOnline.h"



RENEGADE_FUNCTION
RefPtr<WWOnline::Session> WWOnline::Session::GetInstance(bool)
AT2(0x004CF0E0, 0x004CE980);



RENEGADE_FUNCTION
void WWOnline::Session::RequestPing(const char*, int)
AT2(0x004D74A0, 0x004D6D40);
