#include "threads/synch.h"
#include "vm/frame.h"
#include "vm/page.h"
#include "threads/malloc.h"
#include "threads/loader.h"
#include "threads/palloc.h"
#include "userprog/process.h"
#include "userprog/pagedir.h"
#include <stdio.h>

static struct frame *frames;
static struct list frames_li;
static size_t frames_size;

static struct lock frames_lock;

struct frame *evict_frame (void);

void
frame_init (void)
{
	void *kva;

	lock_init (&frames_lock);
	list_init (&frames_li);

	frames = malloc ((sizeof *frames) * init_ram_pages);

	if (frames == NULL)
		PANIC ("out of memory allocating page frames");

	lock_acquire (&frames_lock);
	while ((kva = palloc_get_page (PAL_USER)) != NULL)
	{
		struct frame *f = &frames[frames_size++];
		lock_init (&f->lock);
		f->kva = kva;
		f->page = NULL;
	}
	lock_release (&frames_lock);
}
bool 
deallocate_frame (struct frame *f)
{
	lock_acquire (&frames_lock);
	//pagedir_clear_page (f->page->thread->pagedir, f->page->addr);
	f->page = NULL;	
	//lock_release (&f->lock);
	lock_release (&frames_lock);

	return true;
}

struct frame *
evict_frame (void)
{
	struct list_elem *e = NULL;
	if (!list_empty (&frames_li))
  	e	= list_pop_front (&frames_li);
	struct frame *f = list_entry (e, struct frame, frame_elem);
	//if page dirty, write to disk
	//don't have to release lock? lock would be released by end of exec or syscall?
	if (!deallocate_frame (f))
		return NULL;
	return f;
}

//allocate locked frame
bool
allocate_frame (struct page *p)
{
	bool success = true;
	lock_acquire (&frames_lock);

	//find free frame
	unsigned i;
	for (i = 0; i < frames_size; i++)
	{
		struct frame *f = &frames[i];
		if (f->page != NULL)
			continue;
	//	if (f->page != NULL)
		//A frame is available
		f->page = p;
		p->frame = f;
		//lock_acquire (&f->lock);
		//pagedir_set_page (p->thread->pagedir, p->addr, f->kva, !p->read_only);
		list_push_back (&frames_li, &f->frame_elem);
		lock_release (&frames_lock);
		//if (install_page (p->addr, f->kva, !p->read_only))
			//return true;//swapping
		return true;
	}
	//PANIC("no frame available");
	goto FINISH;

	//TODO: swap
	struct frame *f = evict_frame ();
	if (f == NULL)
	{
		success = false;
		goto FINISH;
	}
	f->page = p;
	p->frame = f;
	list_push_back (&frames_li, &f->frame_elem);
	success = true;

FINISH:
	lock_release (&frames_lock);
	return success;
}
