#include "vm/mmap.h"
#include "threads/thread.h"
#include "lib/kernel/list.h"

struct mmap *
lookup_mmap (int id)
{
	struct thread *t = thread_current ();
	struct list_elem *e;

	for (e = list_begin (&t->mmap_li); e != list_end (&t->mmap_li); e = list_next (e))
	{
		struct mmap *m = list_entry (e, struct mmap, mmap_elem);
		if (m->id == id)
			return m;
	}

	thread_exit ();
	//return NULL;
}
