#ifndef TT_INCLUDE_BINK_H
#define TT_INCLUDE_BINK_H
#include "render2d.h"
#include "texture.h"
#include "Rect.h"
struct BinkStruct {
	unsigned int width;
	unsigned int height;
	unsigned int frames;
	unsigned int currentframe;
	unsigned int lastframe;
	unsigned int rate1; //has something to do with frame rate
	unsigned int rate2; //has something to do with frame rate
};
extern "C" {
typedef FARPROC (CALLBACK *BinkSound) (unsigned long sound);
FARPROC WINAPI BinkOpenDirectSound(unsigned long sound);
long WINAPI BinkSetSoundSystem(BinkSound soundfunc,unsigned long sound);
int WINAPI BinkDoFrame(BinkStruct *bink);
void WINAPI BinkClose(BinkStruct *bink);
int WINAPI BinkWait(BinkStruct *bink);
void WINAPI BinkNextFrame(BinkStruct *bink);
BinkStruct *WINAPI BinkOpen(const char *name,unsigned int flags);
int WINAPI BinkCopyToBuffer(BinkStruct *bink,void *dest,int destwidth,int destheight,int destx,int desty,unsigned int flags);
};
class MovieTexture {
public:
	TextureClass *texture; //0
	uint width; //4
	uint height; //8
	uint widthstart; //C
	uint heightstart; //10
	RectClass UV; //14 18 1C 20
	RectClass position; //24 28 2C 30
};
class Subtitle;
class SHADERS_API BinkMovie
{

private:

	static REF_DECL1(instance, BinkMovie*);

	StringClass moviePath; // 0000
	BinkStruct* binkHandle; // 0004
	bool render; // 0008
	int textureCount; // 000C
	int unk0010; // 0010
	MovieTexture* textures; // 0014
	char* buffer; // 0018
	Render2DClass movieRenderer; // 001C
	Subtitle* subtitle; // 05D0

public:

	static void Create(const char *moviename,const char *subtitleini,FontCharsClass *subtitlefont);
	static void Destroy();
	static void Think();
	static void Render();

	void BinkRender();
	void BinkThink();
	BinkMovie(const char *moviename,const char *subtitleini,FontCharsClass *subtitlefont);
	~BinkMovie();

};
#endif
