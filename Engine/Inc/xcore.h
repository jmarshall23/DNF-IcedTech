#ifndef _XCORE_H_
#define _XCORE_H_

#include <stdlib.h>   // For _MAX_PATH definition
#include <stdio.h>
#include <malloc.h>

#ifndef _XTYPES_H_
#include <xtypes.h>
#endif
#ifndef _INC_STRING
#include <string.h>
#endif

/* if windows.h is included */
#ifdef _WINDOWS_
#define XHANDLE HANDLE
#define XHWND   HWND
#define INVALID_XHANDLE_VALUE INVALID_HANDLE_VALUE
#define X_WIN32_FIND_DATA WIN32_FIND_DATAA
#else
#define XHANDLE XHandle
#define XHWND   XHandle
#define INVALID_XHANDLE_VALUE (XHandle)-1
#define X_WIN32_FIND_DATA XWin32FindData
#endif

/* disable DLL warnings */
#pragma warning(disable:4251)
#pragma warning(disable:4275)

/* disable infix warning */
#pragma warning(disable:4284)


__forceinline void _xheap_check(void) {

}

__forceinline voidp xmalloc(U32 size) {
	return malloc(size);
}

__forceinline voidp _xrealloc(void* ptr, U32 size) {
	return realloc(ptr, size);
}
__forceinline voidp xmalloc_tmp(U32 size) {
	return malloc(size);
}
__forceinline void xfree(void* ptr) {
	free(ptr);
}

__forceinline voidp xmalloc2(U32 size) {
	return malloc(size);
}

__forceinline void xfree2(void* ptr) {
	free(ptr);
}

__forceinline voidp xmalloc3(U32 size) {
	return malloc(size);
}

__forceinline void xfree3(void* ptr) {
	free(ptr);
}

__forceinline voidp xmalloc4(U32 size) {
	return malloc(size);
}

__forceinline void xfree4(void* ptr) {
	free(ptr);
}


namespace NS_STRING
{
	enum special_keys
	{
		KEY_WHITE		=0x01,
		KEY_NEWLINE		=0x02,
		KEY_ALPHA		=0x04,
		KEY_DIGIT		=0x08,
		KEY_LOWER		=0x10,
		KEY_UPPER		=0x20,
		KEY_HEX_ALPHA	=0x40,
		KEY_HEX			=(KEY_DIGIT | KEY_HEX_ALPHA),
		KEY_QUOTE		=0x80
	};
}
extern CU8 _app_char_flags[256];

extern "C" {
#define MAX_PRINTF_SIZE 4096
extern char  _err_printf_string[MAX_PRINTF_SIZE];
}

extern "C" {
/* bit scan forward support */
 U32 __regcall(1) _bsf(U32 value);
 U32 __regcall(1) _bsfs(U32 *value);
/* string functions */
 U32 __regcall(1) fstrlen(CC8 *str);
 charp __regcall(2) fstrcpy(char *dst,CC8 *src);
 U32 __regcall(3) fstrncpy(char *dst,CC8 *src,U32 size);
 charp __regcall(2) fstrcpy_tolower(char *dst,CC8 *src);
 U32 __regcall(3) fstrncpy_tolower(char *dst,CC8 *src,U32 size);
 U32 __regcall(2) fstrexp_eq(CC8 *exp,CC8 *str);
 U32 __regcall(2) fstreq(CC8 *str1,CC8 *str2);
 U32 __regcall(3) fstrneq(CC8 *str1,CC8 *str2,U32 size);
 charp __regcall(2) fstrchr(CC8 *str,char key);
 U32 __regcall(2) fitoa(I32 num,char *str);
 U32 __regcall(2) futoa(U32 num,char *str);

 U32 __regcall(2) hex32(U32 val,char *ptr);
 U32 hex64(U64 val,char *ptr);

 CC8P __regcall(3) fscan_f(CC8 *&str,CU8 *flag_list,U32 flags);
 CC8P __regcall(3) fscan_nf(CC8 *&str,CU8 *flag_list,U32 flags);
 CC8P __regcall(3) fstrnrchr(CC8 *text,U32 key,U32 size);
 CC8P __regcall(3) fstrprchr(CC8 *text,U32 key,CC8 *end);

 CC8P __regcall(2) fpath_append(CC8 *path,CC8 *more);
 CC8P __regcall(2) fclean_path(CC8 *path,U32 len);
 CC8P __regcall(1) fget_extension(CC8 *path);
 charp __regcall(1) fset_extension(char *path,CC8 *the_ext);
 CC8P __regcall(1) fget_filename(CC8 *path);
 U32 __regcall(1) file_exist(CC8 *path);

#define fscan_white(x) fscan_f(x,_app_char_flags,NS_STRING::KEY_WHITE)
/* ascii flag macros */
#define is_alpha(x) (_app_char_flags[(U8)x] & NS_STRING::KEY_ALPHA)
#define is_hex(x) (_app_char_flags[(U8)x] & NS_STRING::KEY_HEX)
#define is_digit(x) (_app_char_flags[(U8)x] & NS_STRING::KEY_DIGIT)
#define is_newline(x) (_app_char_flags[(U8)x] & NS_STRING::KEY_NEWLINE)
#define is_white(x) (_app_char_flags[(U8)x] & NS_STRING::KEY_WHITE)

#define fsetlower(x) (x + (_app_char_flags[((U8)(x))] & NS_STRING::KEY_UPPER))

void begin_tick(U64 *tick);
void end_tick(U64 *tick);

 U32 _test_leak(void);
}

enum _gbl_object_mem_enum{TMP_MEMORY=1};

#ifdef XCORE_PURE
/* override global new and delete operators */
__inline void *operator new(size_t size){return xmalloc(size);}
__inline void operator delete(void *ptr){xfree(ptr);}
#endif XCORE_PURE

#define XOBJ_DEFINE() \
	void *operator new(size_t size){return xmalloc(size);} \
	void operator delete(void *ptr){xfree(ptr);}

/* Window Popup Message */
 void WinMessage(XHWND hwnd,CC8 *string);
/* Console Message */
 void ConMessage(CC8 *string);



/* include interprocess communication */
//#ifndef _XIPC_H_
//#include <xipc.h>
//#endif

#endif /* ifndef _XCORE_H_ */