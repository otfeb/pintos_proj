/* vm.c: Generic interface for virtual memory objects. */

#include "vm/vm.h"

#include <stdbool.h>

#include "hash.h"
#include "threads/malloc.h"
#include "threads/vaddr.h"
#include "vm/inspect.h"
#include "vm/uninit.h"
/* Initializes the virtual memory subsystem by invoking each subsystem's
 * intialize codes. */
#define VA_MASK(va) ((uint64_t)(va) & ~(uint64_t)0xFFF)

struct list frame_table;
void vm_init(void) {
    vm_anon_init();
    vm_file_init();
    list_init(&frame_table);
#ifdef EFILESYS /* For project 4 */
    pagecache_init();
#endif
    register_inspect_intr();
    /* DO NOT MODIFY UPPER LINES. */
    /* TODO: Your code goes here. */
}

/* Get the type of the page. This function is useful if you want to know the
 * type of the page after it will be initialized.
 * This function is fully implemented now. */
enum vm_type
page_get_type(struct page *page) {
    int ty = VM_TYPE(page->operations->type);
    switch (ty) {
        case VM_UNINIT:
            return VM_TYPE(page->uninit.type);
        default:
            return ty;
    }
}

/* Helpers */
static struct frame *vm_get_victim(void);
static bool vm_do_claim_page(struct page *page);
static struct frame *vm_evict_frame(void);

/* 이니셜라이저로 보류 중인 페이지 객체를 만듭니다.
페이지를 만들려면 직접 만들지 말고
이 함수 또는 `vm_alloc_page`를 통해 만드세요.*/
bool vm_alloc_page_with_initializer(enum vm_type type, void *upage, bool writable, vm_initializer *init, void *aux)
// ↳ page type, page의 가상주소?, write 가능여부, page를 실제로 올릴때 실행하는 함수(vm_initializer), vm_initializer함수의 실행시에 넘겨주는 인자
{
    ASSERT(VM_TYPE(type) != VM_UNINIT)  // vm_type은 VM_ANON과 VM_FILE만 가능하다.

    struct supplemental_page_table *spt = &thread_current()->spt;

    /* Check wheter the upage is already occupied or not. */
    if (spt_find_page(spt, upage) == NULL) {
        // ↳ upage라는 가상 메모리에 매핑되는 페이지 존재 x -> 새로 만들어야함
        /* TODO: Create the page, fetch the initialier according to the VM type,
         * TODO: and then create "uninit" page struct by calling uninit_new. You
         * TODO: should modify the field after calling the uninit_new.
         * uninit_new를 호출한 후 필드를 수정해야 함*/
        /*-------------------------[P3]Anonoymous page---------------------------------*/
        struct page *pg = calloc(sizeof(struct page), sizeof(struct page));  // !

        // 페이지 타입에 따라 initializer가 될 초기화 함수를 매칭해준다.
        bool (*initializer)(struct page *, enum vm_type, void *);

        if (VM_TYPE(type) == VM_ANON)
            initializer = anon_initializer;
        else if (VM_TYPE(type) == VM_FILE)
            initializer = file_backed_initializer;

        uninit_new(pg, upage, init, type, aux, initializer);  // UNINIT 페이지 생성
        pg->writable = writable;
        spt_insert_page(spt, pg);
        return true;
        /*-------------------------[P3]Anonoymous page---------------------------------*/
    }
err:
    return false;
}
/* Find VA from spt and return page. On error, return NULL. */
struct page *
spt_find_page(struct supplemental_page_table *spt UNUSED, void *va UNUSED) {
    struct page *page = NULL;
    // REVIEW find_PAGE
    // spt->hash_page   hash테이블이고
    //  가상주소 va는 hash_find로 elem찾아서 해야하나?
    struct hash_elem *tmp;
    tmp = hash_find(&spt->hash_page, &page->hash_elem);
    struct page *tmp_page = hash_entry(tmp, struct page, hash_elem);
    return (tmp_page == NULL) ? NULL : tmp_page;
}

/* Insert PAGE into spt with validation. */
bool spt_insert_page(struct supplemental_page_table *spt UNUSED,
                     struct page *page UNUSED) {
    int succ = false;
    // REVIEW INSERT_PAGE
    struct hash_elem *tmp;
    struct page *tmp_page = hash_entry(tmp, struct page, hash_elem);
    tmp = hash_insert(&spt->hash_page, &tmp_page->hash_elem);
    if (tmp == NULL) {  // 잘 추가되었으면 NULL반환
        succ = true;
    }
    return succ;
}

void spt_remove_page(struct supplemental_page_table *spt, struct page *page) {
    vm_dealloc_page(page);
    return true;
}

/* Get the struct frame, that will be evicted. */
static struct frame *
vm_get_victim(void) {
    struct frame *victim = NULL;
    /* TODO: The policy for eviction is up to you. */
    victim = list_pop_front(&frame_table);
    // REVIEW list가 비어있다면 ?
    return victim;
}

/* Evict one page and return the corresponding frame.
 * Return NULL on error.*/
static struct frame *
vm_evict_frame(void) {
    struct frame *victim UNUSED = vm_get_victim();
    swap_out(victim->page);
    /* TODO: swap out the victim and return the evicted frame. */
    return NULL;
}

/* palloc() and get frame. If there is no available page, evict the page
 * and return it. This always return valid address. That is, if the user pool
 * memory is full, this function evicts the frame to get the available memory
 * space.*/
static struct frame *
vm_get_frame(void) {
    struct frame *frame = NULL;
    PANIC("TODO");
    ASSERT(frame != NULL);
    ASSERT(frame->page == NULL);
    list_push_back(&frame_table, &frame->frame_elem);
    return frame;
}

/* Growing the stack. */
static void
vm_stack_growth(void *addr UNUSED) {
}

/* Handle the fault on write_protected page */
static bool
vm_handle_wp(struct page *page UNUSED) {
}

/* Return true on success */
bool vm_try_handle_fault(struct intr_frame *f UNUSED, void *addr UNUSED,
                         bool user UNUSED, bool write UNUSED, bool not_present UNUSED) {
    struct supplemental_page_table *spt UNUSED = &thread_current()->spt;
    struct page *page = NULL;
    /* TODO: Validate the fault */
    /* TODO: Your code goes here */

    return vm_do_claim_page(page);
}

/* Free the page.
 * DO NOT MODIFY THIS FUNCTION. */
void vm_dealloc_page(struct page *page) {
    destroy(page);
    free(page);
}

/* Claim the page that allocate on VA. */
bool vm_claim_page(void *va UNUSED) {
    struct page *page = NULL;
    struct supplemental_page_table spt = thread_current()->spt;
    page = spt_find_page(&spt, va);
    return vm_do_claim_page(page);
}

/* Claim the PAGE and set up the mmu. */
static bool
vm_do_claim_page(struct page *page) {
    struct frame *frame = vm_get_frame();
    /* Set links */
    frame->page = page;
    page->frame = frame;
    if (!pml4_get_page((uint64_t *)page, &page->va)) {
        frame->kva = ptov(&page->va);
    };

    return swap_in(page, frame->kva);
}

unsigned
page_hash(const struct hash_elem *p_, void *aux UNUSED) {
    const struct page *p = hash_entry(p_, struct page, hash_elem);
    uint64_t masked_va = VA_MASK(p->va);  // VA_MASK
    return hash_bytes(&masked_va, sizeof masked_va);
}

bool page_less(const struct hash_elem *a_,
               const struct hash_elem *b_, void *aux UNUSED) {
    const struct page *a = hash_entry(a_, struct page, hash_elem);
    const struct page *b = hash_entry(b_, struct page, hash_elem);

    return a->va < b->va;
}

/* Initialize new supplemental page table */
void supplemental_page_table_init(struct supplemental_page_table *spt UNUSED) {
    hash_init(&spt->hash_page, page_hash, page_less, NULL);
}

/* Copy supplemental page table from src to dst */
bool supplemental_page_table_copy(struct supplemental_page_table *dst UNUSED,
                                  struct supplemental_page_table *src UNUSED) {
}

/* Free the resource hold by the supplemental page table */
void supplemental_page_table_kill(struct supplemental_page_table *spt UNUSED) {
    /* TODO: Destroy all the supplemental_page_table hold by thread and
     * TODO: writeback all the modified contents to the storage. */
}
