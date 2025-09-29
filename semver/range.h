#pragma once
#include "version.h"
#include <vector>
#include <string>
#include "semver.h"

namespace semver
{
	struct Bound
	{

		Version version;
		
		enum class Included : uint8_t
		{
			NO = 0,
			YES = 1,
		};

		Included included;

		enum class MatchPreReleases : uint8_t
		{
			NO = 0,
			YES = 1,
		};

		MatchPreReleases matchPreReleases;

		inline bool canMatchPreReleases() const { return matchPreReleases == MatchPreReleases::YES; }
		inline bool isIncluded() const { return included == Included::YES; }


		inline bool operator==(const Bound& other) const
		{
			return (version == other.version) && (included == other.included);
		};

		void setToMin();
		void setToMax();

		Bound() { version.flags = Version::MANAGED;	}

		Bound(const Version& version, Included include, MatchPreReleases matchPreReleases)
			: version{ version }, included{ included }, matchPreReleases{ matchPreReleases }
		{
			this->version.flags |= Version::MANAGED;
		}

	};

	enum class ComparatorPrefix : uint8_t
	{
		NONE, // defaults to EQ, but does not move parser cursor
		EQ, // =
		TIL, // ~ 
		CAR, // ^
		GT,  // >
		LT,  // <
		GTE, // >=
		LTE, // <=
	};

	struct Range
	{
		Bound lower;
		Bound upper;
		std::string minPreRelease;

		inline bool isNone() const
		{
		
			int boundComparison = Version::compare(lower.version, upper.version);

			return (boundComparison < 0) ||	(boundComparison == 0 ) &&
				(lower.included == Bound::Included::YES || upper.included == Bound::Included::YES); 	// if the versions match but either or both are excluded, we have nothing left
		}

	
		bool isAll() const
		{
			return upper.version.isMaximum() && lower.version.isMinimum();
		}

		SemverParseResult add(ComparatorPrefix prefix, std::string_view version_str);

		void setToNone();

		void setToAll();


		SemverParseResult addTildeComparator(std::string_view version_str);

		SemverParseResult addCaretComparator(std::string_view version_str);

		SemverParseResult addMinimumAllowWildcards(std::string_view version_str, Bound::Included included);

		SemverParseResult addMaximumAllowWildcards(std::string_view version_str, Bound::Included included);

		SemverParseResult addExactOrWildcardComparator(std::string_view version_str);

		SemverParseResult addRange(std::string_view version_from, std::string_view version_to);


		bool hasWithinBounds(const Version& version) const;
		bool matches(const Version& version) const;
		std::string toString() const;


		Range()
			: lower{ Version{0, 0, 0, Version::MANAGED, '0' }, 
				Bound::Included::YES, Bound::MatchPreReleases::NO },

			  upper{ Version{SEMVER_MAX_NUMERIC_IDENTIFIER, SEMVER_MAX_NUMERIC_IDENTIFIER, SEMVER_MAX_NUMERIC_IDENTIFIER, Version::MANAGED },
				Bound::Included::YES, Bound::MatchPreReleases::NO }
		{}

		Range(const Bound& lower, const Bound& upper) 
			: lower{ lower }, upper{ upper } 
		{}


	};

	struct Query
	{
		std::vector<Range> rangeSet;

		SemverQueryParseResult parse(const char* str, size_t len);

		bool matches(const Version& version) const;
		std::string toString() const;
	
	};




}