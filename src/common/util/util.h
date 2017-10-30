#pragma once

#define KB(x)   ((size_t) (x) << 10)
#define MB(x)   ((size_t) (x) << 20)
#define GB(x)   ((size_t) (x) << 30)
#define TB(x)   ((size_t) (x) << 40)

namespace LTFSDM {
void init(std::string ident = "");
long getkey();
}
