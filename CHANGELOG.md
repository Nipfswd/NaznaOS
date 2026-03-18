## idk about versioning this early
- **Remove macro redefinitions** from ``kernel/mm/paging.c`` for symbols already defined in `kernel/mm/paging.h` (``PAGE_SIZE``, ``PAGE_PRESENT``, ``PAGE_RAW``)