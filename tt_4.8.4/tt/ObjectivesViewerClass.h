#ifndef TT_INCLUDE__OBJECTIVESVIEWERCLASS_H
#define TT_INCLUDE__OBJECTIVESVIEWERCLASS_H

class TextWindowClass;

class ObjectivesViewerClass
{

private:

	bool unk; // 0000
	TextWindowClass *textwindow; // 0004

public:
	
	ObjectivesViewerClass();
	~ObjectivesViewerClass();
	void Shutdown();
	void Initialize();
	void Update();
	void Display(bool);
	void Page_Down();
	void Render();
	int fnCompareObjectivesCallback(const void*, const void*);


};



#endif