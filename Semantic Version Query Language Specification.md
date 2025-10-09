# Semantic Version Query Language Specification

Copyright 2025 Jasper Schellingerhout. All rights reserved.

## Summary

This specification defines a query language for matching **Versions** against ranges, compliant with Semantic Versioning 2.0.0. It focuses on range syntax and matching rules.

## Terms

 1. **Semantic Version Query**:
      - Accepts a **Range Set**
      - Matches zero or more **Versions**.
      - Returns information related to the matched **Versions**.
 2. **Version**: A string per Semantic Versioning 2.0.0, with:
      - **Core Triplet**: Three non-negative integers separated by `.` (`MAJOR.MINOR.PATCH`, e.g., `1.2.3`).
      - Optional **Pre-release Label**: A hyphen (`-`) after `PATCH`, followed by dot-separated identifiers (e.g., `beta.2` in `1.2.3-beta.2`).
      - Optional **Build Metadata**: A plus (`+`) after `PATCH` or **Pre-release Label**, followed by dot-separated identifiers (e.g., `build123` in `1.2.3+build123`). Ignored in ordering or matching.
 3. **Pre-release Label**: Dot-separated **Pre-release Identifiers** determining pre-release ordering per SemVer 2.0.0 precedence rules.
 4. **Stable Version**: A **Version** without a **Pre-release Label**.
 5. **Pre-release Version**: A **Version** with a **Pre-release Label**.
 6. **Version Pattern**: A **Juncture**, **Partial Version**, or **Wildcard Version** used in queries to match **Versions**.
 7. **Juncture**: A specific point in **Version** ordering. A **Version** without **Build Metadata** (e.g., `1.2.3`, `1.2.3-alpha`).
 0. **Partial Version**: `MAJOR` or `MAJOR.MINOR`, not a valid **Version** (e.g., `1`, `1.2`).
11. **Wildcard Version**: A **Partial Version** followed by `.` and **Wildcard**, or a lone **Wildcard** (e.g., `1.2.x`, `*`).
12. **Wildcard**: A token (`x`, `X`, `*`) denoting any non-negative integer in a **Wildcard Version**.
13. **Comparator**: A comparison operator (`>`, `>=`, `<`, `<=`, `=`) with a **Version Pattern** (e.g., `>=1.2.x`, `>1.2.3-alpha.3`). If no operator, `=` is assumed (e.g., `1.2.3` is `=1.2.3`).
14. **Bound**: Restricts a set of **Versions** to those
      - `>` the *highest* value represented by the **Version Pattern**: **Exclusive Lower Bound**,
      - `>=` the *lowest* value represented by the **Version Pattern**: **Inclusive Lower Bound**,
      - `<` the *lowest* value represented by the **Version Pattern**: **Exclusive Upper Bound**, or
      - `<=` the *highest* value represented by the **Version Pattern**: **Inclusive Upper Bound**
15. **Explicit Bound**: Directly specified by an inequality **Comparator**:
16. **Implicit Bound**: Implied when a bound is omitted:
      - **Lowest Bound**: `>=0.0.0`.
      - **Highest Bound**: `<=MAX.MAX.MAX`, where `MAX` is an undefined maximum value for a **Core Triplet** component.
15. **Version Constraint**:
      - **Comparator**: Defines a single **Explicit Bound** (e.g., `>=1.2.3`), but also has an **Implicit Bound**: 
        - `<=MAX.MAX.MAX` with **Explicit Lower Bound**, 
        - `>=0.0.0` with **Explicit Upper Bound**.
      - **Tilde Constraint**: `~` with a **Version Pattern**, defines **Bounds** allowing `PATCH` updates (`MINOR` for **Partial Versions** with only `MAJOR`).
      - **Caret Constraint**: `^` with a **Version Pattern**, defines **Bounds** allowing changes that do not modify the left-most non-zero **Core Triplet** component.
      - **Hyphen Constraint**: Two **Version Patterns** separated by `-`, defines **Inclusive Bounds**.
16. **Range**: Zero or more **Version Constraints**, optionally followed by one **Pre-release Extension**. An empty **Range** matches all **Stable Versions**.
17. **Pre-release Extension**: `@<min-prerelease-label>`, extending a **Range** to also match **Pre-release Versions** with **Pre-release Labels** `>= <min-prerelease-label>` (per SemVer 2.0.0 precedence).
18. **Full Range**: All **Stable Versions** (e.g., `>=0.0.0`).
19. **Match**: A **Version** falls within a **Range**’s bounds and satisfies its **Pre-release Extension** (if any).
20. **Range Set**: Zero or more **Ranges** joined by `||` (union), or an empty string denoting the **Full Range**.
21. **Satisfy**: A **Version** satisfies a **Query** if it matches any of its **Ranges**, or the **Range Set** is empty (matching all **Stable Versions**).

## Version Comparison

Version precedence follows Semantic Versioning 2.0.0:

- **Pre-release Versions** have lower precedence than **Stable Versions** with the same **Core Triplet** (e.g., `1.2.3-beta < 1.2.3`).
- **Build Metadata** is ignored in ordering or matching.
- Operators: `<` (lower), `>` (higher), `=` (equal), `<=` (equal or lower), `>=` (equal or higher).
- **Lowest Version**: `0.0.0-0`.
   - `<0.0.0-0`: Empty set
   - `>=0.0.0-0`: **Full Range** (all **Stable Versions**).
   - `>=0.0.0 @0` := `>=0.0.0-0 @0`: **Full Range** (all **Versions**).
- **Highest Version**: `MAX.MAX.MAX`, an undefined upper bound for **Core Triplet**. Inequalities are adjusted to avoid defining `MAX` (e.g., `<=2.3.MAX` := `<2.4.0-0`).
- **Lowest Pre-release Label**: `0`.
- **Highest Pre-release Label**: `(MAX)` Theoretical infinite series of `z`. Inequalities are adjusted to avoid definition (e.g., `<=2.3.1-(MAX)` := `<2.3.2-0`). `(MAX)` is not a valid **Pre-release Identifier** because `(` and `)` are not allowed.


## Query Syntax

```
range-set ::= range ( logical-or range ) * | ''
logical-or ::= ( ' ' ) * '||' ( ' ' ) *
range ::= ( hyphen-constraint | version-constraint ( ' ' version-constraint ) *) prerelease-extension ? | ''
prerelease-extension ::= ( ' ' ) + '@' prerelease-label
version-constraint ::= comparator | tilde-constraint | caret-constraint
comparator ::= ( '<' | '>' | '>=' | '<=' | '=' ) version-pattern
tilde-constraint ::= '~' version-pattern
caret-constraint ::= '^' version-pattern
hyphen-constraint ::= version-pattern ( ' ' ) * '-' ( ' ' ) * version-pattern
version-pattern ::= juncture | partial-version | wildcard-version
juncture ::= number '.' number '.' number ( '-' prerelease-label ) ?
partial-version ::= number ( '.' number ) ?
wildcard-version ::= wildcard | partial-version '.' wildcard
prerelease-label ::= prerelease-identifier ( '.' prerelease-identifier )*
prerelease-identifier ::= non-number-identifier | number
non-number-identifier ::= ( identifier-character ) * non-digit ( identifier-character ) *
identifier-character ::= digit | non-digit
non-digit ::= alpha | '-'
wildcard ::= 'x' | 'X' | '*'
number ::= '0' | positive-digit ( digit ) *
positive-digit ::= ['1'-'9']
digits ::= digit ( digit )*
digit ::= ['0'-'9']
alpha ::= ['A'-'Z'] | ['a'-'z']
```

## Matching Versions

### Summary

A **Stable Version** satisfies a **Range** if it falls within its **Bounds**. A **Pre-release Version** must also:

- Have a **Pre-release Label** `>=` the **Pre-release Extension**’s `<min-prerelease-label>` (if present).
- Or have the same **Core Triplet** as a **Bound** with a **Pre-release Version** (e.g., `>=1.2.3-alpha` will match `1.2.3-beta`, but not `1.2.4-alpha`).

### Components

1. **Wildcard Version**:
   - Lowest value: Wildcards as `0` (e.g., `>=1.2.x` := `>=1.2.0`).
   - Highest value: Wildcards as `MAX`, comparators are changed to avoid defining `MAX` (e.g., `<=1.2.x` := `<=1.2.MAX` := `<1.3.0-0`).
2. **Juncture**:  
   Represents a single comparable value.
3. **Comparators**: Constrain the set of matched **Versions** to
   - **Explicit Bounds**:
     - **Exclusive Lower Bound**: `>` the *highest* value represented by the **Version Pattern** (e.g., `>2.4` := `>2.4.MAX` := `>=2.5.0`).
     - **Inclusive Lower Bound**: `>=` the *lowest* value represented by the **Version Pattern** (e.g., `>=2.4` := `>=2.4.0`).
     - **Exclusive Upper Bound**: `<` the *lowest* value represented by the **Version Pattern** (e.g., `<2.4` := `<2.4.0`).
     - **Inclusive Upper Bound**: `<=` the *highest* value represented by the **Version Pattern** (e.g., `<=2.4` := `<=2.4.MAX` := `<2.5.0-0`).
     - **Equality**: matching the pattern. The intersection of the **Inclusive Lower Bound** and **Inclusive Upper Bound** for the **Version Pattern** (e.g., `=2` := `>=2.0.0 <=2.MAX.MAX` := `>=2.0.0 <3.0.0-0`, `=2.3.4` := `>=2.3.4 <=2.3.4`).
   - **Implicit Bounds**:
     - Lower-only **Comparator**: `<=MAX.MAX.MAX`.
     - Upper-only **Comparator**: `>=0.0.0`.
4. **Tilde Constraint**:
   - **Inclusive Lower Bound**: **Version Pattern** as specified.
   - **Inclusive Upper Bound**: **Version Pattern** with `PATCH` as `MAX` (e.g., `~1.2.3` := `>=1.2.3 <=1.2.MAX` := `>=1.2.3 <1.3.0-0`).
      - **Partial Version** with only `MAJOR`: `MINOR` as `MAX` (e.g., `~2` := `>=2.0.0 <=2.MAX.MAX` := `>=2.0.0 <3.0.0-0`).
5. **Caret Constraint**:
   - **Inclusive Lower Bound**: **Version Pattern** as specified.
   - **Inclusive Upper Bound**: **Version Pattern** with values to the right of the first non-zero **Core Triplet** component from left as `MAX`:
     - `^2.3.4` := `>=2.3.4 <=2.MAX.MAX` := ` >=2.3.4 <3.0.0-0`.
     - `^0.7.2` := `>=0.7.2 <=0.7.MAX` := `>=0.7.2 <0.8.0-0`.
     - `^0.0.3` := `>=0.0.3 <=0.0.3-(MAX)` := `>=0.0.3 <0.0.4-0`.
6. **Hyphen Constraint**:
   - **Inclusive Lower Bound**: First **Version Pattern** with wildcards as `0`.
   - **Inclusive Upper Bound**: Second **Version Pattern** with wildcards as `0` (e.g., `1.2 - 2.0` := `>=1.2.0 <=2.0.0`).
   - Example: `1.2.3 - 1.3.0` := `>=1.2.3 <=1.3.0`.
7. **Pre-release Extension**:
   - Matches **Stable Versions** or **Pre-release Versions** with **Pre-release Labels** `>= <min-prerelease-label>` within bounds (e.g., `>=1.2.3 <1.3.0 @beta` matches `1.2.3`, `1.2.4-beta`, `1.3.0-rc`, but not `1.2.5-alpha` or `1.2.3-beta`).
8. **Range**: Matches **Versions** satisfying all **Version Constraints** and **Pre-release Extension**. An empty **Range** matches all **Stable Versions**.
9. **Range Set**: Satisfied if any **Range** matches, or empty (matches all **Stable Versions**).

## Examples

Below are examples demonstrating range matching for this specification, covering standard constraints, **Pre-release Extension**, **Sub-Version Juncture**, and edge cases to clarify behavior.

 1. **Tilde Constraint**:

    - Range: `~1.2.3`
    - Versions: `[1.2.2, 1.2.3, 1.2.4, 1.2.3-alpha, 1.3.0]`
    - Matches: `1.2.3`, `1.2.4` (`>=1.2.3 <=1.2.MAX` := `>=1.2.3 <1.3.0-0`).
    - Does Not Match: `1.2.2` (`< 1.2.3`), `1.2.3-alpha` (pre-release excluded), `1.3.0` (`>= 1.3.0-0`).

 2. **Caret Constraint**:

    - Range: `^0.7.2`
    - Versions: `[0.7.1, 0.7.2, 0.7.3, 0.8.0, 0.7.2-beta]`
    - Matches: `0.7.2`, `0.7.3` (`>=0.7.2 <=0.7.MAX` := `<0.8.0-0`).
    - Does Not Match: `0.7.1` (`< 0.7.2`), `0.7.2-beta` (pre-release excluded), `0.8.0` (`>= 0.8.0-0`).

 3. **Hyphen Constraint**:

    - Range: `1.2.3 - 1.2.5`
    - Versions: `[1.2.2, 1.2.3, 1.2.4, 1.2.5, 1.2.6, 1.2.3-alpha]`
    - Matches: `1.2.3`, `1.2.4`, `1.2.5` (`>=1.2.3 <=1.2.5`).
    - Does Not Match: `1.2.2` (`< 1.2.3`), `1.2.6` (`> 1.2.5`), `1.2.3-alpha` (pre-release excluded).

 4. **Wildcard Version**:

    - Range: `*`
    - Versions: `[0.0.0, 1.0.0, 2.0.0-alpha, 999.999.999]`
    - Matches: `0.0.0`, `1.0.0`, `999.999.999`.
    - Does Not Match: `2.0.0-alpha` (pre-release excluded).

 5. **Pre-release Extension**:

    - Range: `>=1.2.3 <1.3.0 @rc`
    - Versions: `[1.2.3-alpha, 1.2.3-rc.1, 1.2.3, 1.2.4-beta, 1.2.4, 1.2.5-rc, 1.3.0]`
    - Matches: `1.2.3`, `1.2.4`, `1.2.5-rc`
    - Does Not Match: `1.2.3-alpha`, `1.2.3-rc.1` (< `1.2.3`), `1.2.4-beta` (`beta` < `rc`), `1.3.0` (`>= 1.3.0`).

 6. **Hyphen Constraint with Pre-release Extension**:

    - Range: `1.2.3 - 1.2.5 @beta`
    - Versions: `[1.2.3-alpha, 1.2.3-beta, 1.2.3, 1.2.4-rc, 1.2.4, 1.2.5-alpha, 1.2.5]`
    - Matches: `1.2.3`, `1.2.4`, `1.2.5` (`>=1.2.3 <=1.2.5`), `1.2.4-rc` (`rc` >= `beta`)
    - Does Not Match:  `1.2.3-alpha`, `1.2.3-beta` (< `1.2.3`), `1.2.5-alpha` (`alpha` < `beta`>).

 7. **Pre-Release Juncture**:

    - Range: `>1.2.3-alpha`
    - Versions: `[1.2.2, 1.2.3-alpha, 1.2.3-beta, 1.2.3, 1.2.4]`
    - Matches: `1.2.3`, `1.2.4` (`>1.2.3-alpha`), `1.2.3-beta` (> `1.2.3-alpha` and same core-tripet as `1.2.3-alpha`)
    - Does Not Match: `1.2.2`, `1.2.3-alpha` (`<= 1.2.3-alpha`)

 8. **Range Set with Union**:

    - Range Set: `1.0.0 || 2.0.0 - 2.1.0 @alpha`
    - Versions: `[1.0.0-alpha, 1.0.0, 1.0.1, 2.0.0-alpha, 2.0.0, 2.0.1, 2.1.0, 2.1.1]`
    - Matches: `1.0.0`, , `2.0.1`, `2.1.0` (`=1.0.0` or `>=2.0.0-alpha <=2.1.0`).
    - Does Not Match:  `1.0.0-alpha`, `1.0.1`, `2.0.0-alpha`  (`≠ 1.0.0` and `<2.0.0`), `2.1.1` (`≠ 1.0.0` and `> 2.1.0`).

 9. **Empty Query**:

    - Range: (empty)
    - Versions: `[0.0.0, 1.2.3, 1.2.3-alpha, 999.999.999]`
    - Matches: `0.0.0`, `1.2.3`, `999.999.999` (all **Stable Versions**).
    - Does Not Match: `1.2.3-alpha` (pre-release excluded).


## Query Results

Returns the matched **Versions** (e.g., `1.2.3`, `1.2.4-beta`) and/or related information (e.g., artifact IDs, resource locations, API definitions, licenses, certificates, source code, or binaries), as defined by the implementation. In many cases, users query solely for the list of **Versions** that satisfy the **Query**, with the version strings themselves (e.g., `["1.2.3", "1.2.4"]`) being the primary or only results.

Implementations may choose to order the results as appropriate and limit the number of results. For instance: restricting the return to the single highest matched version.

## Questions

### What is the difference between a Query and a Range Set? ###

For this version of the specification a **Query** is just a **Range Set**. Future versions may be extended to specify a source, ordering, data aggregation, subset and the selection of fields returned. Today those are assumed to be known by the application receiving the query.

### What is the difference between a Version and a Juncture? ###

A **Juncture** is a specfic point in **Version** ordering.
For this version of the specification a **Juncture** is a **Version** string without **Build Metadata**.

I chose this name because a **Query** matches **Versions** using **Version Patterns** and I needed a name for the patterns corresponding to single comparison point. I considered **Version Point** and **Version Juncture**, but settled on **Juncture**.

An eary draft of this specification expanded the concept of **Junctures** to **Sub-Version Junctures** where **Versions** sort lower or higher than the **Juncture**, but never match it. Future versions of this specification may revisit that idea.


### Aren't **Partial Versions** and **Wildcard Versions** pretty much the same thing? Why have both?

Correct, they are practically the same. Values omitted by a **Partial Version** are implicitly **Wildcard Values**:

- `<2` := `<2.x.x`
- `>=2.3` := `>=2.3.x`
- `2` := `2.x.x`

Both are defined because of user preference and clarity, e.g., `2.*` conveys the meaning "any **Version** starting with 2" more clearly than just `2`. During matching, **Partial Versions** are resolved to **Wildcard Versions** (e.g., `2` becomes `>=2.0.0 <3.0.0-0`).


### Why does the Syntax only allow **Pre-release Extensions** on **Ranges** and not **Version Constraints**?

Because multiple **Pre-release Extensions** in a **Range** would always be redundant.

The intersection of the **Ranges** implicitly produced by **Version Constraints** (space separated) always reduce to a single upper and lower bound (i.e. a single implicit **Range**). Allowing **Pre-release Extensions** on individual **Version Constraints** would permit multiple extensions on that single implicit **Range**, which would be redundant because the most permissive (lowest) **Pre-release Extension** would apply to the entire range. 

For example:  `>=2.3.4 @alpha <2.5.6 @beta` (not valid syntax) is equivalent to `>=2.3.4 <2.5.6 @alpha` 

### Can I use `MAX` in **Version Constraints**?

No, `MAX` is not part of the input syntax. **Version Constraints** with `MAX` must be converted to equivalent syntax without `MAX`:

- `<2.3.MAX` (not valid syntax) := `<=2.4.0-0` (valid syntax).
- `>2.3.MAX` (not valid syntax) := `>2.4.0`.
- `<2.3.4-(MAX)` (not valid syntax) := `<2.3.5-0`.
- `>2.3.4-(MAX)` (not valid syntax) := `>=2.3.5`.

The theoretical highest version `MAX.MAX.MAX` is implicitly included if you only define an **Explicit Lower Bound**:

- `>2.3.4` := `>2.3.4 <=MAX.MAX.MAX`.

Similarly, `(MAX)`, the theoretical upper bound for **Pre-release Labels**, is not part of the syntax.

Future versions of this specification may choose to extend the query syntax to allow these.

### Can my implementations allow **Build Metadata** on **Junctures**?

Yes, implementations that extends this specification without altering **Version** matching behavior as described in this specification are acceptable. **Build Metadata** is informational only and ignored in queries. 

**Version Patterns** (which includes **Junctures**) exclude **Build Metadata** because they are patterns to match **Versions**, not **Versions** themselves. For example, an implementation allowing `=1.2.3+build123` as an extended **Juncture** should treat it the same as `1.2.3` for the purposes of matching **Versions**.


