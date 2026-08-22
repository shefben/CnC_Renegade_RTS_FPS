#ifndef TT_INCLUDE__CDIAGNOSTICS_H
#define TT_INCLUDE__CDIAGNOSTICS_H
class Render2DTextClass;
class Font3DInstanceClass;


class cDiagnostics
{

public:

	static REF_DECL2(PRenderer, Render2DTextClass*);
	static REF_DECL2(PFont, Font3DInstanceClass*);
	static REF_DECL2(DiagnosticX, float);
	static REF_DECL2(DiagnosticY, float);

	static void Init();
	static void Close();
	static void Show_Object_Tally();
	static void Add_Diagnostic(const char*, ...);
	static void Render();

};



#endif