#ifndef TT_INCLUDE__NETWORKOBJECTFACTORYMGRCLASS_H
#define TT_INCLUDE__NETWORKOBJECTFACTORYMGRCLASS_H



class NetworkObjectFactoryClass;



class NetworkObjectFactoryMgrClass
{

public:

   static REF_DECL2(_FactoryListHead, NetworkObjectFactoryClass*);


   static NetworkObjectFactoryClass* Find_Factory       (uint32 uClassID);
   static NetworkObjectFactoryClass* Get_First          ();
   static NetworkObjectFactoryClass* Get_Next           (NetworkObjectFactoryClass* oFactory);
   static void                       Link_Factory       (NetworkObjectFactoryClass* oFactory);
   static void                       Register_Factory   (NetworkObjectFactoryClass* oFactory);
   static void                       Unlink_Factory     (NetworkObjectFactoryClass* oFactory);
   static void                       Unregister         (uint32 uClassID);
   static void                       Unregister_Factory (NetworkObjectFactoryClass* oFactory);

};


#endif
