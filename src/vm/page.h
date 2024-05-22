#ifndef _PAGEH_
#define _PAGEH_

#include <stdbool.h>
#include <hash.h>
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

	bool read_only;
	off_t offset;
	off_t file_bytes;

	block_sector_t sector;
};

void deallocate_page (void *vaddr);
struct page *allocate_page (void *vaddr, bool writable);
unsigned page_hash (const struct hash_elem *e, void *aux);
bool page_cmp (const struct hash_elem *e1, const struct hash_elem *e2, void *aux);
//struct page *addr_to_page (const void *addr);
bool handle_page_fault (struct intr_frame *f, void *fault_addr, bool not_present, bool write, bool user);

#endif
