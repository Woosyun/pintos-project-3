#ifndef _FRAMEH_
#define _FRAMEH_

#include "threads/synch.h"

struct frame {
	struct lock lock;
	void *kva; // kernel virtual base address
	struct page *page; //page that refers this frame
	struct list_elem frame_elem;
};

void frame_init (void);
bool deallocate_frame (struct frame *f);
bool allocate_frame (struct page *p);


#endif
