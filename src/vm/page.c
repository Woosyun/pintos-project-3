#include<stdio.h>
#include "userprog/pagedir.h"
#include "vm/page.h"
#include "threads/thread.h"
#include "threads/malloc.h"
#include "threads/vaddr.h"
#include "vm/frame.h"
#include "vm/page.h"
#include "userprog/process.h"
#include "filesys/file.h"
#include "lib/kernel/hash.h"
#include "lib/string.h"

#define STACK_MAX (1024 * 1024)

struct page *find_page (struct hash *h, void *vaddr);
bool claim_page (void *addr);
bool grow_stack (struct hash *h, void *pagedir, void *addr);

unsigned page_hash (const struct hash_elem *e, void *aux);
bool page_cmp (const struct hash_elem *e1, const struct hash_elem *e2, void *aux);
void destroy_page (struct hash_elem *e, void *aux);
bool load_page (struct hash *h, uint32_t *pagedir, void *upage);

struct hash *
pages_init (void)
{
	struct hash *pages = (struct hash *)malloc (sizeof(struct hash));
	if (pages == NULL)
		return false;

	if(!hash_init (pages, page_hash, page_cmp, NULL))
		return NULL;
	return pages;
}
void
destroy_pages (struct hash *h)
{
	hash_destroy (h, destroy_page);
	free (h);
	h = NULL;
}

void
destroy_page (struct hash_elem *e, void *aux UNUSED)
{
	struct page *p = hash_entry (e, struct page, page_elem);
	if (p->addr != NULL)
	{
		p->frame->page = NULL;
	}
	free (p);
}	

void
deallocate_page (struct hash *h, uint32_t *pagedir, void *page, struct file *f, off_t offset, size_t bytes)
{
	struct page *p = find_page (h, page);
	ASSERT (p != NULL);

	//if page is written, write it to disk
	bool is_dirty = pagedir_is_dirty (pagedir, p->frame->kva);
	if (is_dirty)
		file_write_at (f, p->addr, bytes, offset);
	
	deallocate_frame (p->frame);
	pagedir_clear_page (pagedir, p->addr);
	hash_delete (h, &p->page_elem);
	free (p);
}

struct page *
allocate_page (void *vaddr_, struct file *f, off_t offset, uint32_t read_bytes, uint32_t zero_bytes, bool writable)
{
	//if vaddr % 4kb != 0, then what?
	struct thread *t = thread_current ();
	struct page *p = (struct page *)malloc (sizeof(struct page));
	void *vaddr = (void *)pg_round_down (vaddr_);

	// how can I deal with situation which frame is not available?
	if (p == NULL)
		return NULL;

	//TODO: check whether addr can be devided by 4kb or not
	p->addr = vaddr;
	p->thread = t;
	p->frame = NULL;
	p->file = f;
	p->offset = offset;
	p->read_bytes = read_bytes;
	p->zero_bytes = zero_bytes;
	p->read_only = !writable;
	p->dirty = false;

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
	return hash_int ((int)p->addr);
}
bool
page_cmp (const struct hash_elem *e1, const struct hash_elem *e2, void *aux UNUSED)
{
	struct page *p1 = hash_entry (e1, struct page, page_elem);
  struct page *p2 = hash_entry (e2, struct page, page_elem);
	return p1->addr < p2->addr;
}

struct page *
find_page (struct hash *h, void *va)
{
	struct page p;
	struct hash_elem *e;
	
	p.addr = (void *)pg_round_down (va);
	e = hash_find (h, &p.page_elem);
	if (e != NULL)
		return hash_entry (e, struct page, page_elem);
	return NULL;
}

bool
handle_page_fault (struct intr_frame *f UNUSED, void *fault_addr UNUSED, bool not_present UNUSED, bool write UNUSED, bool user UNUSED)
{
	void *fault_page = (void *)pg_round_down (fault_addr);

//	printf("(handler)Page fault at %p: %s error %s page in %s context.\n", fault_page, not_present ? "not_present" : "rights violation", write ? "writing" : "reading", user ? "user" : "kernel");
	//printf("fault page : %p\n", fault_page);
	
	struct thread *t = thread_current ();
	//void *esp = user ? f->esp : t->user_esp;
	if (t->pages == NULL || !not_present)
		return false;

	// if addr over stack, grow stack
	/*
	if (esp - 8 <= fault_page && PHYS_BASE - 0x100000 <= fault_page)
		return grow_stack (fault_page);
		*/

	struct page *p = find_page (t->pages, fault_page);
	if (p == NULL)
		return false;

	if (p->frame == NULL)
	{
		printf("p->frame is null\n");
		//frame is not allocated
		allocate_frame (p);
		if (p->frame == NULL)
			return false;
		//swap?
	}
	//bring from disk
	/*
	printf("file %s and page's readbytes : %d\n", 
			p->file == NULL ? "not exists" : "exists",
			p->read_bytes
			);
			*/
	file_seek (p->file, p->offset);
	int n_read = file_read (p->file, p->frame->kva, p->read_bytes);
	if (n_read != (int)p->read_bytes)
		return false;
	memset (p->frame->kva + n_read, 0, p->zero_bytes);
	return true;
}

bool
grow_stack (struct hash *h, void *pagedir, void *addr_)
{
	struct page *p;
	void *addr = (void *)pg_round_down (addr_);
	p = find_page (h, addr);
	if (p == NULL)
  	p	= allocate_page (addr, NULL, 0, 0, 0, false);

	allocate_frame (p);
	if (p->frame == NULL)
		return false;

	if (pagedir_get_page (pagedir, addr) != NULL || !pagedir_set_page (pagedir, addr, p->frame->kva, true))
	{
		deallocate_frame (p->frame);
		return false;
	}
	
	return true;
}

bool
load_page (struct hash *h, uint32_t *pagedir, void *upage)
{
	struct page *p;
	p = find_page (h, upage);
	if (p == NULL)
		return false;
	
	if (p->frame == NULL)
	{
		if (!allocate_frame (p))
			return false;
	}

	//fetch data into frame
	int n_read = file_read (p->file, p->frame->kva, p->read_bytes);
	if (n_read != (int)p->read_bytes)
		return false;

	memset (p->frame->kva + p->read_bytes, 0, p->zero_bytes);

	if (!install_page (pagedir, upage, p->frame->kva))
	{
		deallocate_frame (p->frame);
		return false;
	}
	
	pagedir_set_dirty (pagedir, p->frame->kva, false);

	return true;
}
void
load_pages (const void *buffer, size_t size)
{
	struct thread *t = thread_current ();
	uint32_t *pagedir = t->pagedir;

	void *upage;
	for (upage = pg_round_down (buffer); upage < buffer + size; upage += PGSIZE)
	{
		load_page (t->pages, pagedir, upage);
	}
}

