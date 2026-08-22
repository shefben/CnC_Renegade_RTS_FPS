#ifndef TT_INCLUDE_W3DBINDINGS_TRANSFORM_H
#define TT_INCLUDE_W3DBINDINGS_TRANSFORM_H
namespace W3DBindings
{
	void InitializeTransformBindings();
	void InvalidateWorldCache();
	void InvalidateViewCache();
	void InvalidateProjectionCache();
};
#endif
