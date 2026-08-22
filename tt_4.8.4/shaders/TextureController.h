#ifndef _SHADERS_TEXTURECONTROLLER_H_
#define _SHADERS_TEXTURECONTROLLER_H_
#include "shader.h"
#include "DLListClass.h"

struct eqstr;
class CameraClass;
class RenderTargetTexture;
class RenderTargetController;
class RenderTargetDatabase;
class RenderTargetDatabase: public RefCountClass
{
protected:
	friend class RenderTargetController;
	typedef google::dense_hash_map<const char *, RenderTargetTexture *, stdext::hash_compare<const char *>, eqstr> HashMap;
	HashMap Textures;
public:
	RenderTargetDatabase();
	RenderTargetDatabase(const RenderTargetDatabase&);
	~RenderTargetDatabase();

	void Load(ChunkLoadClass& load);
	bool LoadFromINI(INIClass* ini, const char* key);
	void Save(ChunkSaveClass& save);

	void Unload();

	void AddTexture(RenderTargetTexture* tex);
	void RemoveTexture(RenderTargetTexture* tex);
	RenderTargetTexture* QueryTexture(const char* name);


};

class SHADERS_API RenderTargetTexture: public RefCountClass, public DLNodeClass<RenderTargetTexture>
{
protected:
	friend class RenderTargetDatabase;
	friend class RenderTargetController;
	const char* Name;
	float AspectRatio;
	unsigned int TextureWidth, TextureHeight; // might not support non-square textures -_-
	WW3DFormat TextureFormat;
	RenderTargetDatabase* Container;
	TextureClass* Texture;
	unsigned int IdleTime;
	unsigned int LastRenderTime;
public:
	RenderTargetTexture();
	RenderTargetTexture(RenderTargetDatabase*);
	RenderTargetTexture(const RenderTargetTexture&);
	~RenderTargetTexture();

	virtual void Load(ChunkLoadClass& load);
	virtual bool LoadFromINI(INIClass* ini, const char* key);
	virtual void Save(ChunkSaveClass& save);

	inline unsigned int GetIdleTime() const
	{
		return IdleTime;
	};

	inline unsigned int GetLastRenderTime() const
	{
		return LastRenderTime;
	};

	unsigned int GetTextureLastAccessTime();
	void SetTexture(TextureClass* tex);
	TextureClass* PeekTexture();
	
	virtual void Render();
};

class PhysClass;
class CameraRenderTargetTexture: public RenderTargetTexture
{
protected: 
	PhysClass* obj;
public: 
	CameraRenderTargetTexture(RenderTargetDatabase*);
	CameraRenderTargetTexture(const CameraRenderTargetTexture&);
	virtual ~CameraRenderTargetTexture();

	void AttachObject(PhysClass* obj);
	void Render();
};

class SHADERS_API RenderTargetController: public RefCountClass 
{
private:
	DLListClass<RenderTargetTexture> pending;
	CameraClass* cam;
	RenderTargetDatabase* active_db;
protected:
	RenderTargetController();
	RenderTargetController(const RenderTargetController&);
public:
	static RenderTargetController* Instance()
	{
		static RenderTargetController* inst = new RenderTargetController();
		return inst;
	};
	virtual ~RenderTargetController();

	void SetCamera(CameraClass* cam);
	CameraClass* PeekCamera();
	DLListClass<RenderTargetTexture>& GetPendingTextures();

	bool HasReplacementTexture(const char* filename);
	TextureClass* GetReplacementTexture(const char* filename);

	void Load(ChunkLoadClass& load);
	bool LoadFromINI(INIClass* ini);
	void Save(ChunkSaveClass& save);
};

#endif
