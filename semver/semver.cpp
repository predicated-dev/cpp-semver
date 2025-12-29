// Copyright 2025 Jasper Schellingerhout. All rights reserved.

#include "semver.h"
#include "version.h"
#include "range.h"
#include <cstring>
#include <vector>
#include <string_view>
#include <unordered_map>
#include <algorithm>



// version constructor
///////////////////////

SEMVER_API HSemverVersion semver_version_create_defined(uint64_t major,	uint64_t minor, uint64_t patch, const char* prerelease, const char* build) // Create a new version object from a version string, returns NULL on error
{
	semver::Version* v = new semver::Version{major, minor, patch};
	v->trySetPrerelease(prerelease);
	v->trySetBuild(build);
	return reinterpret_cast<HSemverVersion>(v);
}

SEMVER_API HSemverVersion semver_version_create() 
{
	return reinterpret_cast<HSemverVersion>(new semver::Version{});
}


// version parse
/////////////////

SEMVER_API SemverParseResult semver_version_parse(HSemverVersion version, const char* version_str)
{
	return reinterpret_cast<semver::Version*>(version)->parse(version_str, semver::strlenSafe(version_str) );
}

// version array constructors
/////////////////////////////

struct alignas(alignof(semver::Version)) SemverVersionBlock
{
	enum class VersionOwnership : uint32_t // also serve as magic numbers to ensure pointers passed to the API originated from us
	{
		OWNS       = 0xed3d995e,
		REFERENCES  = 0xde3d995e  // first to hex values exchanged
	};


	static const SemverVersionBlock sEmpty;

	static SemverVersionBlock* getEmptyBlockPointer() { return const_cast<SemverVersionBlock*>(&SemverVersionBlock::sEmpty); }

	static HSemverVersions getEmptyBlockHandle() { return reinterpret_cast<HSemverVersions>(getEmptyBlockPointer()); };
	

	static SemverVersionBlock* pointerFromHandle(const HSemverVersions handle)
	{
		if (!handle)
			return getEmptyBlockPointer();

		auto version_block = reinterpret_cast<SemverVersionBlock*>(handle);

		switch (version_block->ownership)
		{ 
		case(VersionOwnership::OWNS):
		case(VersionOwnership::REFERENCES):
			return version_block;
		default:
			return getEmptyBlockPointer();
		}

	}


	VersionOwnership ownership; //must match one of the two magic numbers
	SemverOrder order;
	uint8_t reserved[3]; // explicit padding
	size_t count;
	SemverVersionBlock* owner; // must have VersionOwnership::OWNED or be nullptr

	union
	{
		semver::Version versions[1];     // empty flexible variable array [] is supported in visual studio only 
		semver::Version* versionPtrs[5]; // variable, but since versions are 40 bytes I might as well make this 5 so debugging is easiwer
	};


	semver::Version* getVersionPtrAt(size_t index) const
	{
		switch (ownership)
		{
		case(VersionOwnership::OWNS):
			return const_cast<semver::Version*>(&versions[index]);

		case(VersionOwnership::REFERENCES):
			return versionPtrs[index];

		default:
			return nullptr;
		}
	}

	SemverVersionBlock(size_t count, SemverVersionBlock* owner = nullptr) 
		: ownership( owner ? VersionOwnership::REFERENCES : VersionOwnership::OWNS), 
		order(SEMVER_ORDER_AS_GIVEN),
		owner(owner),
		count(count) {};

};

static std::unordered_map<SemverVersionBlock*, std::vector<SemverVersionBlock*>> sVersionBlockRefs;


static_assert(offsetof(SemverVersionBlock, ownership) == 0, "ownership offset mismatch");
static_assert(offsetof(SemverVersionBlock, count) == 8, "count offset mismatch");
static_assert(offsetof(SemverVersionBlock, owner) == 16, "union offset mismatch");
static_assert(offsetof(SemverVersionBlock, versions) == 24, "union offset mismatch");
static_assert(sizeof(SemverVersionBlock) == 64, "Unexpected struct size"); // with flexible array it was only 32 (24 + some bogus 8 byte padding). With single entry its 64 (Version is 40 bytes)

static_assert(std::is_trivially_copyable<semver::Version*>::value, "Version* must be trivially copyable");
static_assert(alignof(SemverVersionBlock) >= alignof(semver::Version), "Block alignment must support embedded Version");
static_assert(offsetof(SemverVersionBlock, versions) == offsetof(SemverVersionBlock, versionPtrs), "Union layout must be consistent");


const SemverVersionBlock SemverVersionBlock::sEmpty = SemverVersionBlock{ 0, nullptr };

static SemverVersionBlock* createVersionBlock(size_t count)
{
	if (count == 0)
		return SemverVersionBlock::getEmptyBlockPointer(); // all empty blocks share a single empty block pointer 

	size_t versionsSize = sizeof(semver::Version) * count;
	size_t totalSize = sizeof(SemverVersionBlock) - sizeof(semver::Version) + versionsSize; //one version size already counted

	auto* block = static_cast<SemverVersionBlock*>(::operator new(totalSize));
	
	new (block) SemverVersionBlock{ count, nullptr }; // use memory at start of block
	memset(&block->versions, 0, versionsSize); // Versions with all 0s

	for (size_t i = 0; i < block->count; ++i)
		block->versions[i].flags |= semver::Version::Flags::MANAGED;

	return block;
}

static SemverVersionBlock* createVersionReferenceBlock(SemverVersionBlock* owner, size_t count)
{
	if (count == 0)
		return SemverVersionBlock::getEmptyBlockPointer(); // all empty blocks share a single empty block pointer 

	size_t versionRefsSize = sizeof(semver::Version*) * count;
	size_t totalSize = sizeof(SemverVersionBlock) - sizeof(semver::Version) + versionRefsSize; //union has since of Version

	if (totalSize < sizeof(SemverVersionBlock))
		totalSize = sizeof(SemverVersionBlock); // if we have fewer the 5 pointers (Version is 40 bytes) we probably want to allocate at least what sizeof expects

	auto* block = static_cast<SemverVersionBlock*>(::operator new(totalSize));
	new (block) SemverVersionBlock{ count, owner }; 

	std::fill_n(block->versionPtrs, count, nullptr); //all pointers set to null
	
	sVersionBlockRefs[owner].push_back(block);


	return block;

}


static std::vector<std::string_view> splitMultistringBuffer(const char* buffer)
{
	std::vector<std::string_view> tokens;

	const char* current = buffer;

	while (*current != '\0')
	{
		std::string_view sv(current);
		tokens.push_back(sv);
		current += sv.size() + 1;
	}

	return tokens;
}

static std::vector<std::string_view> splitBuffer(std::string_view buffer, std::string_view separator)
{
	std::vector<std::string_view> tokens;

	size_t pos = 0;

	while (pos < buffer.size())
	{
		size_t next = buffer.find(separator, pos);

		if (next == std::string_view::npos)
		{
			tokens.emplace_back(buffer.substr(pos));
			break;
		}

		tokens.emplace_back(buffer.substr(pos, next - pos));
		pos = next + separator.size();
	}

	return tokens;
}

SEMVER_API HSemverVersions semver_versions_from_string(const char* versions_str, const char* separator, SemverOrder order)
{
	if (!versions_str)
		return reinterpret_cast<HSemverVersions>(SemverVersionBlock::getEmptyBlockHandle());

	std::vector<std::string_view> versionStrs = (!separator || *separator == '\0') ? splitMultistringBuffer(versions_str) : splitBuffer(versions_str, separator);

	size_t count = versionStrs.size();


	SemverVersionBlock* block = createVersionBlock(count);

	for (size_t i = 0; i < count; ++i)
		block->versions[i].parse(versionStrs[i].data(), versionStrs[i].size());

	if (order != SEMVER_ORDER_AS_GIVEN)
	{
		std::sort(block->versions, block->versions + count, 
			[order](const semver::Version& a, const semver::Version& b) 
			{
				int comp = semver::Version::compare(a, b);
				return (order == SEMVER_ORDER_ASC) ? (comp < 0) : (comp > 0);
			}
		);

	}

	block->order = order;

	return reinterpret_cast<HSemverVersions>(block);
}

SEMVER_API HSemverVersions semver_versions_create(size_t count)
{
	SemverVersionBlock* block = createVersionBlock(count);
	return reinterpret_cast<HSemverVersions>(block);
}

// query constructor
////////////////////

SEMVER_API HSemverQuery semver_query_create()
{
	semver::Query* q = new semver::Query();
	return reinterpret_cast<HSemverQuery>(q);
}


SEMVER_API SemverQueryParseResult semver_query_parse(HSemverQuery query, const char* query_str)
{
	return reinterpret_cast<semver::Query*>(query)->parse(query_str, semver::strlenSafe(query_str));
}

// destructors
//////////////////////////////

SEMVER_API void semver_version_dispose(HSemverVersion version) // Free the allocated version object
{
	semver::Version* v = reinterpret_cast<semver::Version*>(version);
	
	if (v->flags & semver::Version::Flags::MANAGED)
		return;  // block or range boundary will take care of it (maybe we can just do deleteHeapResources, but definitely not delete)

	v->deleteHeapResources();
	delete v;
}

static void DisposeSemverVersionBlockHeapResources(SemverVersionBlock* block)
{

	if (block->count != 0) // all empty blocks share the same static block which is not disposed
	{

		if (block->ownership == SemverVersionBlock::VersionOwnership::OWNS)
		{
			for (size_t i = 0; i < block->count; ++i)
				block->versions[i].deleteHeapResources(); // don't delete the version pointer! The block holds the data
		}

		::operator delete(block);
	}

}

SEMVER_API void semver_versions_dispose(HSemverVersions version_array)
{
	SemverVersionBlock* version_block = SemverVersionBlock::pointerFromHandle(version_array);

	if (version_block->ownership == SemverVersionBlock::VersionOwnership::REFERENCES)
	{
		if (version_block->owner) //we expect this for blocks holding refs only
		{
			auto it = sVersionBlockRefs.find(version_block->owner);
			if (it != sVersionBlockRefs.end()) // does this block have references? 
			{
				auto& blockRefs = it->second;

				if (std::erase(blockRefs, version_block) && (blockRefs.size() == 0))
					sVersionBlockRefs.erase(version_block->owner);
			}

		}
	}
	else // it owns the versions and may have reference lists
	{

		auto it = sVersionBlockRefs.find(version_block);
		if (it != sVersionBlockRefs.end()) // does this block have references? 
		{
			auto& blockRefs = it->second;

			for (auto blockref : it->second)
				DisposeSemverVersionBlockHeapResources(blockref);

			sVersionBlockRefs.erase(it); //it no longer valid beyond this
		}
	} 


	DisposeSemverVersionBlockHeapResources(version_block);

}

SEMVER_API void semver_query_dispose(HSemverQuery query)
{
	semver::Query* q = reinterpret_cast<semver::Query*>(query);
	for (semver::Range& r : q->rangeSet) 
	{
		r.lower.juncture.deleteHeapResources();
		r.upper.juncture.deleteHeapResources(); // while not technically allowed it is possible to add Build meta data so I don't just delete Prerelease heap resources
	}
	delete q;
}


// version array info
/////////////////////

SEMVER_API size_t semver_versions_count(HSemverVersions version_array)
{
	return SemverVersionBlock::pointerFromHandle(version_array)->count;
}

SEMVER_API HSemverVersion semver_versions_get_version_at_index(HSemverVersions version_array, size_t index)
{
	SemverVersionBlock* version_block = SemverVersionBlock::pointerFromHandle(version_array);

   return reinterpret_cast<HSemverVersion>(version_block->getVersionPtrAt(index));


}

// version info
///////////////

SEMVER_API uint64_t semver_get_version_major(const HSemverVersion version)
{
	return reinterpret_cast<semver::Version*>(version)->major;
}

SEMVER_API uint64_t semver_get_version_minor(const HSemverVersion version)
{
	return reinterpret_cast<semver::Version*>(version)->minor;
}

SEMVER_API uint64_t semver_get_version_patch(const HSemverVersion version)
{
	return reinterpret_cast<semver::Version*>(version)->patch;
}

SEMVER_API const char* semver_get_version_prerelease(const HSemverVersion version) // no need to dispose char*, disposed by HSemverVersion in semver_version_dispose
{
	return reinterpret_cast<semver::Version*>(version)->getPrerelease();
}

SEMVER_API const char* semver_get_version_build(const HSemverVersion version) // no need to dispose char*, disposed by HSemverVersion in semver_version_dispose
{
	return reinterpret_cast<semver::Version*>(version)->getBuild();
}

SEMVER_API char* semver_get_version_string(const HSemverVersion version) // \0 terminated string, freed with semver_free_version_string
{
	std::string s = reinterpret_cast<semver::Version*>(version)->toString();
	return semver::cloneStr(s.data(), s.size());
}

// version/query string destructor
////////////////////////////
SEMVER_API void semver_free_string(char* str)
{
	free(str);
}


// Query info
/////////////
SEMVER_API size_t semver_query_get_range_count(const HSemverQuery query)
{
	semver::Query* q = reinterpret_cast<semver::Query*>(query);
	return q->rangeSet.size();
}

SEMVER_API HSemverRange semver_query_get_range_at_index(const HSemverQuery query, size_t index)
{
	semver::Query* q = reinterpret_cast<semver::Query*>(query);
	return reinterpret_cast<HSemverRange>(&q->rangeSet[index]);
}

SEMVER_API HSemverBound semver_range_get_lower_bound(const HSemverRange range)
{
	semver::Range* r = reinterpret_cast<semver::Range*>(range);
	return reinterpret_cast<HSemverBound>(&r->lower);
}

SEMVER_API HSemverBound semver_range_get_upper_bound(const HSemverRange range)
{
	semver::Range* r = reinterpret_cast<semver::Range*>(range);
	return reinterpret_cast<HSemverBound>(&r->upper);
}

SEMVER_API const char* semver_range_get_min_prerelease(const HSemverRange range)
{
	semver::Range* r = reinterpret_cast<semver::Range*>(range);
	return r->minPreRelease.c_str(); // disposed with Range
}

SEMVER_API BOOL semver_bound_get_is_inclusive(const HSemverBound bound)
{
	semver::Bound* b = reinterpret_cast<semver::Bound*>(bound);
	return b->included == semver::Bound::Included::YES;
}

SEMVER_API HSemverVersion semver_bound_get_juncture(const HSemverBound bound)
{
	semver::Bound* b = reinterpret_cast<semver::Bound*>(bound);
	return reinterpret_cast<HSemverVersion>(&b->juncture);
}


// version mutators
///////////////////

SEMVER_API SemverParseResult semver_set_version_major(HSemverVersion version, uint64_t major)
{
	if (major > SEMVER_MAX_NUMERIC_IDENTIFIER)
		return SEMVER_PARSE_MAJOR_TOO_LARGE;

	reinterpret_cast<semver::Version*>(version)->major = major;

	return SEMVER_PARSE_SUCCESS;
}

SEMVER_API SemverParseResult semver_set_version_minor(HSemverVersion version, uint64_t minor)
{
	if (minor > SEMVER_MAX_NUMERIC_IDENTIFIER)
		return SEMVER_PARSE_MINOR_TOO_LARGE;
	
	reinterpret_cast<semver::Version*>(version)->minor = minor;

	return SEMVER_PARSE_SUCCESS;
}

SEMVER_API SemverParseResult semver_set_version_patch(HSemverVersion version, uint64_t patch)
{
	if (patch > SEMVER_MAX_NUMERIC_IDENTIFIER)
		return SEMVER_PARSE_PATCH_TOO_LARGE;
	
	reinterpret_cast<semver::Version*>(version)->patch = patch;

	return SEMVER_PARSE_SUCCESS;
}

SEMVER_API SemverParseResult semver_set_version_prerelease(HSemverVersion version, const char* prerelease)
{
	return static_cast<SemverParseResult>( reinterpret_cast<semver::Version*>(version)->trySetPrerelease(prerelease) );
}

SEMVER_API SemverParseResult semver_set_version_build(HSemverVersion version, const char* build)
{
	return static_cast<SemverParseResult>( reinterpret_cast<semver::Version*>(version)->trySetBuild(build) );
}

SEMVER_API SemverParseResult semver_set_version_core_triplet(HSemverVersion version, uint64_t major, uint64_t minor, uint64_t patch)
{
	if (major > SEMVER_MAX_NUMERIC_IDENTIFIER)
		return SEMVER_PARSE_MAJOR_TOO_LARGE;

	if (minor > SEMVER_MAX_NUMERIC_IDENTIFIER)
		return SEMVER_PARSE_MINOR_TOO_LARGE;

	if (patch > SEMVER_MAX_NUMERIC_IDENTIFIER)
		return SEMVER_PARSE_PATCH_TOO_LARGE;

	semver::Version* v = reinterpret_cast<semver::Version*>(version);
	v->major = major;
	v->minor = minor;
	v->patch = patch;

	return SEMVER_PARSE_SUCCESS;
}

SEMVER_API SemverParseResult semver_set_version_values(HSemverVersion version, uint64_t major, uint64_t minor, uint64_t patch, const char* prerelease, const char* build)
{
	if (major > SEMVER_MAX_NUMERIC_IDENTIFIER)
		return SEMVER_PARSE_MAJOR_TOO_LARGE;

	if (minor > SEMVER_MAX_NUMERIC_IDENTIFIER)
		return SEMVER_PARSE_MINOR_TOO_LARGE;

	if (patch > SEMVER_MAX_NUMERIC_IDENTIFIER)
		return SEMVER_PARSE_PATCH_TOO_LARGE;

	auto prerelease_result = semver::Version::parsePrerelease(prerelease);

	if (prerelease_result != semver::Version::PreleaseParseResult::SUCCESS)
		return static_cast<SemverParseResult>(prerelease_result);

	auto build_result = semver::Version::parseBuild(build);

	if (build_result != semver::Version::BuildParseResult::SUCCESS)
		return static_cast<SemverParseResult>(build_result);

	semver::Version* v = reinterpret_cast<semver::Version*>(version);

	v->major = major;
	v->minor = minor;
	v->patch = patch;
	v->setPrerelease(prerelease);
	v->setBuild(build);

	return SEMVER_PARSE_SUCCESS;

}


// Query mutators
/////////////
SEMVER_API HSemverRange semver_query_add_range(HSemverQuery query)
{
	semver::Query* q = reinterpret_cast<semver::Query*>(query);
	semver::Range& r = q->rangeSet.emplace_back(semver::Range());
	return reinterpret_cast<HSemverRange>(&r);
}


SEMVER_API void semver_query_erase_range_at_index(HSemverQuery query, size_t index) //also disposes the range
{
	semver::Query* q = reinterpret_cast<semver::Query*>(query);

	q->rangeSet[index].lower.juncture.deleteHeapResources(); // some fools might add build meta data so I can't just delete prerelease info
	q->rangeSet[index].upper.juncture.deleteHeapResources(); 

	q->rangeSet.erase(q->rangeSet.begin() + index);
}

SEMVER_API SemverParseResult semver_range_set_min_prerelease(HSemverRange range, const char* prerelease)
{
	auto prerelease_result = semver::Version::parsePrerelease(prerelease);

	if (prerelease_result != semver::Version::PreleaseParseResult::SUCCESS)
		return static_cast<SemverParseResult>(prerelease_result);

	semver::Range* r = reinterpret_cast<semver::Range*>(range);
	r->minPreRelease = std::string(prerelease);

	return SEMVER_PARSE_SUCCESS;
}

SEMVER_API void semver_range_set_to_all(HSemverRange range)
{
	reinterpret_cast<semver::Range*>(range)->setToAll();
}

SEMVER_API void semver_range_set_to_none(HSemverRange range)
{
	reinterpret_cast<semver::Range*>(range)->setToNone();
}


SEMVER_API void semver_bound_set_is_inclusive(HSemverBound bound, bool inclusive)
{
	semver::Bound* b = reinterpret_cast<semver::Bound*>(bound);
	b->included = inclusive ? semver::Bound::Included::YES : semver::Bound::Included::NO;
} 

SEMVER_API void semver_bound_set_juncture(HSemverBound bound, HSemverVersion juncture)
{
	semver::Bound* b = reinterpret_cast<semver::Bound*>(bound);
	semver::Version* v = reinterpret_cast<semver::Version*>(juncture);

	b->juncture.major = v->major;
	b->juncture.minor = v->minor;
	b->juncture.patch = v->patch;
	b->juncture.setPrerelease(v->getPrerelease()); // juncture should not have build info, but if they do I don't want to copy that

}

SEMVER_API void semver_bound_set_to_min(HSemverBound bound)
{
	reinterpret_cast<semver::Bound*>(bound)->setToMin();
}

SEMVER_API void semver_bound_set_to_max(HSemverBound bound)
{
	reinterpret_cast<semver::Bound*>(bound)->setToMax();
}

SEMVER_API SemverParseResult semver_set_juncture(HSemverVersion juncture, uint64_t major, uint64_t minor, uint64_t patch, const char* prerelease)
{
	if (major > SEMVER_MAX_NUMERIC_IDENTIFIER)
		return SEMVER_PARSE_MAJOR_TOO_LARGE;

	if (minor > SEMVER_MAX_NUMERIC_IDENTIFIER)
		return SEMVER_PARSE_MINOR_TOO_LARGE;

	if (patch > SEMVER_MAX_NUMERIC_IDENTIFIER)
		return SEMVER_PARSE_PATCH_TOO_LARGE;

	auto prerelease_result = semver::Version::parsePrerelease(prerelease);

	if (prerelease_result != semver::Version::PreleaseParseResult::SUCCESS)
		return static_cast<SemverParseResult>(prerelease_result);

	semver::Version* v = reinterpret_cast<semver::Version*>(juncture);

	v->major = major;
	v->minor = minor;
	v->patch = patch;
	v->setPrerelease(prerelease);

	return SEMVER_PARSE_SUCCESS;
}


// version comparison
/////////////////////

SEMVER_API int semver_compare(const HSemverVersion lhs, const HSemverVersion rhs)
{
	return semver::Version::compare(*reinterpret_cast<semver::Version*>(lhs), *reinterpret_cast<semver::Version*>(rhs));
}


// version check methods
//////////////////////////

SEMVER_API SemverParseResult semver_check_version_string(const char* version_str)
{
	semver::Version v{};
	SemverParseResult result = v.parse(version_str, semver::strlenSafe(version_str));
	v.deleteHeapResources();
	return result;
}

SEMVER_API BOOL semver_version_is_valid(const HSemverVersion version)
{
	semver::Version* v = reinterpret_cast<semver::Version*>(version);
	return
		v->major <= SEMVER_MAX_NUMERIC_IDENTIFIER &&
		v->minor <= SEMVER_MAX_NUMERIC_IDENTIFIER &&
		v->patch <= SEMVER_MAX_NUMERIC_IDENTIFIER &&
		v->parsePrerelease() == semver::Version::PreleaseParseResult::SUCCESS &&
		v->parseBuild() == semver::Version::BuildParseResult::SUCCESS;


}


// Query methods
/////////////////

SEMVER_API BOOL semver_query_matches_version(const HSemverQuery query, const HSemverVersion version)
{
	semver::Query* q = reinterpret_cast<semver::Query*>(query);
	
	return q->matches(*reinterpret_cast<semver::Version*>(version));

}


struct StartEndIndex
{
	size_t startIndex;
	size_t endIndex;
};



// PRE: block is not empty
StartEndIndex findASCSortedBlockStartIndex(const semver::Query& q,  const SemverVersionBlock& b)
{

	const semver::Version& minVersion = q.lowBound().juncture;
	const semver::Version& maxVersion = q.highBound().juncture;

	

	size_t startindex = 0;
	int low;
	int high;
	int mid;

	if (!minVersion.isMinimum())
	{
		low = 0;
		high = b.count - 1;

		while (low <= high)
		{
			mid = low + (high - low) / 2;
			semver::Version& v = *b.getVersionPtrAt(mid);

			if (v >= minVersion)
			{
				startindex = mid;
				high = mid - 1;
			}
			if (v < minVersion)
				low = mid + 1;

		}
	}

	size_t endindex = b.count-1;

	if (!maxVersion.isMaximum())
	{
		low = startindex;
		high = b.count - 1;

		while (low <= high)
		{
			mid = low + (high - low) / 2;
			semver::Version& v = *b.getVersionPtrAt(mid);

			if (v <= maxVersion)
			{
				endindex = mid;
				low = mid + 1;
			}
			if (v > maxVersion)
				high = mid - 1;

		}
	}


	return { startindex, endindex };

}

// PRE: block is not empty
StartEndIndex findDESCSortedBlockStartIndex(const semver::Query& q, const SemverVersionBlock& b)
{

	const semver::Version& minVersion = q.lowBound().juncture;
	const semver::Version& maxVersion = q.highBound().juncture;

	int low;
	int high;
	int mid;

	size_t startindex = 0;
	if (!maxVersion.isMaximum())
	{

		low = 0;
		high = b.count - 1;
		while (low <= high)
		{
			mid = low + (high - low) / 2;
			semver::Version& v = *b.getVersionPtrAt(mid);

			if (v <= maxVersion)
			{
				startindex = mid;
				high = mid - 1;
			}
			if (v > maxVersion)
				low = mid + 1;

		}
	}

	size_t endindex = b.count-1;

	if (!minVersion.isMinimum())
	{

		low = startindex;
		high = b.count - 1;

		while (low <= high)
		{
			mid = low + (high - low) / 2;
			semver::Version& v = *b.getVersionPtrAt(mid);

			if (v >= minVersion)
			{
				endindex = mid;
				low = mid + 1;
			}
			if (v < minVersion)
				high = mid - 1;

		}
	}


	return { startindex, endindex };

}


SEMVER_API HSemverVersions semver_query_match_versions(const HSemverQuery query, const HSemverVersions versions)
{
	SemverVersionBlock* b = SemverVersionBlock::pointerFromHandle(versions);

	if (b->count == 0)
		return SemverVersionBlock::getEmptyBlockHandle();

	semver::Query* q = reinterpret_cast<semver::Query*>(query);

	StartEndIndex indices{ 0, b->count-1 };

	if (b->order == SEMVER_ORDER_ASC)
		indices = findASCSortedBlockStartIndex(*q, *b);
	else if (b->order == SEMVER_ORDER_DESC)
		indices = findDESCSortedBlockStartIndex(*q, *b);

	if (indices.startIndex > indices.endIndex)
		return SemverVersionBlock::getEmptyBlockHandle();


	std::vector<semver::Version*> matched;

	for (size_t i = indices.startIndex; i <= indices.endIndex; ++i)
	{
		semver::Version* v = b->getVersionPtrAt(i);

		if (q->matches(*v))
			matched.push_back(v);
	}

	size_t matchCount = matched.size();

	if (matchCount == 0)
		return reinterpret_cast<HSemverVersions>(SemverVersionBlock::getEmptyBlockHandle());

	SemverVersionBlock* result = createVersionReferenceBlock(b->owner? b->owner : b, matchCount); // reference blocks don't own reference blocks

	std::memcpy(result->versionPtrs, matched.data(), sizeof(semver::Version*) * matchCount);

	return reinterpret_cast<HSemverVersions>(result);

}


SEMVER_API HSemverVersion semver_query_highest_match(const HSemverQuery query, const HSemverVersions versions)
{
	semver::Query* q = reinterpret_cast<semver::Query*>(query);
	SemverVersionBlock* b = SemverVersionBlock::pointerFromHandle(versions);

	// consider a solution that sorts versions in a reference block or if the source versions is sorted (may need a flag)
	semver::Version* vMax = nullptr;

	for (size_t i = 0; i < b->count; ++i)
	{
		semver::Version* v = b->getVersionPtrAt(i);

		if (q->matches(*v))
		{
			if (!vMax || semver::Version::compare(*v, *vMax) > 0)
				vMax = v;
		}
	}

	return reinterpret_cast<HSemverVersion>(vMax);

}

// Query check methods
///////////////////////
SEMVER_API const char* semver_get_query_string(const HSemverQuery query) // \0 terminated string. Caller needs to free
{
	semver::Query* q = reinterpret_cast<semver::Query*>(query);
	std::string s = q->toString();
	return semver::cloneStr(s.c_str(), s.size());
}
