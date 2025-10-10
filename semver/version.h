// Copyright 2025 Jasper Schellingerhout. All rights reserved.

#pragma once
#include "API/semver.h"
#include <string>
#include <cstring> //why is this not imported via semver.h?

namespace semver
{
	char* cloneStr(const char* src, size_t len);

	inline size_t strlenSafe(const char* src) // we could add some extra safety checks for ridiculously long strings also later
	{
		if (!src)
			return 0;

		const char* it = src;
			
		while (*it && !(*it & 0x80)) // only lower 7 bit ASCII chars allowed
			++it;

		return it - src;
		
	}

	struct Version
	{

		uint64_t major, minor, patch; // 8 + 8 + 8

		enum Flags : uint8_t
		{
			PRERELEASE_ON_HEAP = 1,
			BUILD_IN_MAP = 2, // build and potential other meta data now in dictionary
			BUILD_UNDEFINED = 4, // build undefined
			MANAGED = 8, // don't dispose the Version, it's memory is managed (used by Version Arrays)
		};

		uint8_t flags; // 1
		static constexpr size_t heap_prerelease_pad = 7;
		static constexpr size_t inline_prerelease_len = heap_prerelease_pad + sizeof(const char*); // exploit 7 byte padding that would have been after flags
		char inline_prerelease[inline_prerelease_len];    // Inline string (null-terminated): 13 usable chars

		enum class UninitializedDefault : uint8_t
		{
			TAG,
			ZERO,
			WILD
		};

		void clear();


		inline bool majorIsWild() const { return major == SEMVER_WILDCARD_IDENTIFIER; }
		inline bool minorIsWild() const { return minor == SEMVER_WILDCARD_IDENTIFIER; }
		inline bool patchIsWild() const { return patch == SEMVER_WILDCARD_IDENTIFIER; }
		
		inline bool majorIsUndefined() const { return major == SEMVER_UNINITIALIZED_IDENTIFIER; }
		inline bool minorIsUndefined() const { return minor == SEMVER_UNINITIALIZED_IDENTIFIER; }
		inline bool patchIsUndefined() const { return patch == SEMVER_UNINITIALIZED_IDENTIFIER; }
		inline bool preReleaseIsUndefined() const { return isPrereleaseInline() && inline_prerelease[0] == SEMVER_UNINITIALIZED_LABEL[0]; }
		inline bool buildIsUndefined() const { return flags & BUILD_UNDEFINED; }

		inline bool isDefined() const
		{
			return major < SEMVER_MAX_NUMERIC_IDENTIFIER &&
				minor < SEMVER_MAX_NUMERIC_IDENTIFIER &&
				patch < SEMVER_MAX_NUMERIC_IDENTIFIER &&
				!buildIsUndefined() && !preReleaseIsUndefined();
		}

																	

		SemverParseResult parseInternal(const char* str, size_t len, bool ignoreBuild,
			UninitializedDefault uninitializedDefault);

		inline SemverParseResult parse(const char* str, size_t len, 
			UninitializedDefault uninitializedDefault = UninitializedDefault::TAG)
		{
			constexpr bool ignoreBuild_FALSE = false;
			return parseInternal(str, len, ignoreBuild_FALSE, uninitializedDefault);
		}

		inline SemverParseResult parseIgnoreBuild(const char* str, size_t len, 
			UninitializedDefault uninitializedDefault = UninitializedDefault::TAG)
		{
			constexpr bool ignoreBuild_TRUE = true;
			return parseInternal(str, len, ignoreBuild_TRUE, uninitializedDefault);
		}


		// prerelease 
		////////////

	
		enum class PreleaseParseResult : uint8_t
		{
			SUCCESS = SEMVER_PARSE_SUCCESS,
			EMPTY_IDENTIFIER = SEMVER_PARSE_PRERELEASE_EMPTY_IDENTIFIER,
			UNSUPPORTED_CHARACTER = SEMVER_PARSE_PRERELEASE_UNSUPPORTED_CHARACTER,
			DIGITS_WITH_LEADING_ZERO = SEMVER_PARSE_PRERELEASE_LEADING_ZERO_ON_NUMERIC_IDENTIFIER,
		};

		PreleaseParseResult trySetPrerelease(const char* str, size_t len);
		PreleaseParseResult trySetPrerelease(const char* str) { return trySetPrerelease(str, strlenSafe(str)); }


		static PreleaseParseResult parsePrerelease(const char* buffer, size_t len);
		inline static PreleaseParseResult parsePrerelease(const char* buffer) { return Version::parsePrerelease(buffer, strlenSafe(buffer)); }

		PreleaseParseResult parsePrerelease() const { return Version::parsePrerelease(getPrerelease()); }


		void setPrerelease(const char* str, size_t len);
		
		void setPrerelease(const char* str) 
			{ setPrerelease(str, strlenSafe(str)); }
		
		void deletePrerelease();
		
		inline bool isPrereleaseOnHeap() const 	{ return flags & PRERELEASE_ON_HEAP; }

		inline bool isPrereleaseInline() const { return !(flags & PRERELEASE_ON_HEAP); }
	
		inline const char* get_heap_prerelease() const 
		{ 
			return *reinterpret_cast<const char* const*>(&inline_prerelease[heap_prerelease_pad]); // so it aligns to a 64-bit boundary
		} 

		inline const char* getPrerelease() const { return isPrereleaseOnHeap() ? get_heap_prerelease(): inline_prerelease; }

		inline void setPrereleaseInline() { flags &= ~PRERELEASE_ON_HEAP; }
		inline void setPrereleaseOnHeap() { flags |= PRERELEASE_ON_HEAP; }

		void setHeapPrerelease(const char* ptr)
		{
			memcpy(&inline_prerelease[heap_prerelease_pad], &ptr, sizeof(ptr)); // the last sizeof(pointer) (8) chars of inline_prerelease, are now actually a pointer
			setPrereleaseOnHeap();
		}


		

		// build (metadata) - stored outside of struct
		//////////////////////////////////////////////

		enum class BuildParseResult : uint8_t
		{
			SUCCESS = SEMVER_PARSE_SUCCESS,
			EMPTY_IDENTIFIER = SEMVER_PARSE_BUILD_EMPTY_IDENTIFIER,
			UNSUPPORTED_CHARACTER = SEMVER_PARSE_BUILD_UNSUPPORTED_CHARACTER,
		};

		BuildParseResult trySetBuild(const char* str, size_t len);
		BuildParseResult trySetBuild(const char* str) { return trySetBuild(str, strlenSafe(str)); }

		static Version::BuildParseResult parseBuild(const char* buffer, size_t len);
		inline static Version::BuildParseResult parseBuild(const char* buffer) { return Version::parseBuild(buffer, strlenSafe(buffer)); }

		Version::BuildParseResult parseBuild() const { return Version::parseBuild(getBuild()); }
		
		void setBuild(const char* str, size_t len);
		
		void setBuild(const char* str) 
			{ setBuild(str, strlenSafe(str)); }
		
		void deleteBuild();

		inline bool hasBuild() const { return flags & BUILD_IN_MAP; }
		inline bool isPrerelease() const;

		const char* getBuild() const;
	
		// Operators/ Comparison
		////////////////////////
		inline bool sameCore(const Version& other) const { return major == other.major && minor == other.minor && patch == other.patch; }

		inline bool operator==(const Version& other) const
		{
			return sameCore(other) && strcmp(getPrerelease(), other.getPrerelease()) == 0; // builds are not compared
		};

		inline bool operator<(const Version& other) const
		{
			return Version::compare(*this, other) < 0;
		}
		
		inline bool operator>(const Version& other) const
		{
			return Version::compare(*this, other) > 0;
		}

		inline bool operator<=(const Version& other) const
		{
			return Version::compare(*this, other) <= 0;
		}

		inline bool operator>=(const Version& other) const
		{
			return Version::compare(*this, other) >= 0;
		}


		static int comparePrereleases(const char* lhs, const char* rhs);

		static int compare(const Version& lhs, const Version& rhs);


		// Cleanup
		//////////

		inline void deleteHeapResources() 
		{ 
			deletePrerelease(); 
			deleteBuild();
		}

		// Interpreted Info
		///////////////////


		std::string toString() const;

		inline bool isMaximum() const
		{
			return 
				major == SEMVER_MAX_NUMERIC_IDENTIFIER &&
				minor == SEMVER_MAX_NUMERIC_IDENTIFIER &&
				patch == SEMVER_MAX_NUMERIC_IDENTIFIER;
		}

		inline bool isMinimum() const
		{
			return 
				major == 0 &&
				minor == 0 &&
				patch == 0 &&
				isPrereleaseInline() &&
				inline_prerelease[0] == SEMVER_LOWEST_PRERELEASE[0];
		}


	};


	static_assert(offsetof(Version, major) == 0, "major offset mismatch");
	static_assert(offsetof(Version, minor) == 8, "minor offset mismatch");
	static_assert(offsetof(Version, patch) == 16, "patch offset mismatch");
	static_assert(offsetof(Version, flags) == 24, "flags offset mismatch");
	static_assert(offsetof(Version, inline_prerelease) == 25, "inline_prerelease_len offset mismatch");
	static_assert(offsetof(Version, inline_prerelease[Version::inline_prerelease_len]) == sizeof(Version), "inline_prerelease not aligned with end of struct");
	static_assert(sizeof(Version) == 40, "Unexpected Version struct size"); // with flexible array it was only 32 (24 + some bogus 8 byte padding). With single entry its 64 (Version is 40 bytes)
	static_assert(std::is_trivially_copyable<semver::Version>::value, "Version must be trivially copyable"); // no RAII we manage heap


}