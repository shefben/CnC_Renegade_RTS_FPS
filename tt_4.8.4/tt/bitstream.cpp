#include "General.h"
#include "bitstream.h"
#include "encoderlist.h"
#include "encodertypeentry.h"
BitStreamClass::BitStreamClass()
   : UncompressedSizeBytes(0)
{
}
void BitStreamClass::Add
   (bool bBit)
{
   this->Add_Bits (bBit, cEncoderList::Is_Compression_Enabled() ? 1 : 8);
   ++this->UncompressedSizeBytes;
}
void BitStreamClass::Add_Terminated_String
   (const char* cString, bool)
{
   uint32 uLength = ::strlen (cString);

   this->Add_Bits (uLength, 16);
   
   // why does += cause a warning?
   this->UncompressedSizeBytes = this->UncompressedSizeBytes + (uint16)uLength + 2;

   while (uLength--)
      this->Add_Bits (*(cString++), 8);
}
void BitStreamClass::Add_Wide_Terminated_String
   (const wchar_t* wString, bool)
{
   uint32 uIndex = (this->Get_Bit_Write_Position() + 7) / 8;

   if (uIndex >= MAX_BUFFER_SIZE - 20)
      return;

   uint32 uBytesLeft = MAX_BUFFER_SIZE - 20 - uIndex;
   uint32 uLength = ::wcslen (wString);

   if (uLength * 2 > uBytesLeft)
      uLength = uBytesLeft / 2;

   this->Add_Bits (uLength, 16);
   
   // why does += cause a warning?
   this->UncompressedSizeBytes = this->UncompressedSizeBytes + (uint16)uLength * 2 + 2;

   while (uLength--)
      this->Add_Bits ((uint16)*(wString++), 16);
}
bool BitStreamClass::Get
   (bool &bBit)
{
   ULONG uValue;
   this->Get_Bits (uValue, cEncoderList::Is_Compression_Enabled() ? 1 : 8);

   bBit = (uValue != 0);
   return bBit;
}
uint32 BitStreamClass::Get_Compressed_Size_Bytes() const
{
   return (this->Get_Bit_Write_Position() + 7) / 8;
}
void BitStreamClass::Get_Terminated_String
   (char* cBuffer, uint16 uBufferLength, bool)
{
   ULONG uLength;
   this->Get_Bits (uLength, 16);

   if (!uLength)
   {
      *cBuffer = 0;
      return;
   }

   uint32 u = 0;
   ULONG uTemp;

   for (; u < uLength; ++u)
   {
      this->Get_Bits (uTemp, 8);
      if (u + 1 < uBufferLength)
         cBuffer[u] = (char)uTemp;
   }

   if (!uBufferLength)
      return;

   if (u < uBufferLength)
      cBuffer[u] = 0;
   else
      cBuffer[uBufferLength - 1] = 0;
}
void BitStreamClass::Get_Wide_Terminated_String
   (wchar_t* wBuffer, uint16 uBufferLength, bool)
{
   ULONG uLength;
   this->Get_Bits (uLength, 16);

   if (!uLength)
   {
      *wBuffer = 0;
      return;
   }

   uint32 u = 0;
   ULONG uTemp;

   for (; u < uLength; ++u)
   {
      this->Get_Bits (uTemp, 16);
      if (u + 1 < uBufferLength)
         wBuffer[u] = (wchar_t)uTemp;
   }

   if (!uBufferLength)
      return;

   if (u < uBufferLength)
      wBuffer[u] = 0;
   else
      wBuffer[uBufferLength - 1] = 0;
}
BitStreamClass& BitStreamClass::operator =
   (const BitStreamClass& oStream)
{
   if (&oStream == this)
      return *this;

   cBitPacker::operator = (oStream);
   this->UncompressedSizeBytes = oStream.UncompressedSizeBytes;

   return *this;
}
