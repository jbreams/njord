#pragma once

class VarStore
{
public:
	VarStore(void);
	~VarStore(void);
	BOOL AddVariable(LPTSTR name, LPTSTR string);
	BOOL AddVariable(LPTSTR name, BOOL boolean);
	BOOL AddVariable(LPTSTR name, DWORD number);
	void ParseIntoXML(void * mozView);
	void SuckOutVars(void * MozView);
	void RecursePushIn(void * documentptr, void * parent, void * list);

	enum vartype { STRING, BOOLEAN, NUMBER };
	struct storage
	{
		vartype type;
		union {
			LPTSTR string;
			BOOL boolean;
			DWORD number;
		};
		LPTSTR name;
		struct storage * next;
	} *  head;
	DWORD varCount;
	struct storage * GetVarObj(LPTSTR name, BOOL wipe);
};

#ifndef VARSTORE_OBJ_DEFINED
extern JSClass varStoreClass;
extern JSObject * varStoreProto;
#endif