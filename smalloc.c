    #include <stdio.h>
    #include <stdlib.h>
    #include <unistd.h>
    #include <sys/types.h>
    #include <sys/stat.h>
    #include <fcntl.h>
    #include <sys/mman.h>
    #include "smalloc.h"
    #include <string.h>
    #include <stdbool.h>

    //
    // Explicit List Implementation
    /*
    64 BIT SYSTEMM         
    Given Block [[BLOCK SIZE int 4 bytes AND Allocated int 4 BYTES][NEXT FREE][PREV FREE][PAYLOAD BLOCK SIZE AND PADDING)]]
    1 = Allocated
    0 = FREE
    */ 

    //globs
    #define ROUND_VAL 4096
    #define HEADER 24
    #define CASE1 0
    #define CASE2 1
    #define CASE3 2
    #define CASE4 3

    static void* header = NULL;
    static void* p = NULL;

    void myPrint(char *msg)
    {
        write(STDOUT_FILENO, msg, strlen(msg));
    }

    int round_up(int pre_round, int round){
        if(pre_round % round == 0){
        return pre_round;
        }
        return ((pre_round / round) + 1) * round;

    }

    void set_size(void *block, int size){
        *(int*)block = size;
    }

    int get_size(void* block){
        return *(int*)block;
    }

    void set_alloc(void *block, int val){
        *(int*)((char*)block + 4) = val;
    }

    void set_next(void *block, void *next){
        *(void**)((char*)block + 8) = next;
    }

    void* get_next(void* block){
        return *(void**)((char*)block + 8);
    }

    void set_prev(void *block, void *next){
        *(void**)((char*)block + 16) = next;
    }

    void* get_prev(void *block){
        return *(void**)((char*)block + 16);

    }

    void* get_payload(void* block){
        return (void*)((char*)block + HEADER);
    }

    void* get_header(void* payload){
        return (void*)((char*)payload - HEADER);
    }

    int find_pos(void* block){
        void* next = get_next(block);
        void* prev = get_prev(block);

        if (prev == NULL && next == NULL) {
            return CASE1;
        } else if (prev == NULL) {
            return CASE2;
        } else if (next == NULL) {
            return CASE3;
        } else {
            return CASE4;
        }
    }

    void heal(void* block, void* prev, void* next){
        void* nnext = get_next(block);
        void* pprev = get_prev(block);

        if(pprev != NULL){
            set_next(pprev, prev);
        }else{
            header = prev;
        }

        if(nnext != NULL){
            set_prev(nnext, next);
        }
    }

    void remove_free(void* block){
        heal(block, get_next(block), get_prev(block));
        
    }

    void put_inside(void* prev, void* newa){
        set_next(newa, get_next(prev));
        set_prev(newa, get_prev(prev));
        heal(prev, newa, newa);

    }

    void* create_free(void* where, int size){
        set_size(where, size);
        set_alloc(where, 0);
        set_next(where, NULL);
        set_prev(where, NULL);

        return where;
    }

    void create_alloc(void* where, int size){
        set_size(where, size);
        set_alloc(where, 1);
        set_next(where, NULL);
        set_prev(where, NULL);
    }

    int p_offset(void* pp){
        return (int)((unsigned long)pp - (unsigned long)p);
    }

    bool compare_add(void* add_a, void* add_b){
        return (unsigned long)(add_a) < (unsigned long)(add_b);
    }

    void go_through(void* block, void** left, void** right){
        *left = header;
        while(get_next(*left) != NULL && compare_add(get_next(*left), block)){
            *left = get_next(*left);
        }
        *right = get_next(*left);
    }

    void put_between(void* block, void* left, void* right){
        set_alloc(block, 0);
        set_next(block, right);
        set_prev(block, left);

        if(left != NULL){
            set_next(left, block);
        }
        if(right != NULL){
            set_prev(right, block);
        }
    }

    bool check_adj(void* left, void* right){
        return (unsigned long)left + get_size(left) == (unsigned long) right;
    }

    void merge_right(void* block){
        void *right = get_next(block);
        set_size(block, get_size(block) + get_size(right));

        void* rright = get_next(right);
        set_next(block, rright);
        if(rright != NULL){
            set_prev(rright, block);
        }
    }

    void merge_left(void* block){
        void* left = get_prev(block);
        set_size(left, get_size(left) + get_size(block));

        void* right = get_next(block);
        set_next(left, right);
        if(right != NULL){
            set_prev(right, left);
        }
    }


    void merge_it(void* block){
           int pos = find_pos(block);

           if(pos == CASE2 || pos == CASE4){
                if(check_adj(block, get_next(block))){
                    merge_right(block);
                }
           }

           pos = find_pos(block);

           if(pos == CASE3 || pos == CASE4){
                if(check_adj(get_prev(block), block)){
                    merge_left(block);
                }
           }
    
    
    }
    


    void break_block(void* curr, int size){
        int ssize = get_size(curr);
        int new_pay = round_up(size, 8);
        int new_size = HEADER + new_pay;
        int rest = ssize - new_size;
        
        void* free = (void*)(char*)(curr + new_size);
        create_free(free, rest);
        put_inside(curr, free);
        create_alloc(curr, new_size);
    }

    void  it_worked(Malloc_Status* status, void* pay, int hops){
        status->success = 1;
        status->payload_offset = p_offset(pay);
        status->hops = hops;
    }

    void didnt_work(Malloc_Status* status){
        status->success = 0;
        status->payload_offset = -1;
        status->hops = -1;
    }
    /*
    * my_init() is called one time by the application program to to perform any 
    * necessary initializations, such as allocating the initial heap area.
    * size_of_region is the number of bytes that you should request from the OS using
    * mmap().
    * Note that you need to round up this amount so that you request memory in 
    * units of the page size, which is defined as 4096 Bytes in this project.
    */
    int my_init(int size_of_region) {
    /*
    * Implement your initialization here.
    */
        int round = round_up(size_of_region, ROUND_VAL);
        int fd = open("/dev/zero", O_RDWR);

        void* initial = mmap(NULL, round, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
        close(fd);

        if(initial == MAP_FAILED){
            return -1;
        }

        create_free(initial, round);
        p = initial;
        header = initial;
        return 0;
    }

    /*
    * get_head_pointer() returns an offset to the start of head block (not the payload) 
    * relative to the start of the heap.
    * This should be (unsigned long)head_pointer - (unsigned long)start_of_heap.
    * If there is no free block, return -1.
    */
    int get_head_pointer(Pointer_Status* status) {
    /*
    * Implement your get_head_pointer here.
    */
        if(header == NULL){
            status->success = 2;
            status->block_size = -1;
            return -1;
        }

        status->success = 3;
        status->block_size = get_size(header);
        return p_offset(header);
    }

    /*
    * get_next_pointer() takes as input a pointer to the payload of a block
    * and returns an offset to the payload of the next free block in the 
    * free list (relative to the start of the heap).
    * If there is no next free block, return -1.
    * NOTE: "curr" points to the start of the payload of a block.
    */
    int get_next_pointer(void *curr) {
    /*
    * Implement your get_next_pointer here.
    */

        void* block = get_header(curr);
        void* next = get_next(block);

        if(next == NULL){
        return -1;
        }

        return p_offset(get_payload(next));
    }

    /*
    * get_prev_pointer() takes as input a pointer to the payload of a block
    * and returns an offset to the payload of the previous free block in the 
    * free list (relative to the start of the heap).
    * If there is no previous free block, return -1.
    * NOTE: "curr" points to the start of the payload of a block.
    */
    int get_prev_pointer(void *curr) {
    /*
    * Implement your get_prev_pointer here.
    */

        void* block = get_header(curr);
        
        void* prev = get_prev(block);

        if(prev == NULL){
            return -1;
        }

        return p_offset(get_payload(prev));
    }

    /*
    * smalloc() takes as input the size in bytes of the payload to be allocated and 
    * returns a pointer to the start of the payload. The function returns NULL if 
    * there is not enough contiguous free space within the memory allocated 
    * by my_init() to satisfy this request.
    */
    void *smalloc(int size_of_payload, Malloc_Status *status) {
    /*
    * Implement your malloc here.
    */
        void* curr = header;
        int hops = 0;

        while(curr != NULL){
            int size = get_size(curr);

            if(size >= HEADER + size_of_payload){
                int extra = size - HEADER - size_of_payload;

                if(extra < HEADER){
                    remove_free(curr);
                    create_alloc(curr, size);

                }else{
                    break_block(curr, size_of_payload);
                }

                it_worked(status, get_payload(curr), hops);
                return get_payload(curr);
        }
        hops += 1;
        curr = get_next(curr);
    
    }
        didnt_work(status);
        return NULL;
}


    /*
    * sfree() frees the target block. "ptr" points to the start of the payload.
    * NOTE: "ptr" points to the start of the payload, rather than the block (header).
    */
    void sfree(void *ptr)
    {
        if(ptr == NULL){
            return;
        }

        void *block = get_header(ptr);

        if(header == NULL){
            put_between(block, NULL, NULL);
            header = block;
            return;
        }

        if(compare_add(block, header)){
            put_between(block, NULL, header);
            header = block;
        }else{
            void* left = NULL;
            void* right = NULL;
            go_through(block, &left, &right);
            put_between(block, left, right);
        }
        merge_it(block);


    }