//
// Filename:     encoderlist.h
// Project:      wwbitpack.lib
// Author:       Tom Spencer-Smith
// Date:         June 1998
// Description:  
//					  

#ifndef ENCODERLIST_H
#define ENCODERLIST_H

#include "encodertypeentry.h"
#define MISCUTIL_EPSILON 0.0001f
const int MAX_ENCODERTYPES = 100;

class cEncoderList
{
	public:
		static void Clear_Entries();

		static void Set_Compression_Enabled(bool flag) {IsCompressionEnabled = flag;}
		static bool Is_Compression_Enabled() {return IsCompressionEnabled;}

		static cEncoderTypeEntry & Get_Encoder_Type_Entry(int index);

#pragma auto_inline(off)
		//------------------------------------------------------------------------------------
		template<class T> static T Set_Precision(int type, T min, T max, 
			T resolution = 1)
		{
			EncoderTypes[type].Init(
				static_cast<double>(min), 
				static_cast<double>(max), 
				static_cast<double>(resolution));

			//
			// Return maximum representation error
			//
			return static_cast<T>(resolution / 2.0f + MISCUTIL_EPSILON);
			/*
			double max_error = EncoderTypes[type].Get_Resolution() / 2.0f + MISCUTIL_EPSILON;
			if (::fabs(max_error - static_cast<T>(max_error)) < MISCUTIL_EPSILON) {
				//return static_cast<T>(max_error);
				return static_cast<T>(max_error);
			} else {
				return static_cast<T>(ceil(max_error));
			}
			/**/
		}
		//------------------------------------------------------------------------------------
		static void Set_Precision(int type, int num_bits)
		{
			EncoderTypes[type].Init(num_bits);
		}
		//------------------------------------------------------------------------------------
#pragma auto_inline(on)

	private:
		static REF_ARR_DECL2(EncoderTypes, cEncoderTypeEntry, MAX_ENCODERTYPES);
		static REF_DECL2(IsCompressionEnabled, bool);
};

#endif // ENCODERLIST_H
