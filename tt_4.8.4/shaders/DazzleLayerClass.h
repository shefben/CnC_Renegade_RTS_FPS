#ifndef TT_INCLUDE__DAZZLELAYERCLASS_H
#define TT_INCLUDE__DAZZLELAYERCLASS_H



class DazzleRenderObjClass;
class CameraClass;



class DazzleLayerClass
{

private:

	DazzleRenderObjClass** visible_lists;

public:

	DazzleLayerClass();
	~DazzleLayerClass();
	virtual int Get_Visible_Item_Count(unsigned int type);
	virtual void Clear_Visible_List(unsigned int type);
	void Render(CameraClass* camera);

};



#endif
