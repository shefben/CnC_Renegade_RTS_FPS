#include "General.h"
#include "BitPacker.h"

cBitPacker::cBitPacker()
   : BitWritePosition(0), BitReadPosition(0)
{
   ::memset (this->Buffer, 0, sizeof (this->Buffer));
}
void cBitPacker::Add_Bits
   (ULONG uBits, UINT uBitCount)
{
   uint32 uPosition = this->BitWritePosition;
   uint32 uIndex = uPosition / 8;

   this->BitWritePosition += uBitCount;

   uBits &= 0xFFFFFFFF >> (32 - uBitCount);

   while (uBitCount && uBits)
   {
      uint8 uBitsFree  = (uint8)(8 - (uPosition & 0x07));
      UINT uBitsWrite = uBitsFree;

      if (uBitsWrite > uBitCount)
         uBitsWrite = uBitCount;

      this->Buffer[uIndex++] |= ((uBits << (32 - uBitCount)) >> (32 - uBitsWrite)) << (uBitsFree - uBitsWrite);
      uPosition += uBitsWrite;
      uBitCount  = uBitCount - uBitsWrite;
      uBits     &= 0xFFFFFFFF >> (32 - uBitCount);
   }
}
void cBitPacker::Get_Bits
   (ULONG& uTarget, UINT uBitCount)
{
   UINT  uTemp;
   uint32 uIndex = this->BitReadPosition / 8;
   sint32 sBitsLeft, sPosition, sTemp;

   sPosition = this->BitReadPosition & 0x07;
   this->BitReadPosition += uBitCount;

   uTemp = 8 - (uint8)sPosition;
   if (uTemp > uBitCount)
      uTemp = uBitCount;

   uTarget = this->Buffer[uIndex] << (sPosition + 24);
   uIndex++;
   sBitsLeft = uBitCount - uTemp;
   sPosition = 24 - uTemp;

   while (sPosition > 0 && sBitsLeft > 0)
   {
      uTarget |= this->Buffer[uIndex] << sPosition;
      uIndex++;

      sPosition -= 8;
      sBitsLeft -= 8;
   }

   if (sBitsLeft)
   {
      sTemp = -sPosition;
      uTemp = this->Buffer[uIndex];
      uTemp >>= sTemp;
      uTarget |= uTemp;
   }

   uint32 uMask = 0xFFFFFFFF >> (32 - uBitCount);

   uTarget >>= 32 - uBitCount;
   uTarget &= uMask;
}
void cBitPacker::Set_Bit_Write_Position
   (uint32 BitWritePosition)
{
   this->BitWritePosition = BitWritePosition;
}
cBitPacker& cBitPacker::operator =
   (const cBitPacker& oPacker)
{
   if (&oPacker == this)
      return *this;

   ::memcpy (this->Buffer, oPacker.Buffer, sizeof (cBitPacker::Buffer));

   this->BitWritePosition = oPacker.BitWritePosition;
   this->BitReadPosition  = oPacker.BitReadPosition;

   return *this;
}
cBitPacker::~cBitPacker()
{
}
