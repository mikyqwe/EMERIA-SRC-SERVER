#ifndef __INC_METIN2_GAME_LOCALE_H__
#define __INC_METIN2_GAME_LOCALE_H__

#include "../../common/CommonDefines.h"
#include "../../common/service.h"

extern "C"
{
	void locale_init(const char *filename);
	const char *locale_find(const char *string);
#ifdef ENABLE_MULTI_LANGUAGE_SYSTEM 
	void locale_init_new(BYTE language, const char *filename);
	const char *locale_find_new(BYTE language, const char *string);
#endif

	extern int g_iUseLocale;

#define LC_TEXT(str) locale_find(str)
#ifdef ENABLE_MULTI_LANGUAGE_SYSTEM
#define LC_TEXT_LANGUAGE(language, str) locale_find_new(language, str)
#endif

};

#endif
