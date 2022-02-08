/*
* Copyright (c) 2015-2021 Cadence Design Systems Inc.
*
* Permission is hereby granted, free of charge, to any person obtaining
* a copy of this software and associated documentation files (the
* "Software"), to deal in the Software without restriction, including
* without limitation the rights to use, copy, modify, merge, publish,
* distribute, sublicense, and/or sell copies of the Software, and to
* permit persons to whom the Software is furnished to do so, subject to
* the following conditions:
*
* The above copyright notice and this permission notice shall be included
* in all copies or substantial portions of the Software.
*
* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
* EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
* MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
* IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
* CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
* TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
* SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/
#include <stdio.h>
#include <stdlib.h>

#include "xa_type_def.h"

/* ...debugging facility */
#include "xf-debug.h"
#include "xaf-test.h"
#include "xaf-api.h"
#include "xaf-mem.h"

#include "xaf-fio-test.h"

mem_obj_t g_mem_obj;

void *mem_malloc(int size, int id)
{
    void *heap_ptr = NULL;
    
    if (id != XAF_MEM_ID_DEV && id != XAF_MEM_ID_COMP)
    {
        return NULL;
    }
    
    heap_ptr = malloc(size);

    if (id == XAF_MEM_ID_DEV)
    {
        g_mem_obj.num_malloc_dev++;
        g_mem_obj.persi_mem_dev += size;
    }
    else
    {
        g_mem_obj.num_malloc_comp++;
        g_mem_obj.persi_mem_comp += size;
    }    

    return heap_ptr; 
}

void mem_free(void * heap_ptr, int id)
{
    if (id == XAF_MEM_ID_DEV)
        g_mem_obj.num_malloc_dev--;
    else if (id == XAF_MEM_ID_COMP)
        g_mem_obj.num_malloc_comp--;

    free(heap_ptr);
}

int mem_get_alloc_size(mem_obj_t* mem_handle, int id)
{
    int mem_size = 0;
    if(id == XAF_MEM_ID_DEV)
        mem_size =  g_mem_obj.persi_mem_dev;
    else if(id == XAF_MEM_ID_COMP)
        mem_size = g_mem_obj.persi_mem_comp;
    return mem_size;
}

void* mem_init()
{
    void* ptr;
    ptr = &g_mem_obj;
    return ptr;
}

void mem_exit()
{
    if((g_mem_obj.num_malloc_dev != 0)||(g_mem_obj.num_malloc_comp != 0))
    {
        FIO_PRINTF(stdout,"\nMEMORY LEAK ERROR!!! All the allocated memory is not freed.\n\n\n");
    }
    return;
}
