#include <stdio.h>
#include <unistd.h>
#include <assert.h>

#include "test.h"
#include "cachetable.h"

static void
flush (CACHEFILE f __attribute__((__unused__)),
       CACHEKEY k  __attribute__((__unused__)),
       void *v     __attribute__((__unused__)),
       void *e     __attribute__((__unused__)),
       long s      __attribute__((__unused__)),
       BOOL w      __attribute__((__unused__)),
       BOOL keep   __attribute__((__unused__)),
       LSN m       __attribute__((__unused__)),
       BOOL r      __attribute__((__unused__))
       ) {
    /* Do nothing */
}

static int
fetch (CACHEFILE f        __attribute__((__unused__)),
       CACHEKEY k         __attribute__((__unused__)),
       u_int32_t fullhash __attribute__((__unused__)),
       void **value       __attribute__((__unused__)),
       long *sizep        __attribute__((__unused__)),
       void *extraargs    __attribute__((__unused__)),
       LSN *written_lsn    __attribute__((__unused__))
       ) {
    return 0;
}

static void
cachetable_count_pinned_test (int n) {
    const int test_limit = 2*n;
    int r;
    CACHETABLE ct;
    r = toku_create_cachetable(&ct, test_limit, ZERO_LSN, NULL_LOGGER); assert(r == 0);
    char fname1[] = __FILE__ "test1.dat";
    unlink(fname1);
    CACHEFILE f1;
    r = toku_cachetable_openf(&f1, ct, fname1, O_RDWR|O_CREAT, 0777); assert(r == 0);

    int i;
    for (i=1; i<=n; i++) {
        u_int32_t hi;
        hi = toku_cachetable_hash(f1, make_blocknum(i));
        r = toku_cachetable_put(f1, make_blocknum(i), hi, (void *)(long)i, 1, flush, fetch, 0);
        assert(r == 0);
        assert(toku_cachefile_count_pinned(f1, 0) == i);

        void *v;
        r = toku_cachetable_maybe_get_and_pin(f1, make_blocknum(i), hi, &v);
        assert(r == 0);
        assert(toku_cachefile_count_pinned(f1, 0) == i);

        r = toku_cachetable_unpin(f1, make_blocknum(i), hi, CACHETABLE_CLEAN, 1);
        assert(r == 0);
        assert(toku_cachefile_count_pinned(f1, 0) == i);
    }
    for (i=n; i>0; i--) {
        u_int32_t hi;
        hi = toku_cachetable_hash(f1, make_blocknum(i));
        r = toku_cachetable_unpin(f1, make_blocknum(i), hi, CACHETABLE_CLEAN, 1);
        assert(r == 0);
        if (i-1) assert(toku_cachetable_assert_all_unpinned(ct));
        assert(toku_cachefile_count_pinned(f1, 0) == i-1);
    }
    assert(toku_cachetable_assert_all_unpinned(ct) == 0);
    assert(toku_cachefile_count_pinned(f1, 1) == 0);
    toku_cachetable_verify(ct);

    r = toku_cachefile_close(&f1, NULL_LOGGER); assert(r == 0 && f1 == 0);
    r = toku_cachetable_close(&ct); assert(r == 0 && ct == 0);
}

int main(int argc, const char *argv[]) {
    int i;
    for (i=1; i<argc; i++) {
        if (strcmp(argv[i], "-v") == 0) {
            verbose++;
            continue;
        }
    }
    cachetable_count_pinned_test(8);
    return 0;
}
