#ifndef TT_INCLUDE__TEXTURES_MATERIAL_AND_SHADER_BOOKING_STRUCT_H
#define TT_INCLUDE__TEXTURES_MATERIAL_AND_SHADER_BOOKING_STRUCT_H



#include "engine_3dre.h"



class TextureClass;
class VertexMaterialClass;



class Textures_Material_And_Shader_Booking_Struct
{

public:

	struct Booking
	{
		TextureClass* textures[2];
		VertexMaterialClass* material;
		ShaderClass shader;

		friend bool operator ==(const Booking& a, const Booking& b)
		{
			return a.textures[0] == b.textures[0] &&
			       a.textures[1] == b.textures[1] &&
			       a.shader.Get_Bits() == b.shader.Get_Bits() &&
			       *a.material == *b.material;
		}
	};

private:

	static const uint MAX_BOOKING_COUNT = 64;
	Booking bookings[MAX_BOOKING_COUNT];
	uint bookingCount;

public:

	Textures_Material_And_Shader_Booking_Struct();
	bool Add_Textures_Material_And_Shader(const Booking& booking);
	bool Add_Textures_Material_And_Shader(TextureClass** textures, VertexMaterialClass* material, ShaderClass shader);

};



#endif