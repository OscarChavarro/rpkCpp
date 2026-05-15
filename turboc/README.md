# Turbo C++ Notes for rpkCpp

This is a legacy C++ port aimed to build on Borland Turbo C 3.0 so
project can build for DOS.

## File Renaming Workflow (Long Names vs 8.3)

Development in this repository is done with long, descriptive file names.
The Git repository keeps those long names, and CMake builds are expected to
run with those long names.

For DOS/Turbo C builds, 8.3 file names are required. Use the renaming
mapping and scripts in this folder to switch between both layouts:

- `mappings.csv`: canonical mapping between long paths and 8.3 paths
- `scripts/map_to_83.py`: rename from long names to 8.3 names
- `scripts/map_from_83.py`: restore from 8.3 names back to long names

Recommended flow:

1. Normal development and CMake builds: keep long names.
2. Before DOS/Turbo C compilation: apply 8.3 renaming.
3. After DOS/Turbo C work: restore long names before committing.

Examples:

```bash
# Preview changes only
python3 scripts/map_to_83.py --dry-run

# Apply 8.3 renaming for DOS build
python3 scripts/map_to_83.py

# Restore long names for regular development
python3 scripts/map_from_83.py
```

## C++ Feature Changes (cpp -> turboc)

Changes actually applied in this port are listed here. C++98/C++11 differences.

1. `enum class` (scoped enums) -> traditional `enum`
2. `nullptr` -> `NULL`
3. `override`/`final` removed from class hierarchies
4. `constexpr` replaced with C++98 alternatives (`static const` or macros)
5. C++ casts (`static_cast`, `reinterpret_cast`) -> C-style casts
6. Simplified C++11 initialization -> explicit C++98 forms
   - Uniform initialization: `Type obj{}` -> `Type obj = Type()`
   - Delegating constructor removed in `LookUpTable`
7. Template usage simplified in `HashMap`
   - Removed member-template hash overloads (`hashKeyValue<T>(...)`)
   - Kept a single class-template-aware helper: `hashKeyValue(const K&)`
   - Goal: reduce template complexity for older compilers
8. `try/catch` removed from binary model deserialization path
   - Replaced exception wrapper with explicit checks + `goto fail` cleanup flow
   - Goal: keep deterministic C++98-compatible control flow
9. Remove all uses of namespaces. Those were not still invented in early 1990s.
