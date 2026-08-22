#ifndef TT_INCLUDE__TEXTURELOADER_H
#define TT_INCLUDE__TEXTURELOADER_H

#include "CriticalSectionClass.h"
#include "engine_3dre.h"
#include "ThreadClass.h"
#include "DLListClass.h"
#include "HashTemplateClass.h"
class TextureLoadTaskClass;
class StringClass;
class TextureClass;

class TextureLoadTaskListClass
{
public:
	TextureLoadTaskClass *Head; //0
	TextureLoadTaskClass *Tail; //4
	int unk8; //8
	TextureLoadTaskListClass();
	void Push_Front(TextureLoadTaskClass *task);
	void Push_Back(TextureLoadTaskClass *task);
	TextureLoadTaskClass *Pop_Front();
	TextureLoadTaskClass *Pop_Back();
	void Remove(TextureLoadTaskClass *task);
};

class SynchronizedTextureLoadTaskListClass : public TextureLoadTaskListClass {
public:
	class FastCriticalSectionClass mutex; //C
	SynchronizedTextureLoadTaskListClass();
	void Push_Front(TextureLoadTaskClass *task);
	void Push_Back(TextureLoadTaskClass *task);
	TextureLoadTaskClass *Pop_Front();
	TextureLoadTaskClass *Pop_Back();
	void Remove(TextureLoadTaskClass *task);
};

class TextureLoadTaskClass
{
public:
	enum TaskType {
		Thumbnail = 1,
		Normal = 2,
	};
	enum PriorityType {
		Background = 0,
		Foreground = 1,
	};
	TextureLoadTaskClass *Succ; //0
	TextureLoadTaskClass *Prev; //4
	TextureLoadTaskListClass *Parent; //8
	TextureClass *Texture; //C
	IDirect3DTexture9 *D3DTexture; //10
	WW3DFormat Format; //14
	uint Width; //18
	uint Height; //1C
	unsigned int MipLevelCount; //20
	unsigned int Reduction; //24
	unsigned char* LockedSurfacePtr[14]; //28
	unsigned int LockedSurfacePitch[14]; //58
	TaskType tasktype; //88
	PriorityType prioritytype; //8C
	int CurrentLoadStep; //90


	TextureLoadTaskClass();
	~TextureLoadTaskClass();
	static TextureLoadTaskClass *Create(TextureClass *texture,TaskType tasktype,PriorityType prioritytype);
	void Destroy();
	static void Delete_Free_Pool();
	void Init(TextureClass *texture,TaskType tasktype,PriorityType prioritytype);
	void Deinit();
	bool Begin_Load();
	bool Load();
	void End_Load();
	void Finish_Load();
	void Apply_Missing_Texture();
	void Apply(bool b);
	bool Begin_Compressed_Load();
	bool Begin_Uncompressed_Load();
	void Lock_Surfaces();
	void Unlock_Surfaces();
	bool Load_Compressed_Mipmap();
	bool Load_Uncompressed_Mipmap();
	unsigned char *Get_Locked_Surface_Ptr(int Miplevel);
	unsigned int Get_Locked_Surface_Pitch(int Miplevel);
};

class TextureLoader
{

public:

	static bool TextureLoadSuspended;

	static void Init();
	static void Deinit();
	static bool Is_DX8_Thread();
	static void Validate_Texture_Size(uint&, uint&);
	static IDirect3DTexture9 *Load_Thumbnail(const StringClass&);
	static IDirect3DSurface9 *Load_Surface_Immediate(const StringClass&, WW3DFormat, bool);
	static void Request_Thumbnail(TextureClass*);
	static void Request_Background_Loading(TextureClass*);
	static void Request_Foreground_Loading(TextureClass*);
	SHADERS_API static void Flush_Pending_Load_Tasks();
	SHADERS_API static void Update(void (*)(void));
	SHADERS_API static void Suspend_Texture_Load();
	SHADERS_API static void Continue_Texture_Load();
	static void Process_Foreground_Thumbnail(TextureLoadTaskClass*);
	static void Process_Foreground_Load(TextureLoadTaskClass*);
	static void Begin_Load_And_Queue(TextureLoadTaskClass*);
	static void Load_Thumbnail(TextureClass*);


};

#pragma pack(push,1)
struct _TGAHeader {
	char IDLength;
	char ColorMapType;
	char ImageType;
	short CMapStart;
	short CMapLength;
	char CMapDepth;
	short XOffset;
	short YOffset;
	short Width;
	short Height;
	char PixelDepth;
	char ImageDescriptor;
};

struct _TGA2DateStamp {
	short Month;
	short Day;
	short Year;
};

struct _TGA2TimeStamp {
	short Hour;
	short Minute;
	short Second;
};

struct _TGA2SoftVer {
	short Number;
	char Letter;
};

struct _TGA2Ratio {
	short Numer;
	short Denom;
};



struct _TGA2Extension {
	short ExtSize;
	char AuthName[41];
	char AuthComment[324];
	_TGA2DateStamp Date;
	_TGA2TimeStamp Time;
	char JobName[41];
	_TGA2TimeStamp JobTime;
	char SoftID[41];
	_TGA2SoftVer SoftVer;
	long KeyColor;
	_TGA2Ratio Aspect;
	_TGA2Ratio Gamma;
	long ColorCor;
	long PostStamp;
	long ScanLine;
	char Attributes;
};

struct _TGA2Footer {
	long Extension;
	long Developer;
	char Signature[16];
	char RsvdChar;
	char BZST;
};
#pragma pack(pop)
class Targa {
public:
	_TGAHeader Header; //0
	FileClass* TGAFile; //14
	long mAccess; //18
	long mFlags; //1C
	char* mImage; //20
	char* mPalette; //24
	_TGA2Extension mExtension; //28
	Targa();
	~Targa();
	long Open(const char* name,long mode);
	void Close();
	long Load(const char* name,long flags,bool invert_image);
	long Load(const char* name,char* palette,char* image,bool invert_image);
	char*SetPalette(char* buffer);
	void Clear_File();
	bool Is_File_Open();
	int File_Seek(int pos,int dir);
	int File_Read(void* buffer,int size);
};

class BitmapHandlerClass {
public:
	static void Copy_Image_Generate_Mipmap(unsigned int width, unsigned int height, unsigned char* dest_surface, unsigned int dest_pitch, WW3DFormat dest_format, unsigned char* src_surface, unsigned int src_pitch, WW3DFormat src_format, unsigned char* mip_surface, unsigned int mip_pitch);
	static void Copy_Image(unsigned char* dest_surface, unsigned int dest_surface_width, unsigned int dest_surface_height, unsigned int dest_surface_pitch, WW3DFormat dest_surface_format, unsigned char* src_surface,unsigned int src_surface_width, unsigned int src_surface_height, unsigned int src_surface_pitch, WW3DFormat src_surface_format, unsigned char* src_palette, unsigned int src_palette_bpp, bool generate_mip_level);
};

class LoaderThreadClass : public ThreadClass {
public:
	virtual ~LoaderThreadClass();
	SHADERS_API virtual void Thread_Function();
};

class ThumbnailClass;
class ThumbnailManagerClass : public DLNodeClass<ThumbnailManagerClass> {
public:
	static DLListClass<ThumbnailManagerClass> ThumbnailManagerList;
	bool unkC; //C
	bool ReadFile; //D
	StringClass ThumbFilename; //10
	StringClass MixFilename; //14
	HashTemplateClass<StringClass,ThumbnailClass *> Hash; //18
	unsigned char *ThumbnailBits; //28
	bool Loading; //2C
	unsigned int Time; //30

	void Create_Thumbnails();
	void Load();
	void Save(bool force);
	ThumbnailManagerClass(const char *thumbfilename,const char *mixfilename);
	~ThumbnailManagerClass();
	SHADERS_API static void Add_Thumbnail_Manager(const char *thumbfilename,const char *mixfilename);
	static void Remove_Thumbnail_Manager(const char *thumbfilename);
	ThumbnailClass *Peek_Thumbnail_Instance(const StringClass &texture);
	void Insert_To_Hash(ThumbnailClass *thumbnail);
	ThumbnailClass *Get_From_Hash(const StringClass &texture);
	void Remove_From_Hash(ThumbnailClass *thumbnail);
	static void Update_Thumbnail_File(const char *thumbfilename,bool b);
	static void Init();
	static void Deinit();
};



class ThumbnailClass {
public:
	StringClass TextureFilename; //0
	unsigned char *ThumbnailBits; //4
	int Width; //8
	int Height; //C
	int MaxWidth; //10
	int MaxHeight; //14
	int MipLevels; //18
	WW3DFormat Format; //1C
	WW3DFormat ThumbnailFormat;
	unsigned int Time; //20
	bool IsAllocated; //24
	ThumbnailManagerClass *ThumbnailManager; //28
	ThumbnailClass(ThumbnailManagerClass *manager,char  const*texture,unsigned char *bits,unsigned int width,unsigned int height,unsigned int maxwidth,unsigned int maxheight,unsigned int miplevels,WW3DFormat format,WW3DFormat thumbnailformat,bool isallocated,unsigned long time);
	ThumbnailClass(ThumbnailManagerClass *manager,const StringClass &texture);
	~ThumbnailClass();
};



#endif
