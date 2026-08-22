//
// Filename:     encodertypeentry.h
// Project:      wwbitpack.lib
// Author:       Tom Spencer-Smith
// Date:         June 2000
// Description:  
//
//-----------------------------------------------------------------------------
#if defined(_MSV_VER)
#pragma once
#endif

#ifndef ENCODERTYPEENTRY_H
#define ENCODERTYPEENTRY_H

class cEncoderTypeEntry
{
	public:
		cEncoderTypeEntry::cEncoderTypeEntry();

		void Init(double min, double max, double resolution);
		void Init(int num_bits);

		bool Scale(double value, ULONG & scaled_value);
		double Unscale(ULONG u_value);
		double Clamp(double value);

		UINT	Get_Bit_Precision()	const	{return BitPrecision;}
		double	Get_Resolution()	const	{return Resolution;}

		bool Is_Valid() const;
		void Invalidate();

		bool Is_Value_In_Range(double value) const;

	private:
      cEncoderTypeEntry(const cEncoderTypeEntry& source);			// Disallow
      cEncoderTypeEntry& operator=(const cEncoderTypeEntry& rhs);	// Disallow

		void Calc_Bit_Precision(double resolution);

		double Min;
		double Max;
		double Resolution;
		UINT BitPrecision;
};

#endif // ENCODERTYPEENTRY_H
