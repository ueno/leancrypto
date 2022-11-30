/*
 * Copyright (C) 2022, Stephan Mueller <smueller@chronox.de>
 *
 * License: see LICENSE file in root directory
 *
 * THIS SOFTWARE IS PROVIDED ``AS IS'' AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE, ALL OF
 * WHICH ARE HEREBY DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT
 * OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR
 * BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
 * LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE
 * USE OF THIS SOFTWARE, EVEN IF NOT ADVISED OF THE POSSIBILITY OF SUCH
 * DAMAGE.
 */

#include <linux/slab.h>
#include <linux/mm.h>

#include "lc_memory_support.h"
#include "posix_support.h"

const int errno = 0;

int lc_alloc_aligned(void **memptr, size_t alignment, size_t size)
{
	void *mem = kmalloc(size, GFP_KERNEL);

	WARN_ON(alignment > ARCH_KMALLOC_MINALIGN);
	if (!mem)
		return -ENOMEM;

	*memptr = mem;
	return -0;
}
EXPORT_SYMBOL(lc_alloc_aligned);

void lc_free(void *ptr)
{
	kfree(ptr);
}
EXPORT_SYMBOL(lc_free);

int lc_alloc_high_aligned(void **memptr, size_t alignment, size_t size)
{
	struct page *pages = alloc_pages(GFP_KERNEL,
					 get_order((unsigned long)size));

	if (!pages)
                return -ENOMEM;

	*memptr = page_address(pages);
	return 0;
}
EXPORT_SYMBOL(lc_alloc_high_aligned);

void lc_free_high_aligned(void *ptr, size_t size)
{
	__free_pages(virt_to_page(ptr), get_order((unsigned long)size));
}
EXPORT_SYMBOL(lc_free_high_aligned);
