// dnGame.cpp
//

#include <Engine.h>

#define DN_FORCE_NAME_EXPORT

// Register things.
#define NAMES_ONLY
#define AUTOGENERATE_NAME(name) EDITOR_API FName DNGAME_##name;
#define AUTOGENERATE_FUNCTION(cls,idx,name) IMPLEMENT_FUNCTION(cls,idx,name)
#include "dnGameClasses.h"
#undef AUTOGENERATE_FUNCTION
#undef AUTOGENERATE_NAME
#undef NAMES_ONLY

IMPLEMENT_PACKAGE(dnGame);