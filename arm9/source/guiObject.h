#ifndef _GUI_OBJECT_H
#define _GUI_OBJECT_H

#define GUI_DEBUG

#include "guiEvent.h"

struct _TGuiObject;

#define GUIHANDLER_ARGS (struct _TGuiObject* __pObj__, TGuiEventID e, void* __arg__)

typedef TGuiEventReturn (*TGuiEventHandler) GUIHANDLER_ARGS;

typedef struct _TGuiTypeHeader {
	TGuiEventHandler handler;
	const struct _TGuiTypeHeader* pParent;
	u32 size;
	const char* szName;
} TGuiTypeHeader;

typedef enum _TGuiObjectFlags {
	GUIOBJ_RENDERDIRTY = 1 << 0, //object needs render event
	GUIOBJ_RENDERCLEAR = 1 << 1, //object bounds should be cleared before render event
} TGuiObjectFlags;

typedef struct _TGuiObject {
	const TGuiTypeHeader* pType;
	struct _TGuiObject* pParent; //link to my parent
	struct _TGuiObject* pChildren; //link to my first child
	struct _TGuiObject* pNextChild; //link to next child in list of parent's children
	TGuiEventHandler handler;
	u32 flags;
	TBounds bounds;
} TGuiObject;

#define GUITYPE_HEADER(typeof) __##typeof##TypeHeader__
#define GUITYPE_HANDLER(typeof) __##typeof##TypeHandler__

#define GUIOBJ_DECLARE_CHILD(parentTypeof, typeof) \
	extern const TGuiTypeHeader GUITYPE_HEADER(typeof); \
	struct _##typeof; \
	typedef struct _##typeof typeof; \
	struct _##typeof { \
		parentTypeof parent;

#define GUIOBJ_DECLARE(typeof) GUIOBJ_DECLARE_CHILD(TGuiObject, typeof)

#define GUIOBJ_IMPLEMENT_HANDLER(typeof, name) \
	static TGuiEventReturn name GUIHANDLER_ARGS { \
		typeof* this ATTR_UNUSED = (typeof*)__pObj__; \
		TGuiEventReturn __ret__ = GUIEVENTRET_NOTHANDLED; \
		switch(e) { \
		case GUIEVENT_NULL: {

#define GUIOBJ_IMPLEMENT_CHILD(parentTypeof, typeof) \
	extern const TGuiTypeHeader GUITYPE_HEADER(parentTypeof); \
	static TGuiEventReturn GUITYPE_HANDLER(typeof) GUIHANDLER_ARGS; \
	const TGuiTypeHeader GUITYPE_HEADER(typeof) = { \
		GUITYPE_HANDLER(typeof), \
		&GUITYPE_HEADER(parentTypeof), \
		sizeof(typeof), \
		#typeof \
	}; \
	GUIOBJ_IMPLEMENT_HANDLER(typeof, GUITYPE_HANDLER(typeof))

#define GUIOBJHANDLER_IMPLEMENT(id) \
		break; }\
	case id: { \
		GUIEVENT_ARGTYPE(id)* arg ATTR_UNUSED = (GUIEVENT_ARGTYPE(id)*)__arg__;

#define GUIOBJHANDLER_HANDLE() __ret__ = GUIEVENTRET_HANDLED

#define GUIOBJ_IMPLEMENT_END() \
			break; } \
		default: \
			break; \
		} \
		return __ret__; \
	}

#define GUIOBJ_IMPLEMENT(typeof) GUIOBJ_IMPLEMENT_CHILD(TGuiObject, typeof)

typedef void (*TGuiObjIterator)(TGuiObject* pObj, void* arg);

//TGuiObject functions
void guiObjGetGlobalBounds(const TGuiObject* this, TBounds* pBounds);
void guiObjRenderDirty(TGuiObject* this);
TGuiEventHandler guiObjSetHandler(TGuiObject* this, TGuiEventHandler handler);
TGuiEventReturn guiObjSendEvent(TGuiObject* this, TGuiEventID e, void* arg);
void guiObjForeachChild(const TGuiObject* this, TGuiObjIterator iterator, void* arg);
bool guiObjIsType_r(const TGuiObject* this, const TGuiTypeHeader* pType);

#define guiObjIsType(this, typeof) guiObjIsType_r(this, &GUITYPE_HEADER(typeof))
#define guiObjDynamicCast(this, typeof) (guiObjIsType_r(this, &GUITYPE_HEADER(typeof)) ? (typeof*)(this) : NULL)

#ifdef GUI_DEBUG
#define guiObjStaticCast(this, typeof) \
	(guiObjIsType_r(this, &GUITYPE_HEADER(typeof)) ? (typeof*)(this) : NULL)
#else
#define guiObjStaticCast(this, typeof) (typeof*)(this)
#endif

//inlines
static inline s32 guiObjGetHeight(const TGuiObject* this) {
	return this->bounds.y1 - this->bounds.y0 + 1;
}
static inline bool guiObjIsRoot(const TGuiObject* this) {
	//this == NULL is safety check, replace with ASSERT later
	return this == NULL || this->pParent == NULL;
}

#endif
