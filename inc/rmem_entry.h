/*
 * Copyright (c) 2017 by Milos Tosic. All Rights Reserved.
 * License: http://www.opensource.org/licenses/BSD-2-Clause
 *
 * MTuner SDK header
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE
 */

#ifndef __RMEM_RMEM_ENTRY_H__
#define __RMEM_RMEM_ENTRY_H__

#include "rmem.h"
#include <stddef.h> // size_t

#if RMEM_PLATFORM_WINDOWS || RMEM_PLATFORM_XBOX360
#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */
	void rmemHookAllocs(int);
	void rmemUnhookAllocs();
#ifdef __cplusplus
}
#endif /* __cplusplus */
#endif /* RMEM_PLATFORM_WINDOWS */

#ifdef __GNUC__

#if RMEM_PLATFORM_WINDOWS
	#define WINDOWS_LEAN_AND_MEAN
	#include <windows.h>
	/* MinGW doesn't have _malloc_init, memalign and reallocalign */
	#define RMEM_NO_MALLOC_INIT
	#define RMEM_NO_MEMALIGN
	#define RMEM_NO_REALLOCALIGN
	/* MinGW has no malloc_usable_size */
	#include <malloc.h>
	#define malloc_usable_size _msize
#endif /* RMEM_PLATFORM_WINDOWS */

#if RMEM_PLATFORM_LINUX
	#define RMEM_NO_MALLOC_INIT
	#define RMEM_NO_MEMALIGN
	#define RMEM_NO_REALLOCALIGN
	#define RMEM_NO_EXPAND
	#include <malloc.h>
#endif /* RMEM_PLATFORM_LINUX */

#if RMEM_PLATFORM_PS3 || RMEM_PLATFORM_PS4
	#include <stdlib.h>	/* malloc_usable_size */
	#define RMEM_NO_EXPAND
#endif

#if RMEM_PLATFORM_PS4
	#define RMEM_NO_MALLOC_INIT
#endif

#if RMEM_PLATFORM_ANDROID
	#define RMEM_NO_MALLOC_INIT
	#define RMEM_NO_REALLOCALIGN
	#define RMEM_NO_EXPAND
	extern "C" { extern size_t dlmalloc_usable_size(const void* p);	}
	#define malloc_usable_size dlmalloc_usable_size
#endif

#ifdef __cplusplus
	#define RMEM_EXTERN_C_BEGIN	extern "C" {
	#define RMEM_EXTERN_C_END	}
#else
	#define RMEM_EXTERN_C_BEGIN
	#define RMEM_EXTERN_C_END
#endif /* __cplusplus */

	RMEM_EXTERN_C_BEGIN
	static inline uint32_t mallocGetOverhead(void* _ptr, size_t _size)
	{
		size_t usableSize = malloc_usable_size(_ptr);
		return usableSize - _size;
	}
	RMEM_EXTERN_C_END

#ifdef RMEM_NO_MALLOC_INIT

#if RMEM_PLATFORM_WINDOWS

	/* In case we can't wrap _malloc_init we will use function attributes
	* to de/initialize mtuner library */

	#define RMEM_GCC_ENTRY_INIT																								\
		RMEM_EXTERN_C_BEGIN																									\
		void __attribute__ ((constructor(101))) memwrap_init(void)															\
		{																													\
			rmemHookAllocs(1);																								\
		}																													\
																															\
		void __attribute__ ((destructor(1001))) memwrap_shutdown(void)														\
		{																													\
			rmemUnhookAllocs();																								\
		}																													\
		RMEM_EXTERN_C_END
#else /* RMEM_PLATFORM_WINDOWS */

	#define RMEM_GCC_ENTRY_INIT																								\
		RMEM_EXTERN_C_BEGIN																									\
		void __attribute__ ((constructor(101))) memwrap_init(void)															\
		{																													\
			rmemInit(0);																									\
		}																													\
																															\
		void __attribute__ ((destructor(1001))) memwrap_shutdown(void)														\
		{																													\
			rmemShutDown();																									\
		}																													\
		RMEM_EXTERN_C_END
#endif /* RMEM_PLATFORM_WINDOWS */

#endif /* RMEM_NO_MALLOC_INIT */

#ifndef RMEM_NO_MALLOC_INIT
	#define RMEM_GCC_ENTRY_INIT																								\
		RMEM_EXTERN_C_BEGIN																									\
		void* __real__malloc_init(size_t _alignment, size_t _blockSize);													\
		void* __wrap__malloc_init(size_t _alignment, size_t _blockSize)														\
		{																													\
			rmemInit(0);																									\
			atexit(rmemShutDown);																							\
			void* result = __real__malloc_init(_alignment, _blockSize);														\
			return result;																									\
		}																													\
		RMEM_EXTERN_C_END
#endif /* RMEM_NO_MALLOC_INIT */

	#define RMEM_GCC_ENTRY_WRAP_1																							\
		RMEM_EXTERN_C_BEGIN																									\
		void* __real_malloc(size_t _blockSize);																				\
		void* __wrap_malloc(size_t _blockSize)																				\
		{																													\
			void* result = __real_malloc(_blockSize);																		\
			rmemAlloc(0, result, _blockSize, mallocGetOverhead(result,_blockSize));											\
			return result;																									\
		}																													\
																															\
		void* __real_realloc(void* _ptr, size_t _blockSize);																\
		void* __wrap_realloc(void* _ptr, size_t _blockSize)																	\
		{																													\
			void* result = __real_realloc(_ptr, _blockSize);																\
			rmemRealloc(0, result, _blockSize, mallocGetOverhead(result,_blockSize), _ptr);									\
			return result;																									\
		}																													\
																															\
		void* __real_calloc(size_t _numElements, size_t _elementSize);														\
		void* __wrap_calloc(size_t _numElements, size_t _elementSize)														\
		{																													\
			void* result = __real_calloc( _numElements, _elementSize);														\
			size_t totalSize = _numElements*_elementSize;																	\
			rmemAlloc( 0, result, totalSize, mallocGetOverhead(result,totalSize));											\
			return result;																									\
		}																													\
																															\
		void __real_free(void* _ptr);																						\
		void __wrap_free(void* _ptr)																						\
		{																													\
			if (!_ptr)																										\
				return;																										\
																															\
			rmemFree(0, _ptr);																								\
			__real_free(_ptr);																								\
		}																													\
		RMEM_EXTERN_C_END

#ifndef RMEM_NO_EXPAND
	#define RMEM_GCC_ENTRY_WRAP_2																							\
		RMEM_EXTERN_C_BEGIN																									\
		void* __real__expand(void* _ptr, size_t _blockSize);																\
		void* __wrap__expand(void* _ptr, size_t _blockSize)																	\
		{																													\
			void* result = __real__expand(_ptr, _blockSize);																\
			rmemRealloc(0, result, _blockSize, mallocGetOverhead(result,_blockSize), _ptr);									\
			return result;																									\
		}																													\
		RMEM_EXTERN_C_END
#else /* RMEM_NO_EXPAND */
	#define RMEM_GCC_ENTRY_WRAP_2
#endif /* RMEM_NO_EXPAND */

#ifndef RMEM_NO_MEMALIGN
	#define RMEM_GCC_ENTRY_WRAP_3																							\
		RMEM_EXTERN_C_BEGIN																									\
		void* __real_memalign(size_t _alignment, size_t _blockSize);														\
		void* __wrap_memalign(size_t _alignment, size_t _blockSize)															\
		{																													\
			void* result = __real_memalign(_alignment, _blockSize);															\
			rmemAllocAligned(0, result, _blockSize, mallocGetOverhead(result,_blockSize), _alignment);						\
			return result;																									\
		}																													\
		RMEM_EXTERN_C_END
#else /* RMEM_NO_MEMALIGN */
	#define RMEM_GCC_ENTRY_WRAP_3
#endif /* RMEM_NO_MEMALIGN */

#ifndef RMEM_NO_REALLOCALIGN
	#define RMEM_GCC_ENTRY_WRAP_4																							\
		RMEM_EXTERN_C_BEGIN																									\
		void* __real_reallocalign(void *_ptr, size_t _blockSize, size_t _alignment);										\
		void* __wrap_reallocalign(void *_ptr, size_t _blockSize, size_t _alignment)											\
		{																													\
			void* result = __real_reallocalign(_ptr, _blockSize, _alignment);												\
			rmemReallocAligned(0, result, _blockSize, mallocGetOverhead(result,_blockSize), _ptr, _alignment);				\
			return result;																									\
		}																													\
		RMEM_EXTERN_C_END
#else /* RMEM_NO_REALLOCALIGN */
	#define RMEM_GCC_ENTRY_WRAP_4
#endif /* RMEM_NO_REALLOCALIGN */

#endif /* __GNUC__ */

/*--------------------------------------------------------------------------
 * Program entry for link time binding to rmem/mtuner lib
 *------------------------------------------------------------------------*/
 
#if __GNUC__

	#if RMEM_PLATFORM_WINDOWS

		#define RMEM_ENTRY_CONSOLE			\
					RMEM_GCC_ENTRY_INIT		

		#define RMEM_ENTRY_WINDOWED			\
					RMEM_GCC_ENTRY_INIT		

	#else /* RMEM_PLATFORM_WINDOWS */

		#define RMEM_ENTRY_CONSOLE			\
					RMEM_GCC_ENTRY_INIT		\
					RMEM_GCC_ENTRY_WRAP_1	\
					RMEM_GCC_ENTRY_WRAP_2	\
					RMEM_GCC_ENTRY_WRAP_3	\
					RMEM_GCC_ENTRY_WRAP_4

		#define RMEM_ENTRY_WINDOWED			\
					RMEM_GCC_ENTRY_INIT		\
					RMEM_GCC_ENTRY_WRAP_1	\
					RMEM_GCC_ENTRY_WRAP_2	\
					RMEM_GCC_ENTRY_WRAP_3	\
					RMEM_GCC_ENTRY_WRAP_4

	#endif /* RMEM_PLATFORM_WINDOWS */

#elif RMEM_PLATFORM_WINDOWS

	#ifdef __cplusplus
	extern "C" {
	#endif /* __cplusplus */
		void mainCRTStartup();
		void WinMainCRTStartup();
	#ifdef __cplusplus
	}
	#endif /* __cplusplus */

	/* Console app */
	#define RMEM_ENTRY_CONSOLE			\
		void rmemEntry()				\
		{								\
			rmemHookAllocs(1);			\
			mainCRTStartup();			\
		}

	/* Windowed app */
	#define RMEM_ENTRY_WINDOWED 		\
		void rmemEntry()				\
		{								\
			rmemHookAllocs(1);			\
			WinMainCRTStartup();		\
		}

#elif RMEM_PLATFORM_XBOX360

	#ifdef __cplusplus
	extern "C" {
	#endif /* __cplusplus */
		void mainCRTStartup();
	#ifdef __cplusplus
	}
	#endif /* __cplusplus */

	/* Console app */
	#define RMEM_ENTRY_CONSOLE			\
		void rmemEntry()				\
		{								\
			rmemHookAllocs(1);			\
			mainCRTStartup();			\
			atexit(rmemUnhookAllocs);	\
		}

	/* Windowed app */
	#define RMEM_ENTRY_WINDOWED	RMEM_ENTRY_CONSOLE

#else
	#error "Unsupported compiler!"
	/*#define RMEM_ENTRY_CONSOLE*/
	/*#define RMEM_ENTRY_WINDOWED*/
#endif /* _MSC_VER */

#endif /* __RMEM_RMEM_ENTRY_H__ */
