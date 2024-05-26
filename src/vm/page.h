#ifndef _PAGEH_
#define _PAGEH_

#include <stdbool.h>
#include <hash.h>
#include "lib/kernel/hash.h"
#include "filesys/off_t.h"
#include "vm/frame.h"
#include "devices/block.h"
#include "threads/interrupt.h"

struct page {
	void *addr; //user virtual address
	struct thread *thread;
	struct frame *frame; // mapped frame
	struct file *file;

	struct hash_elem page_elem;

	off_t offset;
	off_t read_bytes;
	off_t zero_bytes;

	bool dirty;
	bool read_only;
};

struct hash *pages_init (void);
void deallocate_page (struct hash *h, uint32_t *pagedir, void *page, struct file *f, off_t offset, size_t bytes);
bool create_page (void *upage, bool writable);
struct page *allocate_page (void *vaddr, struct file *f, off_t offset, uint32_t read_bytes, uint32_t zero_bytes, bool writable);
//unsigned page_hash (const struct hash_elem *e, void *aux);
//bool page_cmp (const struct hash_elem *e1, const struct hash_elem *e2, void *aux);
//struct page *addr_to_page (const void *addr);
void destroy_pages (struct hash *h);
void free_pages (struct hash *h);
struct page *find_page (struct hash *h, void *addr);
bool handle_page_fault (struct intr_frame *f, void *fault_addr, bool not_present, bool write, bool user);
bool grow_stack (struct hash *h, void *pagedir, void *addr);
void load_pages (const void *buffer, size_t size);

#endif
