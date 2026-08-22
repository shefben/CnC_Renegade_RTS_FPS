#ifndef TT_INCLUDE__PROTOTYPECLASS_H
#define TT_INCLUDE__PROTOTYPECLASS_H
class RenderObjClass;
class PrototypeClass {
public:
	PrototypeClass* NextHash;
	virtual ~PrototypeClass();
	virtual const char *Get_Name() const = 0;
	virtual int Get_Class_ID() const = 0;
	virtual RenderObjClass *Create() = 0;
};

#endif
