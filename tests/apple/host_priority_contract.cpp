#include "src/host/host_priority.h"
#include <cassert>
#include <cstdlib>
int main() {
    setenv("RAC1_PRIORITY", "1", 1);
    rac1::host_priority_set_enabled(true);
    assert(!rac1::host_priority_enabled());
    assert(!rac1::host_priority_realtime(123, 10));
    assert(rac1::host_priority_raise_all(-10) == 0);
    rac1::host_priority_set_enabled(false);
    assert(!rac1::host_priority_enabled());
}
