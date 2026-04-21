#include "../include/shared.h"
#include "../include/weak.h"

void odr_helper_use() {
    auto sp = MakeShared<int>(1);
    WeakPtr<int> wp(sp);
    (void)wp.Lock();
}