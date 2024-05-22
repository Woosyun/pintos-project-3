#include "userprog/pagedir.h"
#include "vm/page.h"
#include "threads/thread.h"
#include "threads/malloc.h"
#include "threads/vaddr.h"
#include "vm/frame.h"
#include "vm/page.h"
#include "userprog/process.h"
#include "filesys/file.h"
#include <hash.h>
#include "lib/kernel/hash.h"

#define STACK_MAX (1024 * 1024)
static struct page *addr_to_page (const void *addr);

struct page *find_page (void *vaddr);
bool claim_page (void *addr);
void stack_growth (void *addr);

void
deallocate_page (void *vaddr)
{
	struct page *p = find_page (vaddr);
	ASSERT (p != NULL);

	lock_acquire (&p->frame->lock);
	if (p->frame != NULL)
	{
		//TODO: page out?
		p->frame->page = NULL;
	}
	hash_delete (thread_current ()->pages, &p->page_elem);
	free (p);
}

struct page *
allocate_page (void *vaddr, bool writable)
{
	//if vaddr % 4kb != 0, then what?
	
	struct thread *t = thread_current ();
	struct page *p = malloc (sizeof *p);

	// how can I deal with situation which frame is not available?
	if (p == NULL)
		return NULL;

	//TODO: check whether addr can be devided by 4kb or not
	p->addr = vaddr;
	p->thread = t;
	p->frame = NULL;
	p->file = NULL;
	p->read_only = !writable;
	p->offset = 0;
	p->file_bytes = 0;

	p->sector = (block_sector_t) - 1;

	//TODO: insert this page to thread's page table
	if (hash_insert (t->pages, &p->page_elem) != NULL)
	{
		free (p);
		p = NULL;
	}
	return p;
}

unsigned
page_hash (const struct hash_elem *e, void *aux UNUSED)
{
	const struct page *p = hash_entry (e, struct page, page_elem);
	return ((uintptr_t) p->addr) >> PGBITS;
}
bool
page_cmp (const struct hash_elem *e1, const struct hash_elem *e2, void *aux UNUSED)
{
	const struct page *p1 = hash_entry (e1, struct page, page_elem);
	const struct page *p2 = hash_entry (e2, struct page, page_elem);
	return p1->addr < p2->addr;
}

static struct page *
addr_to_page (const void *addr)
{
	if (addr < PHYS_BASE)
	{
		struct page p;
		struct hash_elem *e;
		struct thread *t = thread_current ();

		p.addr = (void *)pg_round_down (addr);
		e = hash_find (t->pages, &p.page_elem);
		if (e != NULL)
			return hash_entry (e, struct page, page_elem);

		//handle stack growth
		if ((p.addr > PHYS_BASE - STACK_MAX) && ((void *)t->stack - 32 < addr))
			return allocate_page (p.addr, false);
		else
		{

		}
	}
	return NULL;
}

struct page *
find_page (void *va)
{
	struct page p;
	struct hash_elem *e;
	struct thread *t = thread_current ();
	
	p.addr = (void *)pg_round_down (va);
	e = hash_find (t->pages, &p.page_elem);
	if (e != NULL)
		return hash_entry (e, struct page, page_elem);
	return NULL;
}

bool
handle_page_fault (struct intr_frame *f UNUSED, void *fault_addr UNUSED, bool not_present UNUSED, bool write UNUSED, bool user UNUSED)
{
	struct thread *t = thread_current ();

	if (t->pages == NULL)
		return false;

	//if addr is in kernel space, return false
	if (!is_user_vaddr (fault_addr))
		return false;
	void *esp = is_user_vaddr (f->esp) ? f->esp : t->stack;

	int success = false;
	if (not_present)
	{
		if (!claim_page (fault_addr))
		{
			//if addr < stack, grow stack
			if (esp - 8 <= fault_addr && PHYS_BASE - 0x100000 <= fault_addr) 
			{
				stack_growth (t->stack - PGSIZE);
				success = true;
			}
		}
	}
	struct page *p = find_page (fault_addr);
	ASSERT (p != NULL);
	ASSERT (p->frame != NULL);
	lock_release (&p->frame->lock);
	return success;
}
bool
claim_page (void *addr)
{
	struct page *p = find_page (addr);
	if (p != NULL)
	{
		if (allocate_frame (p))
			return true;
	}
	return false;
}

void
stack_growth (void *addr)
{
	struct page *p = allocate_page (addr, false);
	if (p != NULL)
	{
		claim_page (addr);
		thread_current ()->stack -= PGSIZE;
	}
}

