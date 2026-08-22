/***********************************************************************************************
 ***              C O N F I D E N T I A L  ---  W E S T W O O D  S T U D I O S               ***
 ***********************************************************************************************
 *                                                                                             *
 *                 Project Name : wwbitpack                                                    *
 *                                                                                             *
 *                     $Archive:: /Commando/Code/wwbitpack/bitstream.h                        $*
 *                                                                                             *
 *              Original Author:: Tom Spencer-Smith                                            *
 *                                                                                             *
 *                      $Author:: Patrick                                                     $*
 *                                                                                             *
 *                     $Modtime:: 6/13/01 9:05a                                               $*
 *                                                                                             *
 *                    $Revision:: 3                                                           $*
 *                                                                                             *
 *---------------------------------------------------------------------------------------------*
 * Functions:                                                                                  *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

#ifndef BITSTREAM_H
#define BITSTREAM_H


#include "bitpacker.h"
#include "encoderlist.h"
#include "engine_string.h"
#include "bitpackids.h"
#include "cMathUtil.h"
#define BYTE_DEPTH(x)		(sizeof(x))
#define BIT_DEPTH(x)			(8 * sizeof(x))
#define MISCUTIL_EPSILON 0.0001f

/**
** BitStreamClass
**
** Author:       Tom Spencer-Smith
** Date:         June 1998
** Description:  A class for minimal bit encoding.
**					  Notes:
**					  - Uncompressed data may be included in the bitstream.
**					  - Compression may be disabled entirely if desired.
**					  - Bools are compressed to 1 bit without requiring a precision
**					    setup.
**					  - Strings and raw data are uncompressed.
**
** (gth, 08/31/2000) - renamed this class to BitStreamClass (from cTypeEncoder) and
** cleaned it up to become the interface that all game and library code uses to
** package up their state variables for network transmission, converted to westwood 
** naming convention since it is going to propogate to a lot of other code.
*/

class BitStreamClass : public cBitPacker
{
	public:

		BitStreamClass();
      BitStreamClass& operator=(const BitStreamClass& rhs);

		UINT Get_Uncompressed_Size_Bytes() const {return UncompressedSizeBytes;}
		UINT Get_Compressed_Size_Bytes() const;
		UINT Get_Compression_Pc() const;

      //
      // For data which may include NULL's.
		// Data will not be compressed.
      //
      void Add_Raw_Data(LPCSTR data, USHORT data_size);
		void Get_Raw_Data(char * buffer, USHORT buffer_size, USHORT data_size);

      //
      // For data terminated with NULL.
		// Data will not be compressed.
		// You may permit or disallow empty strings to be passed.
      //
      void Add_Terminated_String(LPCSTR string, bool permit_empty = false);
		void Get_Terminated_String(char * buffer, USHORT buffer_size, bool permit_empty = false);

      //
      // For data terminated with NULL.
		// Data will not be compressed.
		// You may permit or disallow empty strings to be passed.
      //
      void Add_Wide_Terminated_String(const WCHAR *string, bool permit_empty = false);
		void Get_Wide_Terminated_String (WCHAR *buffer, USHORT buffer_len, bool permit_empty = false);		

		//
		// Bool is special-cased because we know that we can always 
		// represent it as 1 bit.
		//
		void Add(bool value);
		bool Get(bool & value);

		// 
		// For all other data types that we want to support, call into our internal 
		// template function.  
		//
		enum {NO_ENCODER = -1};

		void		Add(BYTE val,int type = NO_ENCODER)							{ Internal_Add(val,type); }
		void		Add(USHORT val,int type = NO_ENCODER)						{ Internal_Add(val,type); }
		void		Add(UINT val,int type = NO_ENCODER)							{ Internal_Add(val,type); }
		void		Add(ULONG val,int type = NO_ENCODER)						{ Internal_Add(val,type); }
		void		Add(char val,int type = NO_ENCODER)							{ Internal_Add(val,type); }
		void		Add(int val,int type = NO_ENCODER)							{ Internal_Add(val,type); }
		void		Add(float val,int type = NO_ENCODER)						{ Internal_Add(val,type); }

		BYTE		Get(BYTE & set_val,int type = NO_ENCODER)					{ return Internal_Get(set_val,type); }
		USHORT	Get(USHORT & set_val,int type = NO_ENCODER)				{ return Internal_Get(set_val,type); }
		ULONG		Get(ULONG & set_val,int type = NO_ENCODER)				{ return Internal_Get(set_val,type); }
		UINT		Get(UINT & set_val,int type = NO_ENCODER)					{ return Internal_Get(set_val,type); }
		char		Get(char & set_val,int type = NO_ENCODER)					{ return Internal_Get(set_val,type); }
		int		Get(int & set_val,int type = NO_ENCODER)					{ return Internal_Get(set_val,type); }
		float		Get(float & set_val,int type = NO_ENCODER)				{ return Internal_Get(set_val,type); }

	private:
		
		//
		// Add/Get for remaining atomic data types.
		// I really wish the following 3 methods were in the source file, but
		// the compiler won't accept this. Hopefully the pragma and MSVC
		// will prevent inlining.
		//
#pragma auto_inline(off)
      //------------------------------------------------------------------------------------
		template<class T> void Internal_Add(T value, int type = NO_ENCODER) {
			if (cEncoderList::Is_Compression_Enabled() && type != NO_ENCODER) {

				cEncoderTypeEntry & entry = cEncoderList::Get_Encoder_Type_Entry(type);

				//
				// If the following assert hits then the value of the type 
				// parameter is unknown.
				//

				ULONG scaled_value;
				bool is_in_range = entry.Scale(value, scaled_value);
				if (!is_in_range) {
					//WWDEBUG_SAY(("BitStreamClass::Add : Warning: out-of-range value clamped (type %d).\n",
					//	type));
					//DIE;
				}

				Add_Bits(scaled_value, entry.Get_Bit_Precision());

			} else {
				Add_Bits(*(reinterpret_cast<ULONG *>(&value)), BIT_DEPTH(T));
			}

			UncompressedSizeBytes += BYTE_DEPTH(T);
		}
		
		//------------------------------------------------------------------------------------
      template<class T> T Internal_Get(T & value, int type = NO_ENCODER) {

			if (cEncoderList::Is_Compression_Enabled() && type != NO_ENCODER) {

				cEncoderTypeEntry & entry = cEncoderList::Get_Encoder_Type_Entry(type);

				//
				// If the following assert hits then the value of the type 
				// parameter is unknown.
				//

				ULONG u_value;
				Get_Bits(u_value, entry.Get_Bit_Precision());

				double f_value = entry.Unscale(u_value);

				if ((::fabs(f_value - static_cast<T>(f_value)) < MISCUTIL_EPSILON)) {
					//
					// N.B. More error may be introduced here
					//
					value = static_cast<T>(f_value);
				} else {
					value = static_cast<T>(cMathUtil::Round(f_value));
				}


			} else {
				ULONG u_value;
				Get_Bits(u_value, BIT_DEPTH(T));

				value = *(reinterpret_cast<T *>(&u_value));
			}
			return value;
		}

		//------------------------------------------------------------------------------------
#pragma auto_inline(on)

		UINT UncompressedSizeBytes; // for statistics only
};

#endif // TYPEENCODER_H
