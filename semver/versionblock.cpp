// Copyright 2026 Jasper Schellingerhout. All rights reserved.

#include "versionblock.h"

namespace semver
{

	
	const SemverVersionBlock SemverVersionBlock::sEmpty = { 0, nullptr };


	std::unordered_map<SemverVersionBlock*, std::vector<SemverVersionBlock*>> sVersionBlockRefs;

	SemverVersionBlock* createVersionBlock(size_t count)
	{
		if (count == 0)
			return SemverVersionBlock::getEmptyBlockPointer(); // all empty blocks share a single empty block pointer 

		size_t versionsSize = sizeof(Version) * count;
		size_t totalSize = sizeof(SemverVersionBlock) - sizeof(Version) + versionsSize; //one version size already counted

		auto* block = static_cast<SemverVersionBlock*>(::operator new(totalSize));

		new (block) SemverVersionBlock{ count, nullptr }; // use memory at start of block
		memset(&block->versions, 0, versionsSize); // Versions with all 0s

		for (size_t i = 0; i < block->count; ++i)
			block->versions[i].flags |= Version::Flags::MANAGED;

		return block;
	}

	SemverVersionBlock* createVersionReferenceBlock(SemverVersionBlock* owner, size_t count)
	{
		if (count == 0)
			return SemverVersionBlock::getEmptyBlockPointer(); // all empty blocks share a single empty block pointer 

		size_t versionRefsSize = sizeof(Version*) * count;
		size_t totalSize = sizeof(SemverVersionBlock) - sizeof(Version) + versionRefsSize; //union has since of Version

		if (totalSize < sizeof(SemverVersionBlock))
			totalSize = sizeof(SemverVersionBlock); // if we have fewer the 5 pointers (Version is 40 bytes) we probably want to allocate at least what sizeof expects

		auto* block = static_cast<SemverVersionBlock*>(::operator new(totalSize));
		new (block) SemverVersionBlock{ count, owner };

		std::fill_n(block->versionPtrs, count, nullptr); //all pointers set to null

		sVersionBlockRefs[owner].push_back(block);


		return block;

	}

	void DisposeSemverVersionBlockHeapResources(SemverVersionBlock* block)
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

	void DisposeSemverVersionBlock(SemverVersionBlock* version_block)
	{
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
}
