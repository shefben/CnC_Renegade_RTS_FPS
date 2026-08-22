/*	Renegade Scripts.dll
	W3D Bindings
	Copyright 2009 Mark Sararu, Jonathan Wilson

	This file is part of the Renegade scripts.dll
	The Renegade scripts.dll is free software; you can redistribute it and/or modify it under
	the terms of the GNU General Public License as published by the Free
	Software Foundation; either version 2, or (at your option) any later
	version. See the file COPYING for more details.
	In addition, an exemption is given to allow Run Time Dynamic Linking of this code with any closed source module that does not contain code covered by this licence.
	Only the source code to the module(s) containing the licenced code has to be released.
*/
#include "general.h"

#include "w3dbindings.h"
#include "w3dbindings_transform.h"
#include "effect.h"
#include "engine_3d.h"

uint transform_bits;

namespace W3DBindings
{
	DECLARE_ROOT_BINDING_CACHED(World)
	{
		assert(data != NULL);
		assert(size != NULL);
		static Matrix4 matrix;
		if (transform_bits & 1 << 1) {
			matrix = Transforms.world;
			transform_bits &= ~(1 << 1);
		}
		*data = &matrix;
		*size = sizeof(matrix);
		return true;
	};

	DECLARE_BINDING_DIRTY_FUNC(World)
	{
		if (transform_bits & 1 << 1) return true;
		return false;
	};

	DECLARE_ROOT_BINDING_CACHED(WorldTranspose)
	{
		assert(data != NULL);
		assert(size != NULL);
		static Matrix4 matrix;
		if (transform_bits & 1 << 2) {
			matrix = (Transforms.world).Transpose();
			transform_bits &= ~(1 << 2);
		}
		*data = &matrix;
		*size = sizeof(matrix);
		return true;
	};

	DECLARE_BINDING_DIRTY_FUNC(WorldTranspose)
	{
		if (transform_bits & 1 << 2) return true;
		return false;
	};

	DECLARE_ROOT_BINDING_CACHED(WorldInverse)
	{
		assert(data != NULL);
		assert(size != NULL);
		static Matrix4 matrix;
		if (transform_bits & 1 << 3) {
			matrix = (Transforms.world).Inverse();
			transform_bits &= ~(1 << 3);
		}
		*data = &matrix;
		*size = sizeof(matrix);
		return true;
	};

	DECLARE_BINDING_DIRTY_FUNC(WorldInverse)
	{
		if (transform_bits & 1 << 3) return true;
		return false;
	};

	DECLARE_ROOT_BINDING_CACHED(WorldInverseTranspose)
	{
		assert(data != NULL);
		assert(size != NULL);
		static Matrix4 matrix;
		if (transform_bits & 1 << 4) {
			matrix = (Transforms.world).Inverse().Transpose();
			transform_bits &= ~(1 << 4);
		}
		*data = &matrix;
		*size = sizeof(matrix);
		return true;
	};

	DECLARE_BINDING_DIRTY_FUNC(WorldInverseTranspose)
	{
		if (transform_bits & 1 << 4) return true;
		return false;
	};

	DECLARE_ROOT_BINDING_CACHED(View)
	{
		assert(data != NULL);
		assert(size != NULL);
		static Matrix4 matrix;
		if (transform_bits & 1 << 5) {
			matrix = Transforms.view;
			transform_bits &= ~(1 << 5);
		}
		*data = &matrix;
		*size = sizeof(matrix);
		return true;
	};

	DECLARE_BINDING_DIRTY_FUNC(View)
	{
		if (transform_bits & 1 << 5) return true;
		return false;
	};

	DECLARE_ROOT_BINDING_CACHED(ViewTranspose)
	{
		assert(data != NULL);
		assert(size != NULL);
		static Matrix4 matrix;
		if (transform_bits & 1 << 6) {
			matrix = (Transforms.view).Transpose();
			transform_bits &= ~(1 << 6);
		}
		*data = &matrix;
		*size = sizeof(matrix);
		return true;
	};

	DECLARE_BINDING_DIRTY_FUNC(ViewTranspose)
	{
		if (transform_bits & 1 << 6) return true;
		return false;
	};

	DECLARE_ROOT_BINDING_CACHED(ViewInverse)
	{
		assert(data != NULL);
		assert(size != NULL);
		static Matrix4 matrix;
		if (transform_bits & 1 << 7) {
			matrix = (Transforms.view).Inverse();
			transform_bits &= ~(1 << 7);
		}
		*data = &matrix;
		*size = sizeof(matrix);
		return true;
	};

	DECLARE_BINDING_DIRTY_FUNC(ViewInverse)
	{
		if (transform_bits & 1 << 7) return true;
		return false;
	};

	DECLARE_ROOT_BINDING_CACHED(ViewInverseTranspose)
	{
		assert(data != NULL);
		assert(size != NULL);
		static Matrix4 matrix;
		if (transform_bits & 1 << 8) {
			matrix = (Transforms.view).Inverse().Transpose();
			transform_bits &= ~(1 << 8);
		}
		*data = &matrix;
		*size = sizeof(matrix);
		return true;
	};

	DECLARE_BINDING_DIRTY_FUNC(ViewInverseTranspose)
	{
		if (transform_bits & 1 << 8) return true;
		return false;
	};

	DECLARE_ROOT_BINDING_CACHED(Projection)
	{
		assert(data != NULL);
		assert(size != NULL);
		static Matrix4 matrix;
		if (transform_bits & 1 << 9) {
			matrix = Transforms.projection;
			transform_bits &= ~(1 << 9);
		}
		*data = &matrix;
		*size = sizeof(matrix);
		return true;
	};

	DECLARE_BINDING_DIRTY_FUNC(Projection)
	{
		if (transform_bits & 1 << 9) return true;
		return false;
	};

	DECLARE_ROOT_BINDING_CACHED(ProjectionTranspose)
	{
		assert(data != NULL);
		assert(size != NULL);
		static Matrix4 matrix;
		if (transform_bits & 1 << 10) {
			matrix = (Transforms.projection).Transpose();
			transform_bits &= ~(1 << 10);
		}
		*data = &matrix;
		*size = sizeof(matrix);
		return true;
	};

	DECLARE_BINDING_DIRTY_FUNC(ProjectionTranspose)
	{
		if (transform_bits & 1 << 10) return true;
		return false;
	};

	DECLARE_ROOT_BINDING_CACHED(ProjectionInverse)
	{
		assert(data != NULL);
		assert(size != NULL);
		static Matrix4 matrix;
		if (transform_bits & 1 << 11) {
			matrix = (Transforms.projection).Inverse();
			transform_bits &= ~(1 << 11);
		}
		*data = &matrix;
		*size = sizeof(matrix);
		return true;
	};

	DECLARE_BINDING_DIRTY_FUNC(ProjectionInverse)
	{
		if (transform_bits & 1 << 11) return true;
		return false;
	};

	DECLARE_ROOT_BINDING_CACHED(ProjectionInverseTranspose)
	{
		assert(data != NULL);
		assert(size != NULL);
		static Matrix4 matrix;
		if (transform_bits & 1 << 12) {
			matrix = (Transforms.projection).Inverse().Transpose();
			transform_bits &= ~(1 << 12);
		}
		*data = &matrix;
		*size = sizeof(matrix);
		return true;
	};

	DECLARE_BINDING_DIRTY_FUNC(ProjectionInverseTranspose)
	{
		if (transform_bits & 1 << 12) return true;
		return false;
	};

	DECLARE_ROOT_BINDING_CACHED(WorldView)
	{
		assert(data != NULL);
		assert(size != NULL);
		static Matrix4 matrix;
		if (transform_bits & 1 << 13) {
			matrix = Transforms.world * Transforms.view;
			transform_bits &= ~(1 << 13);
		}
		*data = &matrix;
		*size = sizeof(matrix);
		return true;
	};

	DECLARE_BINDING_DIRTY_FUNC(WorldView)
	{
		if (transform_bits & 1 << 13) return true;
		return false;
	};

	DECLARE_ROOT_BINDING_CACHED(WorldViewTranspose)
	{
		assert(data != NULL);
		assert(size != NULL);
		static Matrix4 matrix;
		if (transform_bits & 1 << 14) {
			matrix = (Transforms.world * Transforms.view).Transpose();
			transform_bits &= ~(1 << 14);
		}
		*data = &matrix;
		*size = sizeof(matrix);
		return true;
	};

	DECLARE_BINDING_DIRTY_FUNC(WorldViewTranspose)
	{
		if (transform_bits & 1 << 14) return true;
		return false;
	};

	DECLARE_ROOT_BINDING_CACHED(WorldViewInverse)
	{
		assert(data != NULL);
		assert(size != NULL);
		static Matrix4 matrix;
		if (transform_bits & 1 << 15) {
			matrix = (Transforms.world * Transforms.view).Inverse();
			transform_bits &= ~(1 << 15);
		}
		*data = &matrix;
		*size = sizeof(matrix);
		return true;
	};

	DECLARE_BINDING_DIRTY_FUNC(WorldViewInverse)
	{
		if (transform_bits & 1 << 15) return true;
		return false;
	};

	DECLARE_ROOT_BINDING_CACHED(WorldViewInverseTranspose)
	{
		assert(data != NULL);
		assert(size != NULL);
		static Matrix4 matrix;
		if (transform_bits & 1 << 16) {
			matrix = (Transforms.world * Transforms.view).Inverse().Transpose();
			transform_bits &= ~(1 << 16);
		}
		*data = &matrix;
		*size = sizeof(matrix);
		return true;
	};

	DECLARE_BINDING_DIRTY_FUNC(WorldViewInverseTranspose)
	{
		if (transform_bits & 1 << 16) return true;
		return false;
	};

	DECLARE_ROOT_BINDING_CACHED(ViewProjection)
	{
		assert(data != NULL);
		assert(size != NULL);
		static Matrix4 matrix;
		if (transform_bits & 1 << 17) {
			matrix = Transforms.view * Transforms.projection;
			transform_bits &= ~(1 << 17);
		}
		*data = &matrix;
		*size = sizeof(matrix);
		return true;
	};

	DECLARE_BINDING_DIRTY_FUNC(ViewProjection)
	{
		if (transform_bits & 1 << 17) return true;
		return false;
	};

	DECLARE_ROOT_BINDING_CACHED(ViewProjectionTranspose)
	{
		assert(data != NULL);
		assert(size != NULL);
		static Matrix4 matrix;
		if (transform_bits & 1 << 18) {
			matrix = (Transforms.view * Transforms.projection).Transpose();
			transform_bits &= ~(1 << 18);
		}
		*data = &matrix;
		*size = sizeof(matrix);
		return true;
	};

	DECLARE_BINDING_DIRTY_FUNC(ViewProjectionTranspose)
	{
		if (transform_bits & 1 << 18) return true;
		return false;
	};

	DECLARE_ROOT_BINDING_CACHED(ViewProjectionInverse)
	{
		assert(data != NULL);
		assert(size != NULL);
		static Matrix4 matrix;
		if (transform_bits & 1 << 19) {
			matrix = (Transforms.view * Transforms.projection).Inverse();
			transform_bits &= ~(1 << 19);
		}
		*data = &matrix;
		*size = sizeof(matrix);
		return true;
	};

	DECLARE_BINDING_DIRTY_FUNC(ViewProjectionInverse)
	{
		if (transform_bits & 1 << 19) return true;
		return false;
	};

	DECLARE_ROOT_BINDING_CACHED(ViewProjectionInverseTranspose)
	{
		assert(data != NULL);
		assert(size != NULL);
		static Matrix4 matrix;
		if (transform_bits & 1 << 20) {
			matrix = (Transforms.view * Transforms.projection).Inverse().Transpose();
			transform_bits &= ~(1 << 20);
		}
		*data = &matrix;
		*size = sizeof(matrix);
		return true;
	};

	DECLARE_BINDING_DIRTY_FUNC(ViewProjectionInverseTranspose)
	{
		if (transform_bits & 1 << 20) return true;
		return false;
	};

	DECLARE_ROOT_BINDING_CACHED(WorldViewProjection)
	{
		assert(data != NULL);
		assert(size != NULL);
		static Matrix4 matrix;
		if (transform_bits & 1 << 21) {
			matrix = Transforms.world * Transforms.view * Transforms.projection;
			transform_bits &= ~(1 << 21);
		}
		*data = &matrix;
		*size = sizeof(matrix);
		return true;
	};

	DECLARE_BINDING_DIRTY_FUNC(WorldViewProjection)
	{
		if (transform_bits & 1 << 21) return true;
		return false;
	};

	DECLARE_ROOT_BINDING_CACHED(WorldViewProjectionTranspose)
	{
		assert(data != NULL);
		assert(size != NULL);
		static Matrix4 matrix;
		if (transform_bits & 1 << 22) {
			matrix = (Transforms.world * Transforms.view * Transforms.projection).Transpose();
			transform_bits &= ~(1 << 22);
		}
		*data = &matrix;
		*size = sizeof(matrix);
		return true;
	};

	DECLARE_BINDING_DIRTY_FUNC(WorldViewProjectionTranspose)
	{
		if (transform_bits & 1 << 22) return true;
		return false;
	};

	DECLARE_ROOT_BINDING_CACHED(WorldViewProjectionInverse)
	{
		assert(data != NULL);
		assert(size != NULL);
		static Matrix4 matrix;
		if (transform_bits & 1 << 23) {
			matrix = (Transforms.world * Transforms.view * Transforms.projection).Inverse();
			transform_bits &= ~(1 << 23);
		}
		*data = &matrix;
		*size = sizeof(matrix);
		return true;
	};

	DECLARE_BINDING_DIRTY_FUNC(WorldViewProjectionInverse)
	{
		if (transform_bits & 1 << 23) return true;
		return false;
	};

	DECLARE_ROOT_BINDING_CACHED(WorldViewProjectionInverseTranspose)
	{
		assert(data != NULL);
		assert(size != NULL);
		static Matrix4 matrix;
		if (transform_bits & 1 << 24) {
			matrix = (Transforms.world * Transforms.view * Transforms.projection).Inverse().Transpose();
			transform_bits &= ~(1 << 24);
		}
		*data = &matrix;
		*size = sizeof(matrix);
		return true;
	};

	DECLARE_BINDING_DIRTY_FUNC(WorldViewProjectionInverseTranspose)
	{
		if (transform_bits & 1 << 24) return true;
		return false;
	};

	void InitializeTransformBindings()
	{
		REGISTER_BINDING(World);
		REGISTER_BINDING(WorldTranspose);
		REGISTER_BINDING(WorldInverse);
		REGISTER_BINDING(WorldInverseTranspose);
		REGISTER_BINDING(View);
		REGISTER_BINDING(ViewTranspose);
		REGISTER_BINDING(ViewInverse);
		REGISTER_BINDING(ViewInverseTranspose);
		REGISTER_BINDING(Projection);
		REGISTER_BINDING(ProjectionTranspose);
		REGISTER_BINDING(ProjectionInverse);
		REGISTER_BINDING(ProjectionInverseTranspose);
		REGISTER_BINDING(WorldView);
		REGISTER_BINDING(WorldViewTranspose);
		REGISTER_BINDING(WorldViewInverse);
		REGISTER_BINDING(WorldViewInverseTranspose);
		REGISTER_BINDING(ViewProjection);
		REGISTER_BINDING(ViewProjectionTranspose);
		REGISTER_BINDING(ViewProjectionInverse);
		REGISTER_BINDING(ViewProjectionInverseTranspose);
		REGISTER_BINDING(WorldViewProjection);
		REGISTER_BINDING(WorldViewProjectionTranspose);
		REGISTER_BINDING(WorldViewProjectionInverse);
		REGISTER_BINDING(WorldViewProjectionInverseTranspose);
		transform_bits = 0;
	};

	//	WVP	VP	WV	P	V	W
	//	F	0	F	0	0	F // World dependant
	//	F	F	F	0	F	0 // View dependant
	//	F	F	0	F	0	0 // Projection dependant

	void InvalidateWorldCache()
	{
		transform_bits |= 0xF0F00F;
	};

	void InvalidateViewCache()
	{
		transform_bits |= 0xFFF0F0;
	};

	void InvalidateProjectionCache()
	{
		transform_bits |= 0xFF0F00;
	};

};
