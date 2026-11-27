#ifndef __RAY_SOLID_QUERY_STATUS__
#define __RAY_SOLID_QUERY_STATUS__

// Supported includes an empty boundary list (all inside or all outside).
class RaySolidQueryStatus { public: enum Value { Unsupported, Supported, InvalidInput, Indeterminate }; };

#endif
