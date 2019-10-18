//
// CS252: MyMalloc Project
//
// The current implementation gets memory from the OS
// every time memory is requested and never frees memory.
//
// You will implement the allocator as indicated in the handout.
//
// Also you will need to add the necessary locking mechanisms to
// support multi-threaded programs.
//

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/mman.h>
#include <pthread.h>
#include <errno.h>
#include <stdbool.h>
#include "MyMalloc.h"

#define ALLOCATED 1
#define NOT_ALLOCATED 0
#define ARENA_SIZE 2097152

pthread_mutex_t mutex;

static bool verbose = false;

extern void atExitHandlerInC()
{
  if (verbose)
    print();
}

static void * getMemoryFromOS(size_t size)
{
  // Use sbrk() to get memory from OS
  _heapSize += size;

  void *mem = sbrk(size);

  if(!_initialized){
      _memStart = mem;
  }

  return mem;
}


/*
 * @brief retrieves a new 2MB chunk of memory from the OS
 * and adds "dummy" boundary tags
 * @param size of the request
 * @return a FreeObject pointer to the beginning of the chunk
 */
static FreeObject * getNewChunk(size_t size)
{
  void *mem = getMemoryFromOS(size);

  // establish fence posts
  BoundaryTag *fencePostHead = (BoundaryTag *)mem;
  setAllocated(fencePostHead, ALLOCATED);
  setSize(fencePostHead, 0);

  char *temp = (char *)mem + size - sizeof(BoundaryTag);
  BoundaryTag *fencePostFoot = (BoundaryTag *)temp;
  setAllocated(fencePostFoot, ALLOCATED);
  setSize(fencePostFoot, 0);

  return (FreeObject *)((char *)mem + sizeof(BoundaryTag));
}

/**
 * @brief If no blocks have been allocated, get more memory and
 * set up the free list
 */
static void initialize()
{
  verbose = true;

  pthread_mutex_init(&mutex, NULL);

  // print statistics at exit
  atexit(atExitHandlerInC);

  FreeObject *firstChunk = getNewChunk(ARENA_SIZE);

  // initialize the list to point to the firstChunk
  _freeList = &_freeListSentinel;
  setSize(&firstChunk->boundary_tag, ARENA_SIZE - (2*sizeof(BoundaryTag))); // ~2MB
  firstChunk->boundary_tag._leftObjectSize = 0;
  setAllocated(&firstChunk->boundary_tag, NOT_ALLOCATED);

  // link list pointer hookups
  firstChunk->free_list_node._next = _freeList;
  firstChunk->free_list_node._prev = _freeList;
  _freeList->free_list_node._prev = firstChunk;
  _freeList->free_list_node._next = firstChunk;

  _initialized = 1;
}

/**
 * @brief TODO: PART 1
 * This function should perform allocation to the program appropriately,
 * giving pieces of memory that large enough to satisfy the request.
 * Currently, it just sbrk's memory every time.
 *
 * @param size size of the request
 *
 * @return pointer to the first usable byte in memory for the requesting
 * program
 */
static void * allocateObject(size_t size)
{
  // Make sure that allocator is initialized
  if (!_initialized)
    initialize();
  //pthread_mutex_unlock(&mutex);
  if(size==0) {
    pthread_mutex_unlock(&mutex);
	  return NULL;
  }
  //size_t roundedSize = ((size + 7) & ~7) + sizeof(BoundaryTag);
  size_t remainder = size % 8;
  size_t roundedSize = size;
  if(remainder != 0) {
  	roundedSize = size + 8 - remainder;
  }
  roundedSize += sizeof(BoundaryTag);
  //Minimum has to be 32 bytes
  if(roundedSize < sizeof(FreeObject)) {
  	roundedSize = sizeof(FreeObject);
  }
  if(roundedSize > (ARENA_SIZE - (3 * sizeof(BoundaryTag)))) {
  	errno = ENOMEM;
    pthread_mutex_unlock(&mutex);
    return NULL;
  }
  FreeObject * f = _freeList->free_list_node._next;

  while (f != _freeList) {
	  if(getSize(&f->boundary_tag) >= roundedSize) {
		    break;//Found chunk 
	  }
	  f = f->free_list_node._next;
  }
  if(f == _freeList) {
	//No memory chunk large enough
	//Call OS
 	//fprintf(stderr, "1\n");
	FreeObject * o = getNewChunk(ARENA_SIZE);
  o->free_list_node._next =  _freeList->free_list_node._next;
  o->free_list_node._next->free_list_node._prev = o;
  _freeList->free_list_node._next = o;
	o->free_list_node._prev = _freeList;
	setSize(&o->boundary_tag, ARENA_SIZE - 2*sizeof(BoundaryTag));
  //fprintf(stderr, "%d: main:\n", o->boundary_tag._objectSizeAndAlloc);

	setAllocated((&o->boundary_tag), NOT_ALLOCATED);
  o->boundary_tag._leftObjectSize = _freeList->boundary_tag._objectSizeAndAlloc;
  f = o;
  }
	size_t newSize = getSize(&f->boundary_tag) - roundedSize;

  if(sizeof(FreeObject) <= newSize) {
	  	newSize = getSize(&f->boundary_tag) - roundedSize;
	  	//if(newSize >= sizeof(FreeObject))
			//Chunk can be split
			//fprintf(stderr, "HERE\n");
			char * temp = (char *) f;
			temp += newSize;
			FreeObject * o = (FreeObject *)temp;
			//o->boundary_tag._leftObjectSize = newSize;
			setSize(&o->boundary_tag, roundedSize);
      //fprintf(stderr, "%d: main:\n", o->boundary_tag._objectSizeAndAlloc);
			o->boundary_tag._leftObjectSize = newSize;
			setSize(&f->boundary_tag, newSize);
			setAllocated((&o->boundary_tag), ALLOCATED);

			if(f != _freeList->free_list_node._next) {

				//fprintf(stderr, "HERE\n");
				f->free_list_node._next->free_list_node._prev = f->free_list_node._prev;
				f->free_list_node._prev->free_list_node._next = f->free_list_node._next;
			}
			/*temp = (char*)o;
			temp+=roundedSize;
			FreeObject * rightObject = (FreeObject*)temp;
			rightObject->boundary_tag._leftObjectSize = roundedSize;
			*/
			    pthread_mutex_unlock(&mutex);
       		return (char *) (&o->boundary_tag) + sizeof(BoundaryTag);
		}
	else {
			//Chunk can not be split so allocate all
			//fprintf(stderr,"3\n");
			f->free_list_node._next->free_list_node._prev = f->free_list_node._prev;
			f->free_list_node._prev->free_list_node._next = f->free_list_node._next;
			setSize(&f->boundary_tag, roundedSize);
      //fprintf(stderr, "%d: main:\n", f->boundary_tag._objectSizeAndAlloc);
			setAllocated((&f->boundary_tag), ALLOCATED);

			pthread_mutex_unlock(&mutex);
       		return (char *) (&f->boundary_tag) + sizeof(BoundaryTag);

			//break;
		}



	 /*if(getSize(&f->boundary_tag) >= roundedSize && getSize(&f->boundary_tag) < roundedSize + sizeof(FreeObject)) {
		//Remove the node
		//setAllocated((&f->boundary_tag), ALLOCATED);
		f->free_list_node._next->free_list_node._prev = f->free_list_node._prev;
		f->free_list_node._prev->free_list_node._next = f->free_list_node._next;
		break;
	  }*/
  pthread_mutex_unlock(&mutex);
  return (char *) (f) + sizeof(BoundaryTag);
}

/**
 * @brief TODO: PART 2
 * This funtion takes a pointer to memory returned by the program, and
 * appropriately reinserts it back into the free list.
 * You will have to manage all coalescing as needed
 *
 * @param ptr
 */
static void freeObject(void *ptr)
{
  char * p = (char *) ptr;
	p = p - sizeof(BoundaryTag);
	FreeObject * f = (FreeObject *) p;
  f->boundary_tag._objectSizeAndAlloc--;
	char * pt = p - f->boundary_tag._objectSizeAndAlloc;
	FreeObject * prev = (FreeObject *) pt;

	char * n = p + f->boundary_tag._objectSizeAndAlloc;
	FreeObject * next = (FreeObject *) n;

	setAllocated(&f->boundary_tag, ALLOCATED);


	size_t sizeAll  = f->boundary_tag._objectSizeAndAlloc + prev->boundary_tag._objectSizeAndAlloc + next->boundary_tag._objectSizeAndAlloc;
	size_t sizePrev = f->boundary_tag._objectSizeAndAlloc + prev->boundary_tag._objectSizeAndAlloc;
	size_t sizeNext = f->boundary_tag._objectSizeAndAlloc + next->boundary_tag._objectSizeAndAlloc;

	int allocPrev = isAllocated(&prev->boundary_tag);
  int allocNext = isAllocated(&next->boundary_tag);
  //fprintf(stderr, "%d: main:\n", f->boundary_tag._objectSizeAndAlloc);
  //fprintf(stderr, "%d: left:\n", prev->boundary_tag._objectSizeAndAlloc);
  //xfprintf(stderr, "%d: right:\n",next->boundary_tag._objectSizeAndAlloc);
	if(allocPrev && allocNext) {
    //fprintf(stderr, "NOT \n" );
    //fprintf(stderr, "NO\n");
	//Add both sides
    f->free_list_node._next = _freeList->free_list_node._next;
    _freeList->free_list_node._next = f;
    f->free_list_node._prev = _freeList;
    f->free_list_node._next->free_list_node._prev = f;
	setAllocated(&f->boundary_tag, 0);
	}
  	else if(!allocPrev && allocNext) {
    //fprintf(stderr, "LEFT\n");
	//Add right
    setSize(&prev->boundary_tag, sizePrev);
  	}
  else if(!allocNext && allocPrev) {
    //fprintf(stderr, "RIGHT\n");
	//Add left
    f->free_list_node._prev = next->free_list_node._prev;
    f->free_list_node._next = next->free_list_node._next;
    next->free_list_node._prev->free_list_node._next = f;
    next->free_list_node._next->free_list_node._prev = f;
    setSize(&f->boundary_tag,sizeNext);
  }
  else {
    //fprintf(stderr, "NOT\n");
	//Dont add
    prev->free_list_node._next = next->free_list_node._next;
		prev->free_list_node._next->free_list_node._prev = prev;
    setSize(&prev->boundary_tag, sizeAll);
  }
	return;
}

void print()
{
  printf("\n-------------------\n");

  printf("HeapSize:\t%zd bytes\n", _heapSize);
  printf("# mallocs:\t%d\n", _mallocCalls);
  printf("# reallocs:\t%d\n", _reallocCalls);
  printf("# callocs:\t%d\n", _callocCalls);
  printf("# frees:\t%d\n", _freeCalls);

  printf("\n-------------------\n");
}

void print_list()
{
    printf("FreeList: ");
    if (!_initialized)
        initialize();
    FreeObject *ptr = _freeList->free_list_node._next;
    while (ptr != _freeList) {
        long offset = (long)ptr - (long)_memStart;
        printf("[offset:%ld,size:%zd]", offset, getSize(&ptr->boundary_tag));
        ptr = ptr->free_list_node._next;
        if (ptr != NULL)
            printf("->");
    }
    printf("\n");
}

void increaseMallocCalls() { _mallocCalls++; }

void increaseReallocCalls() { _reallocCalls++; }

void increaseCallocCalls() { _callocCalls++; }

void increaseFreeCalls() { _freeCalls++; }

//
// C interface
//

extern void * malloc(size_t size)
{
  pthread_mutex_lock(&mutex);
  increaseMallocCalls();
  int remainder;
  remainder = size % 8;
  if(remainder != 0) {
  	size = size + 8 - remainder; //Rounding up requested size to the next 8 bytes
  }

  return allocateObject(size);
}

extern void free(void *ptr)
{
  pthread_mutex_lock(&mutex);
  increaseFreeCalls();

  if (ptr == 0) {
    // No object to free
    pthread_mutex_unlock(&mutex);
    return;
  }

  freeObject(ptr);
}

extern void * realloc(void *ptr, size_t size)
{
  pthread_mutex_lock(&mutex);
  increaseReallocCalls();

  // Allocate new object
  void *newptr = allocateObject(size);

  // Copy old object only if ptr != 0
  if (ptr != 0) {

    // copy only the minimum number of bytes
    FreeObject *o = (FreeObject *)((char *) ptr - sizeof(BoundaryTag));
    size_t sizeToCopy = getSize(&o->boundary_tag);
    if (sizeToCopy > size) {
      sizeToCopy = size;
    }

    memcpy(newptr, ptr, sizeToCopy);

    //Free old object
    freeObject(ptr);
  }

  return newptr;
}

extern void * calloc(size_t nelem, size_t elsize)
{
  pthread_mutex_lock(&mutex);
  increaseCallocCalls();

  // calloc allocates and initializes
  size_t size = nelem * elsize;

  void *ptr = allocateObject(size);

  if (ptr) {
    // No error
    // Initialize chunk with 0s
    memset(ptr, 0, size);
  }

  return ptr;
}
