#ifndef TT_INCLUDE__TEXTDISPLAYGAMEMODECLASS_H
#define TT_INCLUDE__TEXTDISPLAYGAMEMODECLASS_H

#include "engine_common.h"
#include "engine_string.h"
#include "engine_vector.h"
#include "GameModeClass.h"
#include "SList.h"



struct TextDisplayLine
{
	WideStringClass unk0000; // 0000
	UNK unk0004; // 0004
	UNK unk0008; // 0008
	UNK unk000C; // 000C

	TextDisplayLine(WideStringClass&, uint32);
}; // 0010



class TextDisplayGameModeClass :
	public GameModeClass
{

private:

	uint32                              u8;
	uint32                              u12;
	uint32                              u16;
	uint32                              u20;
	uint32                              u24;
	DynamicVectorClass<WideStringClass> o28;
	DynamicVectorClass<uint32>          o52;
	uint32                              u76;
	SList<TextDisplayLine>              o80;
	StringClass                         o92;
	StringClass                         o96;
	StringClass                         o100;
	uint32                              u104;
	uint32                              u108;
	uint32                              u112;
	uint32                              u116;


public:

	static REF_DECL2(Instance, TextDisplayGameModeClass *);
	virtual const char* Name     () const;
	virtual void        Init     ();
	virtual void        Shutdown ();
	virtual void        Render   ();
	virtual void        Think    ();
	void Print_System(const char* format, ...);
	void Flush();

};


#endif
