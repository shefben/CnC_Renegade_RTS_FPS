#include "general.h"
#include "encoderlist.h"
#include "encodertypeentry.h"
REF_DEF2(cEncoderList::IsCompressionEnabled, bool, 0x008163E4, 0x008155BC);
REF_ARR_DEF2(cEncoderList::EncoderTypes, cEncoderTypeEntry, MAX_ENCODERTYPES, 0x0089DCE0, 0x0089CEC8);
cEncoderTypeEntry &cEncoderList::Get_Encoder_Type_Entry
   (int index)
{
	return cEncoderList::EncoderTypes[index];
}
