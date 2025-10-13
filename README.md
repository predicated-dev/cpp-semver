# cpp-semver

## Introduction

`cpp-semver` is a C++ library with C exports for parsing, handling, and querying semantic versions per [Semantic Versioning 2.0.0](https://semver.org/spec/v2.0.0.html) and the [Semantic Version Query Language Specification](Semantic%20Version%20Query%20Language%20Specification.md). It supports parsing version strings, comparing versions, and matching versions against **Range Sets**. It extends node-semver's query syntax with **Pre-release Extensions** (`@<min-prerelease-label>`) for precise pre-release control.

The library is compiles to a Windows DLL / Linux shared library with a C-style API. It’s proprietary, viewable for reference but not licensed for use.

Consider contributing to extend support to other platforms

## Building the Project

Using CMake:

```bash
mkdir build && cd build
cmake ..
cmake --build . --config Debug
ctest -C Debug
```


Or open `CMakeLists.txt` in Visual Studio.

## Usage

API in `api/semver.h` uses C-style exports. API calls are made against transparent handles.

### Creating and Disposing Objects

**Version** (`HSemverVersion`), **Version Array** (`HSemverVersions`) and **Query**(`HSemverQuery`) objects must be constructed before use and disposed to free memory. 

- **Versions** follow [Semantic Versioning 2.0.0](https://semver.org/spec/v2.0.0.html).
- **Queries** follow the [Semantic Version Query Language Specification](Semantic%20Version%20Query%20Language%20Specification.md), extending node-semver (e.g., `>=1.2.3 @rc`).
- Dispose does not modify handles. Callers should consider setting to NULL (0), to track disposed resources.

#### Basic Constructors
```cpp 
SEMVER_API HSemverVersion semver_version_create();

SEMVER_API HSemverVersions semver_versions_create(size_t count);

SEMVER_API HSemverQuery semver_query_create();
```

#### Defining constructors

Versions can be fully defined during construction:

```cpp
SEMVER_API HSemverVersion semver_version_create_defined(uint64_t major, uint64_t minor, uint64_t patch, const char* prerelease, const char* build); 
```

Version arrays can also be constructed and filled from a string with separator:

```cpp
SEMVER_API HSemverVersions semver_versions_from_string(const char* versions_str, const char* separator, SemverOrder order);
```

The separator is specified as a string, but you can also pass `NULL` or an empty string to split on '\0'. The order specifies how you want the results sorted per  [Semantic Versioning 2.0.0 precedence rules](https://semver.org/spec/v2.0.0.html#spec-item-11):

``` cpp
enum SemverOrder : uint8_t
{
   SEMVER_ORDER_AS_GIVEN,
   SEMVER_ORDER_DESC,
   SEMVER_ORDER_ASC,
};
```

Sorting can speed up matching against queries.


#### Destructors

All objects instantiated via constructors must be disposed:

```cpp
SEMVER_API void semver_version_dispose(HSemverVersion version);

SEMVER_API void semver_versions_dispose(HSemverVersions version_array);

SEMVER_API void semver_query_dispose(HSemverQuery query);
```

###  Parsing Versions

To fill a **Version** object from a parsed string:

```cpp
SEMVER_API SemverParseResult semver_version_parse(HSemverVersion version, const char* version_str);
```

To test strings for validity without a **Version** object:

```cpp
SEMVER_API SemverParseResult semver_check_version_string(const char* version_str);
```

Parses versions conforming to the  [Semantic Versioning 2.0.0](https://semver.org/spec/v2.0.0.html).

`SemverParseResult` errors:

| Result                                    | Description                                                                 | Fatal? |
|-------------------------------------------|-----------------------------------------------------------------------------|--------|
| `SUCCESS`                                 | Parsed successfully                                                 |      |
| `LEADING_WHITESPACE`                      | Whitespace before version (hint, continues)                         | No     |
| `TRAILING_WHITESPACE`                     | Whitespace after version (hint, continues)                          | No     |
| `EMPTY_VERSION_STRING`                    | Empty, NULL or only whitespace string                                                | Yes    |
| `TOO_FEW_PARTS`                           | Fewer than three parts (MAJOR.MINOR.PATCH)                          | Yes    |
| `TOO_MANY_PARTS`                          | Extra dots before pre-release/build                                 | Yes    |
| `MAJOR_EMPTY`, `MINOR_EMPTY`, `PATCH_EMPTY` | Core triplet value missing (e.g., starts with dot, `3..2`)         | Yes    |
| `MAJOR_NOT_NUMERIC`, `MINOR_NOT_NUMERIC`, `PATCH_NOT_NUMERIC` | Non-numeric core triplet value (e.g., `1.2.a`)          | Yes    |
| `MAJOR_LEADING_ZERO`, `MINOR_LEADING_ZERO`, `PATCH_LEADING_ZERO` | Leading zero on core triplet (e.g., `1.2.03`)               | Yes    |
| `MAJOR_WILDCARD`, `MINOR_WILDCARD`, `PATCH_WILDCARD` | Wildcard in version (e.g., `2.x.3`). Only valid for **Version Patterns** in **Query** **Ranges**                            | Yes    |
| `MAJOR_TOO_LARGE`, `MINOR_TOO_LARGE`, `PATCH_TOO_LARGE` | Core triplet value exceeds 2^53-1                              | Yes    |
| `PRERELEASE_EMPTY_IDENTIFIER`             | Empty pre-release identifier (e.g., `1.2.3-alpha..beta`)            | Yes    |
| `PRERELEASE_UNSUPPORTED_CHARACTER`        | Invalid pre-release chars (valid: `[0-9A-Za-z-]`)                   | Yes    |
| `PRERELEASE_LEADING_ZERO_ON_NUMERIC_IDENTIFIER` | Leading zero in numeric pre-release (e.g., `1.2.3-alpha.01`)  | Yes    |
| `BUILD_EMPTY_IDENTIFIER`                  | Empty build identifier (e.g., `1.2.3+build..123`)                   | Yes    |
| `BUILD_UNSUPPORTED_CHARACTER`             | Invalid build chars (valid: `[0-9A-Za-z-]`)                         | Yes    |

Fatal errors halt parsing; non-fatal (whitespace) continue with warnings.

### Parsing Queries

To parse queries conforming to the [Semantic Version Query Language Specification](Semantic%20Version%20Query%20Language%20Specification.md)
```cpp
SEMVER_API SemverQueryParseResult semver_query_parse(HSemverQuery query, const char* query_str);
```

Reports `SEMVER_QUERY_PARSE_SUCCESS` if successfully parsed. Errors in **Version Patterns** are equivalent in value to those reported by parsing **Version** objects, except named with a `_QUERY_` infix. For instance `SEMVER_QUERY_PARSE_MAJOR_LEADING_ZERO = SEMVER_PARSE_MAJOR_LEADING_ZERO`. Whitespace and empty strings do not produce errors and wildcards are accepted.

Errors in parsing the **Pre-release Extension** (`@<min-prerelease-label>`) are reported in enums similarly named to **Version Pattern** pre-release errors, but have include `_MIN_` infix and have different ordinal values 
```cpp
SEMVER_QUERY_PARSE_MIN_PRERELEASE_EMPTY_IDENTIFIER,
SEMVER_QUERY_PARSE_MIN_PRERELEASE_UNSUPPORTED_CHARACTER,
SEMVER_QUERY_PARSE_MIN_PRERELEASE_LEADING_ZERO_ON_NUMERIC_IDENTIFIER,
```

NOTE: Even though the string parsed for a **Range** can include any number of **Comparators** each representing a **Range** themselves their intersection will produce a single **Range** with lower and upper **Bound**. This means that **Range** strings with more than one upper/lower bound have redundant **Comparators**. This is not treated as an error and the intersection is calculated.

### Configuring Objects without Parsing

For performance you may choose to rather edit objects directly, rather than parsing strings. 

#### Setting **Version** object values (mutate):

```cpp
SEMVER_API SemverParseResult semver_set_version_major(HSemverVersion version, uint64_t major);

SEMVER_API SemverParseResult semver_set_version_minor(HSemverVersion version, uint64_t minor);

SEMVER_API SemverParseResult semver_set_version_patch(HSemverVersion version, uint64_t patch);

SEMVER_API SemverParseResult semver_set_version_prerelease(HSemverVersion version, const char* prerelease); 

SEMVER_API SemverParseResult semver_set_version_build(HSemverVersion version, const char* build);

SEMVER_API SemverParseResult semver_set_version_core_triplet(HSemverVersion version, uint64_t major, uint64_t minor, uint64_t patch);

SEMVER_API SemverParseResult semver_set_version_values(HSemverVersion version, uint64_t major, uint64_t minor, uint64_t patch, const char* prerelease, const char* build);
```

#### Building Ranges

```cpp
SEMVER_API HSemverRange semver_query_add_range(HSemverQuery query);

SEMVER_API void semver_query_erase_range_at_index(HSemverQuery query, size_t index); //also disposes the range

SEMVER_API SemverParseResult semver_range_set_min_prerelease(HSemverRange range, const char* prerelease);

SEMVER_API void semver_range_set_to_all(HSemverRange range);

SEMVER_API void semver_range_set_to_none(HSemverRange range);

SEMVER_API void semver_bound_set_is_inclusive(HSemverBound bound, bool inclusive);

SEMVER_API void semver_bound_set_juncture(HSemverBound bound, HSemverVersion juncture);

SEMVER_API void semver_bound_set_to_min(HSemverBound bound);

SEMVER_API void semver_bound_set_to_max(HSemverBound bound);

SEMVER_API SemverParseResult semver_set_juncture(HSemverVersion juncture, uint64_t major, uint64_t minor, uint64_t patch, const char* prerelease); //Junctures don't have build metadata
```
- Lower **Bound** has `>` or `>=` inequality
- Upper **Bound** has `<` or `<=` inequality
- "Inclusive" means the **Bound** inequality includes the **Juncture** : i.e., `>=` and `<=`, "exclusive" otherwise (`>` and `<`)
- The minimum **Bound** is `>=0.0.0-0`
- The maximum **Bound** is `<=9007199254740991.9007199254740991.9007199254740991` (implementation-specific)
- The "all" **Range** has the minimum lower **Bound** and maximum upper **Bound**
- The "none" **Range** is `>0.0.0-0 <0.0.0-0`, but exports as this string `<0.0.0-0`

#### Junctures 
- define **Bounds** within **Queries**,
- are specific points in Version ordering (equivalent to **Versions** without build metadata), and
- have the same `HSemverVersion` handle as **Versions**.  


Setting build metadata on a **Juncture** is not defined in the [Semantic Version Query Language Specification](Semantic%20Version%20Query%20Language%20Specification.md)], but this API allows it.


### Reading Components

Get **Version** parts:

```cpp
SEMVER_API uint64_t semver_get_version_major(const HSemverVersion version);

SEMVER_API uint64_t semver_get_version_minor(const HSemverVersion version);

SEMVER_API uint64_t semver_get_version_patch(const HSemverVersion version);

SEMVER_API const char* semver_get_version_prerelease(const HSemverVersion version);

SEMVER_API const char* semver_get_version_build(const HSemverVersion version);
```

- **Core Triplet** parts default to `0xDEAD'DEAD'DEAD'DEAD` (16,045,725,885,737,590,445) on failure (implementation-specific).
- Strings are owned by the **Version**, freed on dispose of the object. Invalid parts return `?`.
- Example: `2.3.01` yields `PATCH_LEADING_ZERO`, string `2.3.?`.


Get **Version Array** elements:
``` c++
SEMVER_API size_t semver_versions_count(HSemverVersions version_array);

SEMVER_API HSemverVersion semver_versions_get_version_at_index(HSemverVersions version_array, size_t index);
```

Get **Query** parts:

``` c++
SEMVER_API size_t semver_query_get_range_count(const HSemverQuery query);

SEMVER_API HSemverRange semver_query_get_range_at_index(const HSemverQuery query, size_t index);

SEMVER_API HSemverBound semver_range_get_lower_bound(const HSemverRange range);

SEMVER_API HSemverBound semver_range_get_upper_bound(const HSemverRange range);

SEMVER_API const char* semver_range_get_min_prerelease(const HSemverRange range);

SEMVER_API BOOL semver_bound_get_is_inclusive(const HSemverBound bound);

SEMVER_API HSemverVersion semver_bound_get_juncture(const HSemverBound bound);
```

**Juncture** is a specific point in **Version** ordering. Defined in the [Semantic Version Query Language Specification](Semantic%20Version%20Query%20Language%20Specification.md) as a **Version** without build metadata.

We can generate strings from **Version** and **Query** object that would produce equivalent objects if parsed.

``` c++
SEMVER_API const char* semver_get_version_string(const HSemverVersion version);

SEMVER_API const char* semver_get_query_string(const HSemverQuery query);
```

These strings must be disposed using 
``` c++
SEMVER_API void semver_free_string(char* str);
```

Note: that build and prerelease strings returned for **Version** objects are owned by the object and must not be disposed. They are disposed with the object.

### Comparing Versions

Compare **Versions**:

```c++
SEMVER_API int semver_compare(const HSemverVersion lhs, const HSemverVersion rhs);
```

Returns:
- Negative: `lhs < rhs`
- Positive: `lhs > rhs`
- Zero: `lhs == rhs`

Convenience helpers are included in the header file:

```cpp
inline BOOL semver_version_is_equal(const HSemverVersion lhs, const HSemverVersion rhs);

inline BOOL semver_version_is_greater(const HSemverVersion lhs, const HSemverVersion rhs);

inline BOOL semver_version_is_less(const HSemverVersion lhs, const HSemverVersion rhs);

inline BOOL semver_version_is_greater_or_equal(const HSemverVersion lhs, const HSemverVersion rhs);

inline BOOL semver_version_is_less_or_equal(const HSemverVersion lhs, const HSemverVersion rhs);
```

**Pre-release Versions** sort before **Stable Versions**; **Build Metadata** is ignored.

### Matching Versions

Check if a **Version** satisfies a **Query**
```cpp
SEMVER_API BOOL semver_query_matches_version(const HSemverQuery query, const HSemverVersion version);
```

A **Version** satisfies a **Query** if it matches any of its **Ranges**, meaning it falls within the **Bounds** of the **Range** with special rules for [Pre-release Versions](#pre-releases).

### Querying a Version Array
To get the subset of a **Version Array** that satisifes a **Query**:

```cpp
SEMVER_API HSemverVersions semver_query_match_versions(const HSemverQuery query, const HSemverVersions versions);
```

You don't need to dispose the returned **Version Array**, it will be disposed with the original **Version Array** you constructed. You *may* dispose it early if you don't need it any more. 

### Example Workflow

Parse and check a **Version** against a **Query**:

```cpp
HSemverVersion version = semver_version_create();
semver_version_parse(version, "1.2.3-beta");

HSemverQuery query = semver_query_create();
semver_query_parse(query, ">=1.0.0 @beta");

if (semver_query_matches_version(query, version)) 
	printf("Version 1.2.3-beta satisfies range >=1.0.0 @beta\n");

semver_version_dispose(version);
semver_query_dispose(query);
```

or for bulk processing

``` c++
HSemverQuery query = semver_query_create();
semver_query_parse(query, ">=1.2.3 @beta");

const char* versions_str = "1.2.2, 1.2.3-alpha, 1.2.3, 1.2.4-alpha, 1.2.3-beta, 1.2.4";

HSemverVersions versions = semver_versions_from_string(versions_str, ", ", SEMVER_ORDER_ASC); // split and sort using Semver 2.0.0 precedence (1.2.3-beta moves to just after 1.2.3-alpha)

HSemverVersions matches = semver_query_match_versions(query, versions); // I can but don't have to dispose matches

size_t match_count = semver_versions_count(matches);
for (size_t i = 0; i < match_count; ++i)
{
   HSemverVersion version = semver_versions_get_version_at_index(matches, i);
// actions for each of the version objects
}

semver_query_dispose(query);
```


## Semantic Versioning

Per [Semantic Versioning 2.0.0](https://semver.org):
- **Core Triplet** (`MAJOR.MINOR.PATCH`): Non-negative integers, no leading zeros (except when a single `0`).
- **Pre-release Label**: `-` prefix (e.g., `1.2.3-alpha`), sorts before **Stable Version**.
- **Build Metadata**: `+` prefix (e.g., `1.2.3+build123`), ignored in comparisons and queries.

See the [Semantic Version Query Language Specification](Semantic%20Version%20Query%20Language%20Specification.md) for **Query** details.

### Implementation-Specific Restrictions

- **Core Triplet** values capped at `2^53-1` (9,007,199,254,740,991) for compatibility with double-precision floating-point (not in Semantic Versioning 2.0.0).
- Failed parses default to `0xDEAD'DEAD'DEAD'DEAD` (core) or `?` (pre-release/build).

## Range Syntax and Terminology

Per the [Semantic Version Query Language Specification](Semantic%20Version%20Query%20Language%20Specification.md):
- **Version Constraint**: **Comparator** (e.g., `>=1.2.3`), **Tilde Constraint** (`~1.2.3`), **Caret Constraint** (`^1.2.3`), or **Hyphen Constraint** (`1.2.3 - 4.5.6`).
- **Range**: **Version Constraints** joined by whitespace with an optional **Pre-release Extension** (e.g., `>=1.2.3 @rc`).
- **Query**: **Ranges** joined by `||` (e.g., `>=1.2.3 <1.3.0 @rc || >=2.0.0`).
- **Juncture**: A point in version ordering (e.g., `1.2.3`, `1.2.3-alpha`). 
- **Pre-release Extension**: `@<min-prerelease-label>` (e.g., `@beta`).
- **Satisfies/Matches**: A **Version** satisfies a **Query** if any of its **Ranges** match the **Version**. A **Range** matches a **Version** if it is within bounds and meet pre-release requirements.

Operators: `>`, `>=`, `<`, `<=`, `=`, `~`, `^`, wildcards (`x`, `X`, `*`).

## Pre-releases

**Pre-release Versions** (e.g., `1.2.3-alpha`) satisfy a **Range** if:
1. Within the **Range**’s bounds, and
2. One of:
   - The **Range** has a **Pre-release Extension** (`@<min-prerelease-label>`), and the Version's **Pre-release Label** is >= `<min-prerelease-label>` (e.g., `>=1.2.3 @beta` matches `1.2.4-beta`, `5.6.7-rc`).
   - A **Bound** matches the **Core Triplet** and includes a **Pre-release Label** (e.g., `>=1.2.3-alpha` matches `1.2.3-alpha`, `1.2.3-beta`, `1.2.3`, but not `1.2.4-alpha`).

### Pre-release Extensions

- Don't alter the range. This means `>=1.2.3 @alpha` does not match `1.2.3-alpha` because it is < `1.2.3`. 
- Mathes **Pre-release Versions** within the range that have a **Pre-release Label** greater than or equal to the minimum pre-release specified.
- Have no affect on **Stable Versions** which statisfies a **Range** (with or witha **Pre-release Extention**) if it falls within its **Bounds** 

Syntax:
- `@<min-prerelease-label>`: Matches pre-releases `>= <min-prerelease-label>` (e.g., `@beta`: `beta`, `rc`, not `alpha`). 
- `<min-prerelease-label>` must be valid per [Semantic Versioning 2.0.0](https://semver.org/spec/v2.0.0.html) (e.g., `@01`, `@!` fail with `SEMVER_QUERY_PARSE_MIN_PRERELEASE_LEADING_ZERO_ON_NUMERIC_IDENTIFIER` and `SEMVER_QUERY_PARSE_MIN_PRERELEASE_UNSUPPORTED_CHARACTER`, respectively).
- `@0`: Matches any pre-release (e.g., `>=1.2.3 @0` matches all pre-release **Versions** after `1.2.3`).

Examples:
- `>=1.2.3 @beta`: Matches  `1.2.4-rc`, `1.2.3`, not `1.2.3-beta`, `1.2.4-alpha`.
- `>=1.2.3-alpha <1.2.4`: Matches `1.2.3-alpha`, `1.2.3-beta`, `1.2.3`.
- `1.2.3 - 2.0.0  @0`: Matches  `1.2.3`, `2.0.0-beta`, `2.0.0`.
- `>1.2.3-rc.2`: Matches `1.2.4`, not `1.2.3-rc`.

**Ranges** without **Pre-release Extensions** or pre-release **Bounds** exclude pre-releases:
- `>=1.2.3 <1.3.0`: Matches `1.2.3`, `1.2.4`, not `1.2.3-alpha`.

## Specifying Range Sets in Queries

A **Query** is a **Range Set** which is **Ranges** joined by `||` (union). A **Range** is **Version Constraints** with an optional **Pre-release Extension**. An empty **Range Set** matches all **Stable Versions**.

### Comparators

Operators with a **Version Pattern** (**Juncture**, **Partial Version**, **Wildcard Version**):
- `>3.2.1`: `>3.2.1 <=MAX.MAX.MAX`. (`MAX` is just illustrative and not part of syntax)
- `<=4.5.2`: `>=0.0.0 <=4.5.2`.
- `>=1.2.3 @beta`: `>=1.2.3 <MAX.MAX.MAX` (pre-releases >= `beta`).
- `>=1.2.3-0`: `>=1.2.3-0 <=MAX.MAX.MAX` (includes all `1.2.3` pre-releases).

For brevity we may exlcude the implicit maximum or minimum bounds in examples

**Partial Versions**:
- `>=2`: `>=2.0.0`.
- `>=2.3`: `>=2.3.0`. 
- `>2`: `>=3.0.0-0`. 
- `>2.3`: `>=2.4.0-0`
- `<=3`: `<4.0.0-0`. 
- `<=4.5`: `<4.6.0-0`.
- `<3`: `<3.0.0`.
- `<3.7`: `<3.7.0`.
- `2`: `>=2.0.0 <3.0.0`.
- `2 @beta`: `>=2.0.0 <3.0.0` (pre-releases >= `beta`).

Please take note of how `>` and `<=` on partial versions are interpreted. `>` is interpeted as starting at the next version's lowest pre-release and `<=` as below the next version's pre-release.

**Intersection** (space-separated):
- `>=1.2.3 <2.0.0`: `1.2.3` to just below `2.0.0`.

**Union** (`||`):
- `>=1.2.3 <1.3.0 @rc|| >=2.0.0`: Matches  `1.2.5-test`, `1.2.555`, `2.0.0`, `999.999.999`, but not `3.0.0-rc`

### The Full Range

Empty string or wildcard (`x`, `X`, `*`):
- `*`: `>=0.0.0-0 <=MAX.MAX.MAX` (all **Stable Versions**).
- `* @beta`: `>=0.0.0-0 <=MAX.MAX.MAX` (pre-releases `>=beta`).
- `* @0`: `>=0.0.0-0 <=MAX.MAX.MAX` (all pre-releases).

### The Empty Range

No-match ranges (returned as `<0.0.0-0`, implementation-specific):
- `<0.0.0-0`. 
- `>1.2.3 <1.2.3`.
- `>=3.2.2 <=3.2.1`.
- `>3.0.0 <=2.0.0`.

Invalid **Ranges** default to `<0.0.0-0` (implementation-specific).

### Exact Version Matching

A wildcard or partial **Version** with no operator implies a range of versions and not an exact match. Exact matches only happen when the **Version Pattern** is a **Juncture** (single point in **Version** ordering)

No operator or equal operator:
- `2.3.4`: Only `2.3.4` (the lower and upper **Bound** of the range will have the same **Juncture** and both marked as inclusive)

Note: adding a **Pre-release Extension** (`@<min-prerelease-label>`) with an exact match has no effect.
- `2.3.4 @alpha` matches only `2.3.4`


### Bounded Ranges (Hyphen Constraints)

Inclusive:
- `1.2.3 - 4.5.6`:= `>=1.2.3 <=4.5.6`.
- `1 - 2.3` := `1.0.0 - 2.3.0`: `>=1.0.0 <=2.3.0`.
- `1.2.3 - 4.5.6 @0` := `>=1.2.3-0 <=4.5.6 @0` (all pre-releases between `1.2.3` and `4.5.6`).
- `1.2.3 - 4.5.6 @beta` := `>=1.2.3-beta <=4.5.6 @beta` (pre-releases `>=beta` between `1.2.3` and `4.5.6`).
- `1.2.3-rc - 4.5.6 @alpha` (pre-releases `>=alpha` between `1.2.3-rc` and `4.5.6`).

### Tilde Constraints

Patch/minor updates:
- `~1.2.3`: `>=1.2.3 <1.3.0`.
- `~1.2`: `>=1.2.0 <1.3.0`.
- `~1`: `>=1.0.0 <2.0.0`.
- `~0`: `>=0.0.0 <1.0.0`.
- `~1.2.3 @rc`: `>=1.2.3 <1.3.0 @rc`.

### Caret Constraints

Updates to first non-zero part:
- `^1.2.3`: `>=1.2.3 <2.0.0`.
- `^0.2.4`: `>=0.2.4 <0.3.0`.
- `^0.0.5`: `>=0.0.5 <0.0.6`.
- `^1.2.3 @beta`: `>=1.2.3 <2.0.0 @beta`.

### Wildcards

Minor/patch wildcards:
- `2.x`: `>=2.0.0 <3.0.0` 
- `3.1.x`: `>=3.1.0 <3.2.0-0`.
- `2.x @beta`: `>=2.0.0 <3.0.0-0` (pre-releases `>=beta`).

Avoid wildcards with `~`/`^` since the tilde or caret will be redundant (e.g., `~0.2.x` := `0.2.x`).

### Builds

**Build Metadata** (e.g., `1.2.3+build123`) is informational and ignored in comparisons and ranges. The [Semantic Version Query Language Specification](Semantic%20Version%20Query%20Language%20Specification.md) defines a **Juncture** as a single point in version ranges which only includes the **Core Triplet** and **Pre-release Label**. Our API will not error for **Build Metadata** if added to a **Juncture**, it will simply be ignored in queries.

## License

The source code is proprietary and not licensed for use, modification, or distribution. It is publicly viewable for reference only. See the `LICENSE` file for details.

## FAQ

### Why did you make separate types for Versions and Junctures?

See [Semantic Version Query Language Specification](Semantic%20Version%20Query%20Language%20Specification.md#Questions) 

### Why is the license not open-source?

The project is proprietary to retain control during early development.

### Will this ever become open-source?

The intention is to open-source the project once it’s more polished, with a license to be determined.

### Can I use it in my project?

The project is not yet licensed for use. Contact the maintainer for inquiries about usage during this pre-release phase.

Copyright 2025 Jasper Schellingerhout. All rights reserved.