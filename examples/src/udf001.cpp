extern "C" {
#include "udf.h"
}

DEFINE_ON_DEMAND(on_demand) {
#if !RP_HOST
    if (!Data_Valid_P()) return;
    Domain *domain = Get_Domain(1);

    Thread *ct;
    thread_loop_c(ct, domain) {
        Message("%d: %s\n", myid, ct->name);
    }
    Thread *ft;
    thread_loop_f(ft, domain) {
        Message("%d: %s\n", myid, ft->name);
    }
#endif
}
