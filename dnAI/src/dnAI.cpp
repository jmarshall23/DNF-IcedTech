// dnGame.cpp
//

#include <Engine.h>
#include "dnAI.h"

DnRand dnRand;

#define DN_FORCE_NAME_EXPORT

// Register things.
#define NAMES_ONLY
#define AUTOGENERATE_NAME(name) DNAI_API FName DNAI_##name;
#define AUTOGENERATE_FUNCTION(cls,idx,name) IMPLEMENT_FUNCTION(cls,idx,name)
#include "dnAIClasses.h"
#undef AUTOGENERATE_FUNCTION
#undef AUTOGENERATE_NAME
#undef NAMES_ONLY

IMPLEMENT_PACKAGE(dnAI);

