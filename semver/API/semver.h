// Copyright 2025 Jasper Schellingerhout. All rights reserved.

#pragma once

#if defined(_WIN32) || defined(_WIN64)
  #ifdef SEMVER_EXPORTS
	#define SEMVER_API __declspec(dllexport)
  #else
	#define SEMVER_API __declspec(dllimport)
  #endif
#else // not Windows
  #ifdef SEMVER_EXPORTS
	#define SEMVER_API __attribute__((visibility("default")))
  #else
	#define SEMVER_API
  #endif
#endif


#include <stdint.h>
#include <stddef.h> //is this needed?

extern "C" // for rest of file
{

#ifndef FALSE
#define FALSE 0
#endif

#ifndef TRUE
#define TRUE 1
#endif

	typedef int BOOL; // to avoid including windows.h 


	static const uint64_t SEMVER_MAX_NUMERIC_IDENTIFIER = 0x001F'FFFF'FFFF'FFFF; // 2^53 - 1, the maximum value for a numeric identifier in Semver (maximum value for lossless representation of integers in double precision floating point variables)

	// values over SEMVER_MAX_NUMERIC_IDENTIFIER are reserved for 
	// special interpretations of the numeric identifier parts of the version
	static const uint64_t SEMVER_UNINITIALIZED_IDENTIFIER = 0xDEAD'DEAD'DEAD'DEAD; // (16 045 725 885 737 590 445) version numbers are initialized to this value. If parsing fails, core version parts will have this value if the parser did not reach them yet 
	static const uint64_t SEMVER_WILDCARD_IDENTIFIER = 0xFEED'FEED'FEED'FEED; // attempting to parse with a wildcard will return this value for major, minor or patch.

    // builds are pre-releases are "labels"
	static const char SEMVER_UNINITIALIZED_LABEL[] = "?";
	static const char SEMVER_EMPTY_LABEL[] = "";

	static const char SEMVER_LOWEST_PRERELEASE[] = "0";

	typedef struct SemverVersionImpl* HSemverVersion; // transparent handle for a version object
	
	typedef struct SemverVersionsImpl* HSemverVersions; // transparent proto handle for a Version block in or a subset or ordered set from that block


	typedef struct SemverQueryImpl* HSemverQuery; // transparent handle for a version query (a set of 1 or more ranges)
	typedef struct SemverRangeImpl* HSemverRange; // transparent handle for a range (a query has 1 or more ranges)
	typedef struct SemverBoundImpl* HSemverBound; // transparent handle for a bound (upper or lower bound in a range)


	enum SemverParseResult : uint8_t 
	{
		SEMVER_PARSE_SUCCESS,
		SEMVER_PARSE_LEADING_WHITESPACE,  // not a fatal error
		SEMVER_PARSE_TRAILING_WHITESPACE, // not a fatal error
		SEMVER_PARSE_EMPTY_VERSION_STRING,
		SEMVER_PARSE_TOO_FEW_PARTS,
		SEMVER_PARSE_TOO_MANY_PARTS,

		SEMVER_PARSE_MAJOR_EMPTY,
		SEMVER_PARSE_MAJOR_NOT_NUMERIC,
		SEMVER_PARSE_MAJOR_LEADING_ZERO,
		SEMVER_PARSE_MAJOR_WILDCARD, // X or x or * 
		SEMVER_PARSE_MAJOR_TOO_LARGE, // Major version cannot be larger than 2^53 - 1

		SEMVER_PARSE_MINOR_EMPTY,
		SEMVER_PARSE_MINOR_NOT_NUMERIC,
		SEMVER_PARSE_MINOR_LEADING_ZERO,
		SEMVER_PARSE_MINOR_WILDCARD, // X or x or * 
		SEMVER_PARSE_MINOR_TOO_LARGE, // Minor version cannot be larger than 2^53 - 1

		SEMVER_PARSE_PATCH_EMPTY,
		SEMVER_PARSE_PATCH_NOT_NUMERIC,
		SEMVER_PARSE_PATCH_LEADING_ZERO,
		SEMVER_PARSE_PATCH_WILDCARD, // X or x or * 
		SEMVER_PARSE_PATCH_TOO_LARGE, // Patch version cannot be larger than 2^53 - 1

		SEMVER_PARSE_PRERELEASE_EMPTY_IDENTIFIER,
		SEMVER_PARSE_PRERELEASE_UNSUPPORTED_CHARACTER,
		SEMVER_PARSE_PRERELEASE_LEADING_ZERO_ON_NUMERIC_IDENTIFIER,

		SEMVER_PARSE_BUILD_EMPTY_IDENTIFIER,
		SEMVER_PARSE_BUILD_UNSUPPORTED_CHARACTER,

		SEMVER_PARSE_ENUM_MAX = SEMVER_PARSE_BUILD_UNSUPPORTED_CHARACTER, //keep as highest
	};

	
	enum SemverQueryParseResult : uint8_t 
	{
		SEMVER_QUERY_PARSE_SUCCESS = 0,
		SEMVER_QUERY_PARSE_VERSION_TOO_MANY_PARTS = SEMVER_PARSE_TOO_MANY_PARTS,
		
		SEMVER_QUERY_PARSE_MAJOR_NOT_NUMERIC = SEMVER_PARSE_MAJOR_NOT_NUMERIC,
		SEMVER_QUERY_PARSE_MAJOR_LEADING_ZERO = SEMVER_PARSE_MAJOR_LEADING_ZERO,
		SEMVER_QUERY_PARSE_MAJOR_TOO_LARGE = SEMVER_PARSE_MAJOR_TOO_LARGE,

		SEMVER_QUERY_PARSE_MINOR_NOT_NUMERIC = SEMVER_PARSE_MINOR_NOT_NUMERIC,
		SEMVER_QUERY_PARSE_MINOR_LEADING_ZERO = SEMVER_PARSE_MINOR_LEADING_ZERO,
		SEMVER_QUERY_PARSE_MINOR_TOO_LARGE = SEMVER_PARSE_MINOR_TOO_LARGE, // Minor version cannot be larger than 2^53 - 1

		SEMVER_QUERY_PARSE_PATCH_EMPTY = SEMVER_PARSE_PATCH_EMPTY,
		SEMVER_QUERY_PARSE_PATCH_NOT_NUMERIC = SEMVER_PARSE_PATCH_NOT_NUMERIC,
		SEMVER_QUERY_PARSE_PATCH_LEADING_ZERO = SEMVER_PARSE_PATCH_LEADING_ZERO,
		SEMVER_QUERY_PARSE_PATCH_TOO_LARGE = SEMVER_PARSE_PATCH_TOO_LARGE, // Patch version cannot be larger than 2^53 - 1

		SEMVER_QUERY_PARSE_PRERELEASE_EMPTY_IDENTIFIER = SEMVER_PARSE_PRERELEASE_EMPTY_IDENTIFIER,
		SEMVER_QUERY_PARSE_PRERELEASE_UNSUPPORTED_CHARACTER = SEMVER_PARSE_PRERELEASE_UNSUPPORTED_CHARACTER,
		SEMVER_QUERY_PARSE_PRERELEASE_LEADING_ZERO_ON_NUMERIC_IDENTIFIER = SEMVER_PARSE_PRERELEASE_LEADING_ZERO_ON_NUMERIC_IDENTIFIER,

		// Min Prerelease does not involve version parsing
		SEMVER_QUERY_PARSE_MIN_PRERELEASE_EMPTY_IDENTIFIER = SEMVER_PARSE_ENUM_MAX + 1,
		SEMVER_QUERY_PARSE_MIN_PRERELEASE_UNSUPPORTED_CHARACTER,
		SEMVER_QUERY_PARSE_MIN_PRERELEASE_LEADING_ZERO_ON_NUMERIC_IDENTIFIER,
	};

	enum SemverOrder : uint8_t
	{
		SEMVER_ORDER_AS_GIVEN,
		SEMVER_ORDER_DESC,
		SEMVER_ORDER_ASC,
	};


	// version constructors
	///////////////////////

	SEMVER_API HSemverVersion semver_version_create_defined(uint64_t major, uint64_t minor, uint64_t patch, const char* prerelease, const char* build); // Create a new version object from a version string, returns an object regardless of error

	SEMVER_API HSemverVersion semver_version_create();

	// version parse
	////////////////

	SEMVER_API SemverParseResult semver_version_parse(HSemverVersion version, const char* version_str);

	// version array constructors
	/////////////////////////////

	SEMVER_API HSemverVersions semver_versions_from_string(const char* versions_str, const char* separator, SemverOrder order);

	SEMVER_API HSemverVersions semver_versions_create(size_t count);

	// query constructor
	////////////////////

	SEMVER_API HSemverQuery semver_query_create();

	// query parser
	///////////////

	SEMVER_API SemverQueryParseResult semver_query_parse(HSemverQuery query, const char* query_str);


	// destructors
	//////////////////////////////

	SEMVER_API void semver_version_dispose(HSemverVersion version); // Free the allocated version object

	SEMVER_API void semver_versions_dispose(HSemverVersions version_array);

	SEMVER_API void semver_query_dispose(HSemverQuery query);


	// version array info
	/////////////////////////////////////////////////////////////////
    
	SEMVER_API size_t semver_versions_count(HSemverVersions version_array);
	SEMVER_API HSemverVersion semver_versions_get_version_at_index(HSemverVersions version_array, size_t index);

	// version info
	///////////////

	SEMVER_API uint64_t semver_get_version_major(const HSemverVersion version);
	SEMVER_API uint64_t semver_get_version_minor(const HSemverVersion version);
	SEMVER_API uint64_t semver_get_version_patch(const HSemverVersion version);
	SEMVER_API const char* semver_get_version_prerelease(const HSemverVersion version); // no need to dispose char*, disposed by HSemverVersion in semver_version_dispose
	SEMVER_API const char* semver_get_version_build(const HSemverVersion version); // no need to dispose char*, disposed by HSemverVersion in semver_version_dispose

	SEMVER_API char* semver_get_version_string(const HSemverVersion version); // \0 terminated string, freed with semver_free_version_string

	// version/query string destructor
	////////////////////////////

	SEMVER_API void semver_free_string(char* str);


	// Query info
	/////////////
	
	SEMVER_API size_t semver_query_get_range_count(const HSemverQuery query);
	SEMVER_API HSemverRange semver_query_get_range_at_index(const HSemverQuery query, size_t index);
	SEMVER_API HSemverBound semver_range_get_lower_bound(const HSemverRange range);
	SEMVER_API HSemverBound semver_range_get_upper_bound(const HSemverRange range);
	SEMVER_API const char* semver_range_get_min_prerelease(const HSemverRange range);
	SEMVER_API BOOL semver_bound_get_is_inclusive(const HSemverBound bound);
	SEMVER_API HSemverVersion semver_bound_get_juncture(const HSemverBound bound);


	// version mutators
	///////////////////

	SEMVER_API SemverParseResult semver_set_version_major(HSemverVersion version, uint64_t major);
	SEMVER_API SemverParseResult semver_set_version_minor(HSemverVersion version, uint64_t minor);
	SEMVER_API SemverParseResult semver_set_version_patch(HSemverVersion version, uint64_t patch);
	SEMVER_API SemverParseResult semver_set_version_prerelease(HSemverVersion version, const char* prerelease); 
	SEMVER_API SemverParseResult semver_set_version_build(HSemverVersion version, const char* build);
	SEMVER_API SemverParseResult semver_set_version_core_triplet(HSemverVersion version, uint64_t major, uint64_t minor, uint64_t patch);
	SEMVER_API SemverParseResult semver_set_version_values(HSemverVersion version, uint64_t major, uint64_t minor, uint64_t patch, const char* prerelease, const char* build);


	// Query mutators
	/////////////////

	SEMVER_API HSemverRange semver_query_add_range(HSemverQuery query);

	SEMVER_API void semver_query_erase_range_at_index(HSemverQuery query, size_t index); //also disposes the range

	SEMVER_API SemverParseResult semver_range_set_min_prerelease(HSemverRange range, const char* prerelease);
	SEMVER_API void semver_range_set_to_all(HSemverRange range);
	SEMVER_API void semver_range_set_to_none(HSemverRange range);


	SEMVER_API void semver_bound_set_is_inclusive( HSemverBound bound, bool inclusive);
	SEMVER_API void semver_bound_set_juncture(HSemverBound bound, HSemverVersion juncture);
	SEMVER_API void semver_bound_set_to_min(HSemverBound bound);
	SEMVER_API void semver_bound_set_to_max(HSemverBound bound);

	SEMVER_API SemverParseResult semver_set_juncture(HSemverVersion juncture, uint64_t major, uint64_t minor, uint64_t patch, const char* prerelease); //Junctures don't have build metadata

	// version comparison
	/////////////////////

	SEMVER_API int semver_compare(const HSemverVersion lhs, const HSemverVersion rhs);

	inline BOOL semver_version_is_equal(const HSemverVersion lhs, const HSemverVersion rhs)
	{
		return semver_compare(lhs, rhs) == 0;
	}

	inline BOOL semver_version_is_greater(const HSemverVersion lhs, const HSemverVersion rhs)
	{
		return semver_compare(lhs, rhs) > 0;
	}

	inline BOOL semver_version_is_less(const HSemverVersion lhs, const HSemverVersion rhs)
	{
		return semver_compare(lhs, rhs) < 0;
	}

	inline BOOL semver_version_is_greater_or_equal(const HSemverVersion lhs, const HSemverVersion rhs)
	{
		return semver_compare(lhs, rhs) >= 0;
	}

	inline BOOL semver_version_is_less_or_equal(const HSemverVersion lhs, const HSemverVersion rhs)
	{
		return semver_compare(lhs, rhs) <= 0;
	}

	
	// version check methods
	//////////////////////////

	SEMVER_API SemverParseResult semver_check_version_string(const char* version_str);
	SEMVER_API BOOL semver_version_is_valid(const HSemverVersion version);


	// Query methods
	/////////////////

	SEMVER_API BOOL semver_query_matches_version(const HSemverQuery query, const HSemverVersion version);

	SEMVER_API HSemverVersions semver_query_match_versions(const HSemverQuery query, const HSemverVersions versions);
	SEMVER_API HSemverVersion semver_query_highest_match(const HSemverQuery query, const HSemverVersions versions);

	// Query check methods
	///////////////////////

	SEMVER_API const char* semver_get_query_string(const HSemverQuery query); // \0 terminated string, freed when HSemverVersion handle is disposed

}