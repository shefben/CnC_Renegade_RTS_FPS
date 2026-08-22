#ifndef TT_INCLUDE__CONVERSATIONMGRCLASS_H
#define TT_INCLUDE__CONVERSATIONMGRCLASS_H
#include "SaveLoadSubSystemClass.h"


class ActiveConversationClass;
template<typename T> class DynamicVectorClass;
class ConversationClass;
class OratorClass;
class ChunkSaveClass;
class ChunkLoadClass;
class PhysicalGameObj;



class ConversationMgrClass : public SaveLoadSubSystemClass
{

public:

	enum CATEGORY;
	
	static void Think();

	ConversationMgrClass();
	~ConversationMgrClass();

	void Reset_Conversations(int, bool);
	void Reset_All_Other_Conversations(ActiveConversationClass*);
	void Reset_Active_Conversations();
	void Reset();
	uint32 Chunk_ID();
	bool Save(ChunkSaveClass&);
	bool Load_Conversations(ChunkLoadClass&, int);
	bool Load(ChunkLoadClass&);
	bool Load_Variables(ChunkLoadClass&);
	void Build_Buddy_List(PhysicalGameObj*, DynamicVectorClass<PhysicalGameObj*>&, bool);
	ActiveConversationClass* Find_Active_Conversation(int);
	ConversationClass* Find_Conversation(const char*);
	ConversationClass* Find_Conversation(int);
	ActiveConversationClass* Start_Conversation(PhysicalGameObj*, int, bool);
	ActiveConversationClass* Start_Conversation(PhysicalGameObj*, const char*, bool);
	ActiveConversationClass* Start_Conversation(PhysicalGameObj*, ConversationClass*, bool);
	ActiveConversationClass* Create_New_Conversation(ConversationClass*, DynamicVectorClass<PhysicalGameObj*>&);
	ActiveConversationClass* Start_Conversation(PhysicalGameObj*);
	bool Test_Conversation(PhysicalGameObj*, ConversationClass*, DynamicVectorClass<PhysicalGameObj*>&, bool);
	bool Test_Orator(ConversationClass*, OratorClass*, PhysicalGameObj*);
	ConversationClass* Pick_Conversation(PhysicalGameObj*, const DynamicVectorClass<PhysicalGameObj*>&, DynamicVectorClass<PhysicalGameObj*>&);
	bool Is_Key_Conversation_Playing();
	void Add_Conversation(ConversationClass*);
	ActiveConversationClass* Create_New_Conversation(ConversationClass*);
	void Remove_Conversation(ConversationClass*);
	void Initialize();
	void Shutdown();
	int Get_Conversation_Count(int);
	ConversationClass* Peek_Conversation(int, int);
	int Get_Active_Conversation_Count();
	void Set_Category_To_Save(CATEGORY);
	bool Are_Emot_Icons_Displayed();
	void Display_Emot_Icons(bool);
	bool Contains_Data() const;
	const char *Name() const;
};



#endif