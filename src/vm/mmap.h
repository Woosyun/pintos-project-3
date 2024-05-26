#include "lib/kernel/list.h"
#include "userprog/syscall.h"
#include "threads/thread.h"

struct mmap {
	struct list_elem mmap_elem;
	int id;
	struct file *file;
	size_t size; // size of file
	void *addr; // uva
};

struct mmap *lookup_mmap (struct thread *t, int id);
int mmap (int id, void *upage, struct file *f);
