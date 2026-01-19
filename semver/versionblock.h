#pragma once

#include "semver.h"
#include "version.h"
#include <unordered_map>

namespace semver
{

	struct alignas(alignof(semver::Version)) SemverVersionBlock
	{
		enum class VersionOwnership : uint32_t // also serve as magic numbers to ensure pointers passed to the API originated from us
		{
			OWNS       = 0xed3d995e,
			REFERENCES = 0xde3d995e  // first two hex values exchanged
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
			: ownership(owner ? VersionOwnership::REFERENCES : VersionOwnership::OWNS),
			order(SEMVER_ORDER_AS_GIVEN),
			
			count(count),
			owner(owner)
		{
		};

	};

	extern std::unordered_map<SemverVersionBlock*, std::vector<SemverVersionBlock*>> sVersionBlockRefs;


	static_assert(offsetof(SemverVersionBlock, ownership) == 0, "ownership offset mismatch");
	static_assert(offsetof(SemverVersionBlock, count) == 8, "count offset mismatch");
	static_assert(offsetof(SemverVersionBlock, owner) == 16, "union offset mismatch");
	static_assert(offsetof(SemverVersionBlock, versions) == 24, "union offset mismatch");
	static_assert(sizeof(SemverVersionBlock) == 64, "Unexpected struct size"); // with flexible array it was only 32 (24 + some bogus 8 byte padding). With single entry its 64 (Version is 40 bytes)

	static_assert(std::is_trivially_copyable<semver::Version*>::value, "Version* must be trivially copyable");
	static_assert(alignof(SemverVersionBlock) >= alignof(semver::Version), "Block alignment must support embedded Version");
	static_assert(offsetof(SemverVersionBlock, versions) == offsetof(SemverVersionBlock, versionPtrs), "Union layout must be consistent");



	extern SemverVersionBlock* createVersionBlock(size_t count);
	extern SemverVersionBlock* createVersionReferenceBlock(SemverVersionBlock* owner, size_t count);
	extern void DisposeSemverVersionBlockHeapResources(SemverVersionBlock* block);
	extern void DisposeSemverVersionBlock(SemverVersionBlock* version_block); 

}