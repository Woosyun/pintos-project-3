#include "lib/kernel/list.h"

struct mmap {
	struct list_elem mmap_elem;
	int id;
	struct file *file;
	uint8_t *base;
	size_t page_count;
};

struct mmap *lookup_mmap (int id);
