#include "threads/synch.h"
#include "vm/frame.h"
#include "vm/page.h"
#include "threads/malloc.h"
#include "threads/loader.h"
#include "threads/palloc.h"
#include "userprog/process.h"

static struct frame *frames;
static size_t frames_size;

static struct lock frames_lock;

void
frame_init (void)
{
	void *kva;

	lock_init (&frames_lock);

	frames = malloc ((sizeof *frames) * init_ram_pages);
	if (frames == NULL)
		PANIC ("out of memory allocating page frames");

	while ((kva = palloc_get_page (PAL_USER)) != NULL)
	{
		struct frame *f = &frames[frames_size++];
		lock_init (&f->lock);
		f->kva = kva;
		f->page = NULL;
	}
}

//allocate locked frame
bool
allocate_frame (struct page *p)
{
	lock_acquire (&frames_lock);

	unsigned i;
	for (i = 0; i < frames_size; i++)
	{
		struct frame *f = &frames[i];
		if (!lock_try_acquire (&f->lock))
			continue;

		//the frame is available
		if (f->page == NULL)
		{
			f->page = p;
			p->frame = f;
			lock_release (&frames_lock);
			if (install_page (p->addr, f->kva, !p->read_only))
				return true;//swapping
			return true;
		}

		lock_release (&f->lock);
	}

	//There is no free frame, so before implementing swapping, fail the allocation or panic the kernel
	lock_release (&frames_lock);
	PANIC ("There is no free frame");	
}
