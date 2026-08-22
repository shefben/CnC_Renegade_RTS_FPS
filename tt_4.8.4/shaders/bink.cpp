#include "General.h"
#include "bink.h"
#include "engine_3d.h"
#include "Subtitle.h"
#include "texture.h"
#include "engine_math.h"


REF_DEF1(BinkMovie::instance, BinkMovie*, 0x0085FD40);



void BinkMovie::BinkRender()
{
	if (binkHandle)
	{
		if (render)
		{
			BinkDoFrame(binkHandle);
			render = false;
			BinkCopyToBuffer(binkHandle, buffer,2 * binkHandle->width, binkHandle->height, 0, 0, 0x7000000A);
			for (int i = 0;i < textureCount;i++)
			{
				if (textures[i].texture->D3DTexture)
				{
					unsigned int width = textures[i].widthstart;
					if (width >= 0)
					{
						if (width >= binkHandle->width)
							width = binkHandle->width - 1;
					}
					else
						width = 0;

					unsigned int height = textures[i].heightstart;
					if (height >= 0)
					{
						if (height >= binkHandle->height)
							height = binkHandle->height - 1;
					}
					else
						height = 0;
					
					char *buf = buffer + 2 * (width + binkHandle->width * height);
					unsigned int texwidth = textures[i].width;
					unsigned int texheight = textures[i].height;
					if (texwidth > binkHandle->width - textures[i].widthstart)
						texwidth = binkHandle->width - textures[i].widthstart;
					
					if (texwidth > binkHandle->width - textures[i].heightstart)
						texwidth = binkHandle->width - textures[i].heightstart;
					
					D3DLOCKED_RECT lockedrect;
					RECT rect;
					rect.right = texwidth;
					rect.left = 0;
					rect.top = 0;
					rect.bottom = texheight;
					textures[i].texture->D3DTexture->LockRect(0,&lockedrect,&rect,0);
					for (unsigned int th = 0;th < texheight;th++)
					{
						char *obuf = ((char *)lockedrect.pBits) + (th * lockedrect.Pitch);
						memcpy(obuf,buf,4 * ((2 * texwidth) >> 2));
						memcpy(&obuf[4 * ((2 * texwidth) >> 2)],&buf[4 * ((2 * texwidth) >> 2)],(2 * texwidth) & 3);
						buf += 2 * binkHandle->width;
					}
					textures[i].texture->D3DTexture->UnlockRect(0);
				}
			}
			if (binkHandle->currentframe < binkHandle->frames)
				BinkNextFrame(binkHandle);
		}
		RectClass r(0,0,1,1);
		for (int i = 0;i < textureCount;i++)
		{
			movieRenderer.Reset();
			movieRenderer.Set_Texture(textures[i].texture);
			movieRenderer.Set_Coordinate_Range(r);
			movieRenderer.Add_Quad(textures[i].position,textures[i].UV,0xFFFFFFFF);
			movieRenderer.Render();
		}

		if (subtitle)
		{
			subtitle->Build(binkHandle->currentframe * unk0010);
			subtitle->Render();
		}
	}
}



void BinkMovie::BinkThink()
{
	if (binkHandle)
		render |= BinkWait(binkHandle) == 0;
}



BinkMovie::BinkMovie(const char* _moviePath, const char* subtitleIniPath, FontCharsClass* subtitleFont) :
	moviePath(_moviePath),
	binkHandle(0),
	render(true),
	unk0010(0),
	movieRenderer(0),
	subtitle(0)
{
	binkHandle = BinkOpen(moviePath, 0);
	if (binkHandle)
	{
		buffer = new char[2 * binkHandle->width * binkHandle->height];

		uint textureWidth = 1;
		while (textureWidth < binkHandle->width)
			textureWidth *= 2;

		if (textureWidth > ShaderCaps::MaxTextureWidth)
			textureWidth = ShaderCaps::MaxTextureWidth;

		uint textureHeight = 1;
		while (textureHeight < binkHandle->height)
			textureHeight *= 2;
		
		if (textureHeight > ShaderCaps::MaxTextureHeight)
			textureHeight = ShaderCaps::MaxTextureHeight;
		
		textureCount = divideRoundUp(binkHandle->height, textureHeight) * divideRoundUp(binkHandle->width, textureWidth);
		textures = new MovieTexture[textureCount];
		
		MovieTexture* texture = &textures[0];
		for (uint y = 0; y < binkHandle->height; y += textureHeight)
		{
			for (uint x = 0; x < binkHandle->width; x += textureWidth)
			{
				texture->texture = new TextureClass(textureWidth, textureHeight, D3DFormat_To_WW3DFormat(D3DFMT_R5G6B5), TextureClass::MIP_LEVELS_1, TextureClass::POOL_MANAGED, false);
				texture->widthstart = x;
				texture->heightstart = y;

				texture->UV.Left = 0;
				texture->UV.Top = 0;
				
				if (textureWidth > binkHandle->width - texture->widthstart)
				{
					texture->width = binkHandle->width - texture->widthstart;
					texture->UV.Right = texture->width / (float)textureWidth;
				}
				else
				{
					texture->width = textureWidth;
					texture->UV.Right = 1.f;
				}
				
				if (textureHeight > binkHandle->height - texture->heightstart)
				{
					texture->height = binkHandle->height - texture->heightstart;
					texture->UV.Bottom = texture->height / (float)textureHeight;
				}
				else
				{
					texture->height = textureHeight;
					texture->UV.Bottom = 1.f;
				}
				
				texture->position.Left = texture->widthstart / (float)binkHandle->width;
				texture->position.Top = texture->heightstart / (float)binkHandle->height;
				texture->position.Right = (texture->widthstart + texture->width) / (float)binkHandle->width;
				texture->position.Bottom = (texture->heightstart + texture->height) / (float)binkHandle->height;

				++texture;
			}
		}

		movieRenderer.Reset();
		unk0010 = 60 / (binkHandle->rate1 / binkHandle->rate2);
		
		if (subtitleIniPath && subtitleFont)
			subtitle = Subtitle::Create(moviePath, subtitleIniPath, subtitleFont);
	}
}



BinkMovie::~BinkMovie()
{
	if (binkHandle)
	{
		BinkClose(binkHandle);
		delete[] buffer;

		if (textures)
		{
			for (int i = 0;i < textureCount;i++)
				REF_PTR_RELEASE(textures[i].texture);
			
			delete[] textures;
		}
		
		delete subtitle;
	}
}



void BinkMovie::Create(const char *moviename,const char *subtitleini,FontCharsClass *subtitlefont)
{
	Destroy();
	instance = new BinkMovie(moviename, subtitleini, subtitlefont);
}



void BinkMovie::Destroy()
{
	SAFE_DELETE(instance);
}



void BinkMovie::Think()
{
	if (instance)
		instance->BinkThink();
}



void BinkMovie::Render()
{
	if (instance)
		instance->BinkRender();
}
