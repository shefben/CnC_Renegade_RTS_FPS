#include "general.h"
#include "ControlClass.h"
#include "bitstream.h"
void ControlClass::Clear_Boolean()
{
   this->booleanA = 0;
   this->booleanB = 0;
}



// 0x0069CB70,0x0069C410,?,?
void ControlClass::Clear_Control()
{
   this->booleanA = 0;
   this->booleanB = 0;
   this->analog[0] = 0;
   this->analog[1] = 0;
   this->analog[2] = 0;
   this->analog[3] = 0;
}



void ControlClass::Export_Cs
   (BitStreamClass& stream)
{
   stream.Add(this->setBooleanA,BITPACK_ONE_TIME_BOOLEAN_BITS);
   this->setBooleanA = 0;
   stream.Add(this->setBooleanB,BITPACK_CONTINUOUS_BOOLEAN_BITS);
   this->setBooleanB = 0;
   stream.Add(this->analog[0],BITPACK_ANALOG_VALUES);
   stream.Add(this->analog[1],BITPACK_ANALOG_VALUES);
   stream.Add(this->analog[2],BITPACK_ANALOG_VALUES);
   stream.Add(this->analog[3],BITPACK_ANALOG_VALUES);
}



void ControlClass::Export_Sc
   (BitStreamClass& stream)
{
   stream.Add(this->booleanB,BITPACK_CONTINUOUS_BOOLEAN_BITS);
   stream.Add(this->analog[0],BITPACK_ANALOG_VALUES);
   stream.Add(this->analog[1],BITPACK_ANALOG_VALUES);
   stream.Add(this->analog[2],BITPACK_ANALOG_VALUES);
   stream.Add(this->analog[3],BITPACK_ANALOG_VALUES);
}



// 0x0069D710,0x0069CFB0,?,?
bool ControlClass::Get_Boolean
   (ControlClass::BooleanControl boolean)
{
   if (boolean <= 22)
      return ((this->booleanA & (1 << boolean)) != 0);
   else
      return ((this->booleanB & (1 << (boolean - 23))) != 0);
}



// 0x0069CB90,0x0069C430,0,0x080F8CA8
void ControlClass::Import_Cs
   (BitStreamClass& stream)
{
	uint32 _booleanA;
	booleanA |= stream.Get(_booleanA,BITPACK_ONE_TIME_BOOLEAN_BITS);
	stream.Get(booleanB,BITPACK_CONTINUOUS_BOOLEAN_BITS);
	stream.Get(this->analog[0],BITPACK_ANALOG_VALUES);
	stream.Get(this->analog[1],BITPACK_ANALOG_VALUES);
	stream.Get(this->analog[2],BITPACK_ANALOG_VALUES);
	stream.Get(this->analog[3],BITPACK_ANALOG_VALUES);

	if (::fabs (this->analog[0]) < 0.005f)
		this->analog[0] = 0;
	if (::fabs (this->analog[1]) < 0.005f)
		this->analog[1] = 0;
	if (::fabs (this->analog[2]) < 0.005f)
		this->analog[2] = 0;
	if (::fabs (this->analog[3]) < 0.005f)
		this->analog[3] = 0;
}



void ControlClass::Import_Sc
   (BitStreamClass& stream)
{
   stream.Get(this->booleanB,BITPACK_CONTINUOUS_BOOLEAN_BITS);
   stream.Get(this->analog[0],BITPACK_ANALOG_VALUES);
   stream.Get(this->analog[1],BITPACK_ANALOG_VALUES);
   stream.Get(this->analog[2],BITPACK_ANALOG_VALUES);
   stream.Get(this->analog[3],BITPACK_ANALOG_VALUES);
   if (::fabs (this->analog[0]) < 0.005f)
      this->analog[0] = 0;
   if (::fabs (this->analog[1]) < 0.005f)
      this->analog[1] = 0;
   if (::fabs (this->analog[2]) < 0.005f)
      this->analog[2] = 0;
   if (::fabs (this->analog[3]) < 0.005f)
      this->analog[3] = 0;
}



// 0x0069D690,0x0069CF30,?,?
void ControlClass::Set_Boolean
   (ControlClass::BooleanControl boolean, bool value)
{
   if (boolean <= 22)
   {
      if (value)
         this->booleanA |= (1 << boolean);
      else
         this->booleanA &= (0xFFFFFFFF ^ (1 << boolean));

      this->setBooleanA |= this->booleanA;
   }
   else
   {
      boolean = static_cast<ControlClass::BooleanControl> (boolean - 23);

      if (value)
         this->booleanB |= (1 << boolean);
      else
         this->booleanB &= (0xFF ^ (1 << boolean));

      this->setBooleanB |= this->booleanB;
   }
}



void ControlClass::Set_Precision()
{
	cEncoderList::Set_Precision(BITPACK_ONE_TIME_BOOLEAN_BITS,23);
	cEncoderList::Set_Precision(BITPACK_CONTINUOUS_BOOLEAN_BITS,4);
	cEncoderList::Set_Precision(BITPACK_CONTROL_MOVES_CS,8);
	cEncoderList::Set_Precision(BITPACK_CONTROL_MOVES_SC,6);
	cEncoderList::Set_Precision(BITPACK_ANALOG_VALUES,-1.0f,1.0f,0.01f);
}

RENEGADE_FUNCTION
bool ControlClass::Load(ChunkLoadClass &chunkLoader)
AT2(0x0069CAB0,0x0069C350);
