#ifdef __MEMMANAGER__


#include <stdlib.h>
#include <stdio.h>
#include <memory.h>
#include <string.h>
#include <memory.h>
#include <stdarg.h> 

#ifdef _WIN32
#include <windows.h>
#endif

#include                  "MEM_ram.h"


//--------------------------------------------------------------------------------------------
// DEFINES
#define  MAX_MEMPOINTERS          80000
#define  SIZE_OF_FILENAME        256

	enum TERM_CMD{
		RESET = 0,
		BRIGHT = 1,
		DIM = 2,
		UNDERLINE = 3,
		BLINK = 4,
		REVERSE = 7,
		HIDDEN = 8

	};

	enum TERM_COLOR{

		BLACK = 0,
		RED = 1,
		GREEN = 2,
		YELLOW = 3,
		BLUE = 4,
		MAGENTA = 5,
		CYAN = 6,
		WHITE = 7
	};	



//--------------------------------------------------------------------------------------------
//  turn  off  macros...
#undef	new
#undef	delete
#undef    malloc
#undef    free


//--------------------------------------------------------------------------------------------
typedef  struct{
	void  *ptr;
	char  filename[SIZE_OF_FILENAME];
	char  function[30];
	int  line;
	void_function_void_pointer deallocator_function;
}tInfoAllocatedPointer;

//--------------------------------------------------------------------------------------------
static void         *Pointer[MAX_MEMPOINTERS];

static tInfoAllocatedPointer        PreAllocatedPointer[MAX_MEMPOINTERS];

//--------------------------------------------------------------------------------------------
// 1st optimize...

static int m_freePointer[MAX_MEMPOINTERS]={0};
static int m_numFreePointers=0;



//--------------------------------------------------------------------------------------------
// 2ns optimize...
typedef  struct{
	void *pointer; //this is the element to  be ordered.
	int index;
}tIndexPointer;

static tIndexPointer indexPointer[MAX_MEMPOINTERS]; // the same allocatedPointers it will have.
//--------------------------------------------------------------------------------------------

typedef  struct{
	int                          type_allocator;
	int                          offset_mempointer_table;
	char                        FileName[SIZE_OF_FILENAME];  //  base    -16-20
	int                          NumberFile;          //  base          -  16
	int	                       size;                      //  base          -  8
	int                          pre_crc;                //  base          -  4
	//void                *alloc;                  //  base          -  8
	//int                  post_crc;              //  crc  out    -  10
	//tMem_HeapInfo        IndexPointerTable;
}tMem_PreHeapInfo;
//--------------------------------------------------------------------------------------------
typedef  struct{
	int                          post_crc;
}tMem_PostHeapInfo;
//--------------------------------------------------------------------------------------------
//  Check  pointer...
//[pre_crc][pointer][post_crc]
//
#define  GET_SIZE_PTR(p)                         *((int  *)((char  *)p  -  sizeof(int) - sizeof(int)))
#define  GET_PREHEADER(p)                         ((tMem_PreHeapInfo    *)((char  *)p-sizeof(tMem_PreHeapInfo)))
#define  GET_POSTHEADER(p)                        ((tMem_PostHeapInfo  *)((char  *)p+(GET_SIZE_PTR(p))))
//--------------------------------------------------------------------------------------------
int          AllocatedBytes  =  0;
int          AllocatedPointers  =  0;
int          PreAllocatedPointers  =  0;
int			  enableRegisterMemoryLeaks = 1;
bool        JGT_Memory_Mannager_Active  =  false;
//--------------------------------------------------------------------------------------------
int  JGT_GetAllocatedBytes()
{
	return  AllocatedBytes;
}
//--------------------------------------------------------------------------------------------
void  MEM_ViewStatus(void);
void  MEM_FreeAllAllocatedPointers( );

void        MEM_setEnableRegisterMemoryLeaks(int enable) {
	enableRegisterMemoryLeaks = enable;
}

#define KEY_NOT_FOUND -1


int getIndexToInsert(tIndexPointer *arr, int size, void * key)
{
  int low = 0, high = size - 1;

  if (key > arr[high].pointer) return size;

  while (high > low) {
    int mid = (low + high) / 2;
    if (arr[mid].pointer > key) high = mid;
    else low = mid + 1;
  }

  return low;
}

int binary_search(tIndexPointer * A, void * key, int imin, int imax)
{
  // continue searching while [imin,imax] is not empty
  while (imax >= imin)
    {
      // calculate the midpoint for roughly equal partition
      int imid = (imin + imax ) >> 1;
      if(A[imid].pointer == key)
        // key found at index imid
        return imid;
      // determine which subarray to search
      else if (A[imid].pointer < key)
        // change min index to search upper subarray
        imin = imid + 1;
      else
        // change max index to search lower subarray
        imax = imid - 1;
    }
  // key was not found
  return KEY_NOT_FOUND;
}

//--------------------------------------------------------------------------------------------
void  STR_GetDirAndFileName(char  *name_dir,  char  *file_name,const  char  *argv)
{
        const  char  *to_down_ptr;
        int  i, lenght;

		if(!argv)
		{
			return;
		}

        if((lenght = (strlen(argv)-1)) > 0)//(to_down_ptr  =  strrchr(argv,'.'))  !=  NULL)
        {

			to_down_ptr = &argv[lenght-1];
            //  get  name  ...
            if((to_down_ptr-1)  >=  argv)
            {
				do
				{
						to_down_ptr--;

				}while(*(to_down_ptr-1)  !=  '\\'  &&  *(to_down_ptr-1)  !=  '/'  &&  to_down_ptr  >  argv);
			}

                if(file_name)
                {

                        sprintf(file_name,"%s",to_down_ptr);
                }

				//file_name[ini_ptr-to_down_ptr+1]=0;

                i=  0;

                if(name_dir)
		{
			to_down_ptr--; // To ignore the first char ''
			to_down_ptr--; // To ignore the slash '/'

			while(to_down_ptr  >=  argv  /*&&  i  <  (NAME_CAD  -1)*/)
			{
					name_dir[i]  =  argv[i];

					to_down_ptr--;
					i++;
			}

			name_dir[i]=0;

		}
    }
}
	

void CColorTerm_setColor(FILE *std_type, int attr, int fg, int bg)
{

	char command[13];

	/* Command is the control command to the terminal */
	sprintf(command, "%c[%d;%d;%dm", 0x1B, attr, fg + 30, bg + 40);
	fprintf(std_type, "%s", command);
}	

#ifndef  __GNUC__
#pragma  managed(push,  off)
#endif
void  print_error_cr(const  char  *string_text, ...) {

	FILE *std_type = stderr;
	char  text[4096] = { 0 };
	va_list  ap;
	
	va_start(ap,  string_text);
	vsprintf(text,  string_text,  ap);
	va_end(ap);	
	
	//  Results  Are  Stored  In  Text
#ifdef _WIN32
  SetConsoleTextAttribute(GetStdHandle(STD_ERROR_HANDLE), FOREGROUND_RED   | FOREGROUND_INTENSITY);
#else // ansi color
	CColorTerm_setColor(std_type, TERM_CMD::BRIGHT, TERM_COLOR::RED, TERM_COLOR::BLACK);
#endif
	fprintf(std_type, "%s", text);
#ifdef _WIN32
	SetConsoleTextAttribute(GetStdHandle(STD_ERROR_HANDLE), FOREGROUND_RED   | FOREGROUND_GREEN | FOREGROUND_BLUE);
#else // ansi color
	CColorTerm_setColor(std_type, TERM_CMD::BRIGHT, TERM_COLOR::WHITE, TERM_COLOR::BLACK);
#endif
	
	
	fprintf(std_type, "\n");
	fflush(std_type);
}

#ifndef  __GNUC__
#pragma  managed(pop)
#endif

void  MEM_Init()
{

	if(!JGT_Memory_Mannager_Active)
	{
		
		
		AllocatedBytes  =  0;
		memset(Pointer,0,sizeof(Pointer));
		AllocatedBytes  =  0;
		AllocatedPointers  =  0;
		PreAllocatedPointers = 0;
		enableRegisterMemoryLeaks = 1;
		JGT_Memory_Mannager_Active  =  true;
		m_numFreePointers = MAX_MEMPOINTERS-1;
		memset(&indexPointer,-1,sizeof(indexPointer));


		for(int i = 0; i < m_numFreePointers; i++){
			m_freePointer[i]=MAX_MEMPOINTERS-1-i;
		}


		fprintf(stdout,"*******************************\n");
		fprintf(stdout,"Memory mannagement initialized!\n");
		fprintf(stdout,"*******************************\n");
		
	}
}
//--------------------------------------------------------------------------------------------
int  MEM_GetFreeCellTableMemPreAllocatedPointer()
{
	int          i  =  0;
	
	if(PreAllocatedPointers  <  MAX_MEMPOINTERS)
	{
		while(i  <  MAX_MEMPOINTERS )
		{
			if(PreAllocatedPointer[i].ptr  ==  NULL)
				return i;//found  =  true;

			i++;
		}
	}
	else
	{

		print_error_cr("Table  mem  pointers  at  full  (max  =  %i)",MAX_MEMPOINTERS);
		exit(EXIT_FAILURE);
	}
	return -1;

}
//--------------------------------------------------------------------------------------------
int  MEM_GetFreeCellTableMem()
{

#ifdef __USE_FAST_DICOTOMIC__
	if(m_numFreePointers > 0){
		return m_freePointer[m_numFreePointers];
	}
	return -1; // no memory free...
#else

	int          i  =  0;
	bool        found  =  false;
	int          index  =  -1;

	if(AllocatedPointers  <  MAX_MEMPOINTERS)
	{
		while(i  <  MAX_MEMPOINTERS  &&  !found)
		{
			if(Pointer[i]  ==  NULL)
			{
				found  =  true;
			}
			else
			{
				i++;
			}
		}
		if(!found)
		{
			print_error_cr("ERROR:  Cannot  found  free  cell");
		}
		else
		{
			index  =  i;
		}
	}
	else
	{
		print_error_cr("Table  mem  pointers  at  full  (max  =  %i)",MAX_MEMPOINTERS);

		exit(EXIT_FAILURE);
	}

	return  index;
#endif
}

//--------------------------------------------------------------------------------------------
bool  MEM_PointerRegistered(void *pointer)
{
	int          i  =  0;

#ifdef __USE_FAST_DICOTOMIC__
	int pos = binary_search(indexPointer,pointer, 0, AllocatedPointers);

	if(pos >=0){
		if(indexPointer[pos].pointer == pointer)
			return true;
	}
#else
	// brute force ?!?!?!
	while(i  <  MAX_MEMPOINTERS )
	{
		if(Pointer[i]  ==  pointer)

			return true;
		i++;
	}
#endif

	return  false;
}
//--------------------------------------------------------------------------------------------
void        JGT_AddReservedPointer(void  *pointer,  void_function_void_pointer deallocator_function,const  char  *filename,  const char *function,int  line)
{
	int  i;
	char  buffer_dir[100];

	if(PreAllocatedPointers<MAX_MEMPOINTERS)
	{
		if((i  =  MEM_GetFreeCellTableMemPreAllocatedPointer())  !=  -1)
		{
			STR_GetDirAndFileName(buffer_dir,  PreAllocatedPointer[i].filename,  filename);
			STR_GetDirAndFileName(buffer_dir,  PreAllocatedPointer[i].function,  function);
			PreAllocatedPointer[i].ptr  =  pointer;
			PreAllocatedPointer[i].line  =  line;
			PreAllocatedPointer[i].deallocator_function = deallocator_function;

			PreAllocatedPointers++;
			return;
		}
	}

	print_error_cr("Cannot  allocate  more  pointers!!!");
}
//--------------------------------------------------------------------------------------------
int JGT_PreAllocatedPointerExist(void *pointer, const char *module, const char *function, int line)
{
	int  i=0;

	if(pointer == NULL) 
		return 0;

	while(i  <  MAX_MEMPOINTERS)
	{
		if(pointer  ==  PreAllocatedPointer[i].ptr)
			return true;
		i++;
	}

	return 0;
}
//-------------------------------------------------------------------------------------------------------------------------
int        JGT_DeleteReservedPointer(void  *pointer, const char *module, const char *function, int line)
{
	int  i=0;
	bool  found  =  false;
	
	if(pointer == NULL) 
	{
		//CLog::print(0,0,CLog::LOG_ERROR	,"(%s:%s:%i):NULL pointer!",module,function,line);
		return 0;
	}

	while(i  <  MAX_MEMPOINTERS  &&  !found)
	{
		if(pointer  ==  PreAllocatedPointer[i].ptr)
		{
			found  =  true;

		}
		else      i++;
	}

	if(found)
	{

		if(PreAllocatedPointers <= 0)
		{
			print_error_cr("pointer can't be removed because PreAllocatedPointers <= 0");
			exit(EXIT_FAILURE);
		}

		PreAllocatedPointer[i].ptr  =  NULL;
		PreAllocatedPointers--;
		return  1;
	}

	print_error_cr("(%s:%s:%i):Pointer not found!",module,function,line);
	return  0;
}
//--------------------------------------------------------------------------------------------
void  *JGT_malloc(size_t  size,  const  char  *filename,  int  line)
{
	tMem_PreHeapInfo  *heap_allocat  =  NULL;
	void  *pointer  =  NULL;
	int  random_number,index;


	if(!JGT_Memory_Mannager_Active)  MEM_Init();  //  auto_inicialize  return  malloc(size);

	heap_allocat  =  (tMem_PreHeapInfo  *)malloc(sizeof(tMem_PreHeapInfo)  +  sizeof(tMem_PostHeapInfo)  +  size);
	if(heap_allocat
			&&
			((index  =  MEM_GetFreeCellTableMem())  !=  -1))
	{
		//print_error_cr( "\n\n Allocating memory .... %s",filename);

		heap_allocat->size  =  size;
		heap_allocat->offset_mempointer_table  =  index;

		STR_GetDirAndFileName(NULL,  heap_allocat->FileName,  filename);
		heap_allocat->NumberFile  =  line;

		random_number  =  (unsigned(rand()%0xFFFF)  <<  16)  |  (rand()%0xFFFF);

		heap_allocat->pre_crc  =  random_number;
		heap_allocat->size        =  size;
		heap_allocat->type_allocator  =  MALLOC_ALLOCATOR;

		Pointer[index] 	    = heap_allocat;

		*((int  *)((char  *)heap_allocat+size+sizeof(tMem_PreHeapInfo)))  =  random_number;

		AllocatedBytes  +=  size;
		AllocatedPointers++;

		//			printf("\nAllocated new pointer %i!",index);
		//  arrr

		pointer  =  ((char  *)heap_allocat+sizeof(tMem_PreHeapInfo));
#ifdef __USE_FAST_DICOTOMIC__
		//-----------------------------------------------------------------
		// INSERT ORDERED ELEMENT...
		int pos = getIndexToInsert(indexPointer,AllocatedPointers,pointer);
		
		for (int i = AllocatedPointers-1; i >= pos; --i)
			indexPointer[i+1] = indexPointer[i];
		indexPointer[pos].pointer = heap_allocat;
		indexPointer[pos].index = index;

		//-----------------------------------------------------------------
#endif

		m_numFreePointers--;
		
		//  memset  pointer
		memset(pointer,0,size);

		return    (pointer);  //  return  base  pointer

	}

	print_error_cr("Table full or not enought memory");
	exit(EXIT_FAILURE);

	//return  NULL;

}
//--------------------------------------------------------------------------------------------
bool  JGT_Free(void  *pointer,  const  char  *filename,  int  line  )//,  char  *name,  int  file)
{



	tMem_PreHeapInfo    *preheap_allocat    =  NULL;
	tMem_PostHeapInfo  *postheap_allocat  =  NULL;
	void  *base_pointer;
	char  name[100],  dir[100];

	STR_GetDirAndFileName(dir,name,filename);

	if(pointer)
	{
		if(!enableRegisterMemoryLeaks) {
			free(pointer);
			return true;

		} else {
			//  Getheaders...
			base_pointer  =  preheap_allocat    =  GET_PREHEADER(pointer);
			postheap_allocat  =  GET_POSTHEADER(pointer);



			//  Check  headers...
			if(preheap_allocat->pre_crc  ==  postheap_allocat->post_crc)  //  crc  ok  :)
			{

				if(preheap_allocat->offset_mempointer_table  >=  0  &&  preheap_allocat->offset_mempointer_table  <=  MAX_MEMPOINTERS)
				{
					//  deallocate  pointer  will  be  ok  :)


					//  Mark  freed...
					Pointer[preheap_allocat->offset_mempointer_table]  =  NULL;

					if(m_numFreePointers<(MAX_MEMPOINTERS-1)){
						m_numFreePointers++;
						m_freePointer[m_numFreePointers] = preheap_allocat->offset_mempointer_table;
					}
					else print_error_cr("max pointer?");

#ifdef __USE_FAST_DICOTOMIC__
					//-----------------------------------------------------------------
					// DELETE ORDERED ELEMENT...
					int pos = binary_search(indexPointer,base_pointer, 0, AllocatedPointers);
					if(pos >= 0){
						memcpy(&indexPointer[pos],&indexPointer[pos+1],(AllocatedPointers-pos)*sizeof(tIndexPointer));
					}
#endif
					//----------------------------------------------

					AllocatedBytes-=preheap_allocat->size;
					AllocatedPointers--;

					//CLog::print(0,0,CLog::LOG_ERROR	,"deallocating memory from %s line %i Alloc:%i",preheap_allocat->FileName,preheap_allocat->NumberFile, AllocatedPointers);

					free(base_pointer);

					// clean preheaps ..


					return  true;
				}
				else
				{
					print_error_cr("MEM  ERROR:  bad  index  mem  table  in  file  \"%s\"  at  line  %i.",filename,line);

				}

			}
			else
			{
				print_error_cr("MEM  ERROR:  Bad  crc  pointer  \"%s\"  at  line  %i.",filename,line);

			}
		}

	}
	else
	{
		 print_error_cr("ERROR:  passed  pointer  is  null  \"%s\"  at  row  %i.",filename,line);

	}

	return  false;

}


void *JGT_realloc(void *ptr, size_t size,  const  char  *filename,  int  line) {
  if (!ptr) {
    // NULL ptr. realloc should act like malloc.
    return JGT_malloc(size, filename, line);
  }

  tMem_PreHeapInfo  *pre_head  =  GET_PREHEADER(ptr);


  if ((size_t)pre_head->size >= size) {
    // We have enough space. Could free some once we implement split.
    return ptr;
  }

  // Need to really realloc. Malloc new space and free old space.
  // Then copy old data to new space.
  void *new_ptr;
  new_ptr = JGT_malloc(size, filename, line);
  if (!new_ptr) {
    return NULL; // TODO: set errno on failure.
  }
  memcpy(new_ptr, ptr, pre_head->size);
  JGT_Free(ptr, filename, line);
  return new_ptr;
}

//  ---------------------------------------------------------------------------------------------------------------------------------
//  Used  by  the  macros
//  ---------------------------------------------------------------------------------------------------------------------------------
#define MAX_STACKED_FILES		512

char	        m_stackedSourceFile[MAX_STACKED_FILES][100]               =  {"??"};
int	        	m_stackedSourceLine[MAX_STACKED_FILES]               =  {0};

int 			m_numStackedFiles=0;


//--------------------------------------------------------------------------------------------
void  MEM_ShowDeallocatorMethod(int  allocator, const char *sourceFile, int sourceLine)
{
	switch(allocator)
	{
	case  MALLOC_ALLOCATOR:
		print_error_cr("ERROR:  Pointer  at  filename  \"%s\"  line  %i  must  freed  with  function  free().",sourceFile,  sourceLine);
		break;
	case  NEW_ALLOCATOR:
		print_error_cr("ERROR:  Pointer  at  filename  \"%s\"  line  %i  must  freed  with  operator  delete.",sourceFile,  sourceLine);
		break;
	case  NEW_WITH_BRACETS_ALLOCATOR:
		print_error_cr("ERROR:  Pointer  at  filename  \"%s\"  line  %i  must  freed  with  operator  delete[].",sourceFile,  sourceLine);
		break;
	}
}

//----------------------------------------------------------------------------------------
void  JGT_PreCheckFree(void  *p,  const  char  *filename,  int  line  )//,  char  *name,  int  file)
{
	if(p)
	{
		tMem_PreHeapInfo  *pre_head  =  GET_PREHEADER(p);

		if(pre_head->type_allocator  ==  MALLOC_ALLOCATOR)
		{
			JGT_Free(p,  filename,  line);
		}
		else  //  error
		{
			MEM_ShowDeallocatorMethod(pre_head->type_allocator, filename, line);
		}
	}
	else
	{
		//   CLog::print(0,0,CLog::LOG_ERROR	,"ERROR:  NULL  pointer  to  deallocate  at  filename  \"%s\"  line  %i.",filename,  line);
	}
}

//----------------------------------------------------------------------------------------
void MEM_ListAllocatedPointers()
{


	tMem_PreHeapInfo    *preheap_allocat;
	int  i;
	for(i  =  0;  i  <  MAX_MEMPOINTERS;  i++)
	{
		if((preheap_allocat  =  (tMem_PreHeapInfo    *)Pointer[i]))
		{
			printf("\n%s:%i:%p. (Allocated pointer)",preheap_allocat->FileName,  preheap_allocat->NumberFile,Pointer[preheap_allocat->offset_mempointer_table]);
		}

		if(PreAllocatedPointer[i].ptr  !=  NULL)
		{
			//CLog::print(0,0,CLog::LOG_ERROR	,"Allocated  pointer  at  filename  \"%s\"  in  row  %i  NOT  DEALLOCATED.",PreAllocatedPointer[i].filename,  PreAllocatedPointer[i].line);
			printf("\n%s:%s:%i: PreAllocated pointer.",PreAllocatedPointer[i].filename, PreAllocatedPointer[i].function, PreAllocatedPointer[i].line);
		}
	}
	printf("\n\n");
	//-----
}



//----------------------------------------------------------------------------------------
void  MEM_FreeAllAllocatedPointers( )//,  char  *name,  int  file)
{

	void *p;

	for(int i = 0; i < MAX_MEMPOINTERS; i++)
	{
		p = Pointer[i];
		if(p)
		{
			//tMem_PreHeapInfo  *pre_head  =  (tMem_PreHeapInfo *)p;
			JGT_Free((char *)p+sizeof(tMem_PreHeapInfo),"MEM_FreeAllAllocatedPointers()",0);
			Pointer[i] = NULL;
		}

		p = PreAllocatedPointer[i].ptr;

		if(p)
		{
			PreAllocatedPointer[i].deallocator_function("MEM_FreeAllAllocatedPointers()",0,0,p);
		}
	}
}
//--------------------------------------------------------------------------------------------
#ifdef  __cplusplus

//  ---------------------------------------------------------------------------------------------------------------------------------
bool	m_setOwner(const  char  *file,  const  unsigned  int  line,  const  char  *func)
{
	char  name_file[1024];
	//        char  name_file2[100];

	if(m_numStackedFiles < MAX_STACKED_FILES)
	{

		STR_GetDirAndFileName(NULL,  name_file,file);

		strcpy(m_stackedSourceFile[m_numStackedFiles], name_file);
		m_stackedSourceLine[m_numStackedFiles] = line;
		m_numStackedFiles++;
	}
	else
	{
		print_error_cr("reached max stacked files!");
		return false;
	}

	return true;
}
//  ---------------------------------------------------------------------------------------------------------------------------------
void*  operator  new(size_t  size)//,  const  char  *file,  const  unsigned  int  line,  const  char  *func)
{
	const char *sourceFile = "??";
	int sourceLine = 0;

	if(m_numStackedFiles > 0)
	{
		sourceFile = m_stackedSourceFile[m_numStackedFiles-1];
		sourceLine = m_stackedSourceLine[m_numStackedFiles-1];
	}

	void  *p = NULL;
	if(!enableRegisterMemoryLeaks) {

		p = malloc(size);

	} else {

			p  =  JGT_malloc(size,
									sourceFile,
									sourceLine);

			if(p)
			{
				tMem_PreHeapInfo  *pre_head  =  GET_PREHEADER(p);

				pre_head->type_allocator  =  NEW_ALLOCATOR;
			}
	}

	// set as not registered pointer ...
	if(m_numStackedFiles > 0)
		m_numStackedFiles--;

	return  p;
}
//--------------------------------------------------------------------------------------------
void*  operator  new[](size_t  size)//,  const  char  *file,  const  unsigned  int  line,  const  char  *func)
{
	const char *sourceFile = "??";
	int sourceLine = 0;

	if(m_numStackedFiles > 0)
	{
		sourceFile = m_stackedSourceFile[m_numStackedFiles-1];
		sourceLine = m_stackedSourceLine[m_numStackedFiles-1];
	}

	void *p = NULL;

	if(!enableRegisterMemoryLeaks) {

		p = malloc(size);

	} else {

		p  =  JGT_malloc(size,sourceFile, sourceLine);

		tMem_PreHeapInfo  *pre_head  =  GET_PREHEADER(p);

		pre_head->type_allocator  =  NEW_WITH_BRACETS_ALLOCATOR;
	}

	// set source/line as not registered pointer
	// set as not registered pointer ...
	if(m_numStackedFiles > 0)
		m_numStackedFiles--;

	return  p;

}
//--------------------------------------------------------------------------------------------
void  operator  delete(void  *p) throw()//,  const  char  *file,  const  unsigned  int  line,  const  char  *func)
{
	//CLog::print(0,0,CLog::LOG_ERROR	,"\ndeallocating memeory form %s %i\n\n",sourceFile,sourceLine);
	const char *sourceFile = "??";
	int sourceLine = 0;

	if(m_numStackedFiles > 0)
	{
		sourceFile = m_stackedSourceFile[m_numStackedFiles-1];
		sourceLine = m_stackedSourceLine[m_numStackedFiles-1];
	}

	if(p)
	{
		if(!enableRegisterMemoryLeaks) {

			free(p);

		} else {

			if(!MEM_PointerRegistered((char *)p-sizeof(tMem_PreHeapInfo)))
			{
				print_error_cr("(%s:%i): Pointer NOT REGISTERED OR POSSIBLE MEMORY CORRUPTION?!?!\n",sourceFile,  sourceLine);
			}
			else
			{

				tMem_PreHeapInfo   *pre_head   =  GET_PREHEADER(p);
				tMem_PostHeapInfo  *post_head  =  GET_POSTHEADER(p);

				if((int)p==0x805c15c){// || (int)pre_head==0x8057a38){
					int h=0;
					h++;
				}

				if(pre_head->pre_crc  ==  post_head->post_crc)
				{
					if(pre_head->type_allocator  ==  NEW_ALLOCATOR)
					{
						JGT_Free(p,  sourceFile,  sourceLine);
					}
					else  //  error
					{
						MEM_ShowDeallocatorMethod(pre_head->type_allocator,  sourceFile,  sourceLine);
					}
				}
				else
				{
					print_error_cr("(%s:%i): CRC  error!",sourceFile,  sourceLine);
				}
			}
		}
	}
	else
	{
		// CLog::print(0,0,CLog::LOG_ERROR	,"ERROR:  NULL  pointer  to  deallocate  at  filename  \"%s\"  line  %i.",sourceFile,  sourceLine);
	}

	// set as not registered pointer ...
	if(m_numStackedFiles > 0)
		m_numStackedFiles--;

}
//--------------------------------------------------------------------------------------------


void  operator  delete[](void  *p) throw()//,  const  char  *file,  const  unsigned  int  line,  const  char  *func)
{
	const char *sourceFile = "??";
	int sourceLine = 0;

	if(m_numStackedFiles > 0)
	{
		sourceFile = m_stackedSourceFile[m_numStackedFiles-1];
		sourceLine = m_stackedSourceLine[m_numStackedFiles-1];
	}


	if(p)
	{

		if(!enableRegisterMemoryLeaks) {

			free(p);

		} else {

			if(!MEM_PointerRegistered((char *)p-sizeof(tMem_PreHeapInfo)))
			{
				print_error_cr("(%s:%i): Pointer NOT REGISTERED WITH MALLOC OR NEW!\n",sourceFile,  sourceLine);
			}
			else
			{

				tMem_PreHeapInfo  *pre_head  =  GET_PREHEADER(p);

				if(pre_head->type_allocator  ==  NEW_WITH_BRACETS_ALLOCATOR)
				{
					JGT_Free(p,  sourceFile,  sourceLine);
				}
				else  //  error
				{
					MEM_ShowDeallocatorMethod(pre_head->type_allocator,  sourceFile,  sourceLine);
				}
			}
		}
	}
	else
	{
		// CLog::print(0,0,CLog::LOG_ERROR	,"ERROR:  NULL  pointer  to  deallocate  at  filename  \"%s\"  line  %i.\n",sourceFile,  sourceLine);
	}

	if(m_numStackedFiles > 0)
		m_numStackedFiles--;
}
#endif  


//--------------------------------------------------------------------------------------------
void        MEM_PrintAllocatedPointers(const char *_file, int line){
	tMem_PreHeapInfo    *preheap_allocat;
	int  i;

	for(i  =  0;  i  <  MAX_MEMPOINTERS;  i++)
	{
		if((preheap_allocat  =  (tMem_PreHeapInfo    *)Pointer[i]))
		{
			if(preheap_allocat->NumberFile>0 && (strcmp("??",preheap_allocat->FileName)!=0)&&!strcmp(preheap_allocat->FileName,_file) && preheap_allocat->NumberFile==line) // leak from others libs
			{
				print_error_cr("%s:%i:Allocated  pointer  NOT  DEALLOCATED (%p).",preheap_allocat->FileName,  preheap_allocat->NumberFile,Pointer[preheap_allocat->offset_mempointer_table]);
			}
		}
	}
}

void  MEM_ViewStatus(void)
{
	tMem_PreHeapInfo    *preheap_allocat;
	int  i;

	unsigned int sizeFromOtherLibs = 0;
	unsigned int allocatedFromOtherLibs = 0;

	for(i  =  0;  i  <  MAX_MEMPOINTERS;  i++)
	{
		if((preheap_allocat  =  (tMem_PreHeapInfo    *)Pointer[i]))
		{
			if(preheap_allocat->NumberFile>0 && (strcmp("??",preheap_allocat->FileName)!=0)) // leak from others libs
			{
				print_error_cr("%s:%i:Allocated  pointer  NOT  DEALLOCATED (%p).",preheap_allocat->FileName,  preheap_allocat->NumberFile,Pointer[preheap_allocat->offset_mempointer_table]);
			}
			else
			{
				sizeFromOtherLibs += preheap_allocat->size;
				allocatedFromOtherLibs++;
			}
		}

		if(PreAllocatedPointer[i].ptr  !=  NULL)
		{
			//CLog::print(0,0,CLog::LOG_ERROR	,"Allocated  pointer  at  filename  \"%s\"  in  row  %i  NOT  DEALLOCATED.",PreAllocatedPointer[i].filename,  PreAllocatedPointer[i].line);
			print_error_cr("%s:%s:%i: PreAllocated pointer NOT  DEALLOCATED.",PreAllocatedPointer[i].filename, PreAllocatedPointer[i].function, PreAllocatedPointer[i].line);
		}
	}

	//-----

	if(((AllocatedPointers-allocatedFromOtherLibs)>0)  ||  (AllocatedBytes-sizeFromOtherLibs)>0  || (PreAllocatedPointers>0))
	{
		print_error_cr("bytes  to  deallocate  =  %i  bytes",JGT_GetAllocatedBytes());
		print_error_cr("Mempointers  to  deallocate  =  %i",AllocatedPointers  +  PreAllocatedPointers);
	}
	else
	{
		fprintf(stdout,"MEMRAM:ok.\n");
	}
}


#endif

