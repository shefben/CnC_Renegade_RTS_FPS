#include "General.h"
#include "Textures_Material_And_Shader_Booking_Struct.h"



Textures_Material_And_Shader_Booking_Struct::Textures_Material_And_Shader_Booking_Struct()
{
	bookingCount = 0;
}



bool Textures_Material_And_Shader_Booking_Struct::Add_Textures_Material_And_Shader(TextureClass** textures, VertexMaterialClass* material, ShaderClass shader)
{
	Booking booking;
	booking.textures[0] = textures[0];
	booking.textures[1] = textures[1];
	booking.material = material;
	booking.shader = shader;
	return Add_Textures_Material_And_Shader(booking);
}



bool Textures_Material_And_Shader_Booking_Struct::Add_Textures_Material_And_Shader(const Booking& booking)
{
	TT_ASSERT(bookingCount != MAX_BOOKING_COUNT);
	if (bookingCount >= MAX_BOOKING_COUNT)
		return false;

	for (uint bookingIndex = 0; bookingIndex < bookingCount; ++bookingIndex)
		if (bookings[bookingIndex] == booking)
			return false;

	bookings[bookingCount++] = booking;
	return true;
}
