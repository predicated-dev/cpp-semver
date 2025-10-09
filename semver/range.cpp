// Copyright 2025 Jasper Schellingerhout. All rights reserved.

#include "range.h"
#include <algorithm>

namespace semver
{

    static const Bound sMinBound = Bound{ Version{0, 0, 0, Version::MANAGED, '0' }, Bound::Included::YES, Bound::MatchPreReleases::NO };
    static const Bound sMaxBound = Bound{ Version{SEMVER_MAX_NUMERIC_IDENTIFIER, SEMVER_MAX_NUMERIC_IDENTIFIER, SEMVER_MAX_NUMERIC_IDENTIFIER, Version::MANAGED},
         Bound::Included::YES, Bound::MatchPreReleases::NO };

    static const Range sAllRange = Range{ sMinBound, sMaxBound };

    inline static size_t prefixSize(ComparatorPrefix p)
    {
        switch (p)
        {
        case ComparatorPrefix::NONE:
            return 0;
        case ComparatorPrefix::GTE:
        case ComparatorPrefix::LTE:
            return 2;
        default:
            return 1;

        }
    }

    static SemverQueryParseResult queryParseResultFromVersionParseResult(SemverParseResult versionParseResult)
    {

        switch (versionParseResult)
        {
        case SEMVER_PARSE_SUCCESS:
        case SEMVER_PARSE_MAJOR_WILDCARD:
        case SEMVER_PARSE_MINOR_WILDCARD:
        case SEMVER_PARSE_PATCH_WILDCARD:
        case SEMVER_PARSE_TOO_FEW_PARTS:
        
        //whitespace leadin/ trailing non-fatal but we should never see this
        case SEMVER_PARSE_LEADING_WHITESPACE:
        case SEMVER_PARSE_TRAILING_WHITESPACE:

        // builds are ignored so we should never see this
        case SEMVER_PARSE_BUILD_EMPTY_IDENTIFIER:
        case SEMVER_PARSE_BUILD_UNSUPPORTED_CHARACTER:
        
 
            return SEMVER_QUERY_PARSE_SUCCESS;

        default:
            return static_cast<SemverQueryParseResult>(versionParseResult);
        }


    }


    const Bound& Query::lowBound() const
    {
        
        if (rangeSet.size() == 0)
            return sMaxBound;

        const Bound* min = &rangeSet[0].lower;

        for (size_t i = 1; i < rangeSet.size(); ++i)
        {
            if (Version::compare(rangeSet[i].lower.juncture, min->juncture) < 0)
                min = &rangeSet[i].lower;
        }

        return *min;
    }

    const Bound& Query::highBound() const
    {
        if (rangeSet.size() == 0)
            return sMinBound;

        const Bound* max = &rangeSet[0].upper;

        for (size_t i = 1; i < rangeSet.size(); ++i)
        {
            if (Version::compare(rangeSet[i].upper.juncture, max->juncture) > 0)
                max = &rangeSet[i].upper;
        }

        return *max;
    }

    SemverQueryParseResult Query::parse(const char* str, size_t len)
	{ 
        rangeSet.clear();

        Range range = sAllRange;
 
        size_t tokenStart = 0;
        size_t tokenEnd = 0;
        SemverQueryParseResult parsedResult = SEMVER_QUERY_PARSE_SUCCESS;

        for (size_t pos = 0; pos < len; )
        {

            while (pos < len && str[pos] <= ' ')
                ++pos;

            bool atEnd = pos == len;
            bool atDivider;

            if (atEnd)
                atDivider = false;
            else if (atDivider = str[pos] == '|' && str[pos + 1] == '|')
            {
                pos += 2;

                while (pos < len && str[pos] <= ' ')
                    ++pos;

                atEnd = pos == len;
            }

            if (atEnd || atDivider)
            {
                rangeSet.push_back(range);

                range = sAllRange;

                if (atEnd) // consider combining overlapped ranges provided that explicit pre-release are retained
                    break;
            }

            ComparatorPrefix prefix = ComparatorPrefix::NONE;
            switch (str[pos])
            {
            case '=':
                prefix = ComparatorPrefix::EQ;
                break;

            case '~':
                prefix = ComparatorPrefix::TIL;
                break;

            case '^':
                prefix = ComparatorPrefix::CAR;
                break;

            case '>':
                if ((pos + 1 < len) && str[pos + 1] == '=')
                    prefix = ComparatorPrefix::GTE;
                else
                    prefix = ComparatorPrefix::GT;
                break;

            case '<':
                if ((pos + 1 < len) && str[pos + 1] == '=')
                    prefix = ComparatorPrefix::LTE;
                else
                    prefix = ComparatorPrefix::LT;
            }

            pos += prefixSize(prefix);

            if (prefix == ComparatorPrefix::NONE)
                prefix = ComparatorPrefix::EQ; // set after adjusting pos

            while (pos < len && str[pos] <= ' ')
                ++pos;

            size_t versionStart = pos;
            ++pos;

            atDivider = false;

            while (pos < len && str[pos] >  ' ' && !(atDivider = str[pos] == '|'))
                ++pos;


            std::string_view versionStr(str + versionStart, pos - versionStart);

            while (pos < len && str[pos] <= ' ')
                ++pos;

            SemverParseResult versionParseResult;
            if (str[pos] == '-')
            {
                ++pos;
                while (pos < len && str[pos] <= ' ')
                    ++pos;

                versionStart = pos;
                ++pos;
                while (pos < len && str[pos] >  ' ')
                    ++pos;


                std::string_view versionStr2(str + versionStart, pos - versionStart);

                versionParseResult = range.addRange(versionStr, versionStr2);
            }
            else
                versionParseResult = range.add(prefix, versionStr);

             parsedResult =  queryParseResultFromVersionParseResult(versionParseResult);

            if (!parsedResult == SEMVER_QUERY_PARSE_SUCCESS)
                return parsedResult;

            while (pos < len && str[pos] <= ' ')
                ++pos;

            if (str[pos] == '@')
            {
                ++pos;
                size_t minPrereleaseStart = pos;
                ++pos;

                while (pos < len && str[pos] > ' ' && str[pos] != '|')
                    pos++;

                const char* prerelease = str + minPrereleaseStart;
                size_t prereleaseLen = pos - minPrereleaseStart;

                switch (Version::parsePrerelease(prerelease, prereleaseLen))
                {
                case Version::PreleaseParseResult::SUCCESS:
                    break;

                case Version::PreleaseParseResult::DIGITS_WITH_LEADING_ZERO:
                    return SEMVER_QUERY_PARSE_MIN_PRERELEASE_LEADING_ZERO_ON_NUMERIC_IDENTIFIER;

                case Version::PreleaseParseResult::EMPTY_IDENTIFIER:
                    return SEMVER_QUERY_PARSE_MIN_PRERELEASE_EMPTY_IDENTIFIER;

                case Version::PreleaseParseResult::UNSUPPORTED_CHARACTER:
                    return SEMVER_QUERY_PARSE_MIN_PRERELEASE_UNSUPPORTED_CHARACTER;
                }

                range.minPreRelease = std::string(prerelease, prereleaseLen);

            }

        }

        if (rangeSet.size() == 0 || !range.isAll()) // consider combining overlapped ranges provided that explicit pre-release are retained
            rangeSet.push_back(range); //only or with all if not already have other bounds

        return parsedResult;
	}

    bool Query::hasWithinAnyRangeBounds(const Version& version) const
    {
        for (const Range& r : rangeSet)
            if (r.hasWithinBounds(version))
                return true;

        return false;
    }

    bool Query::matches(const Version& version) const
    {
        if (version.isDefined())
        {
            if (version.isPrerelease())
            {
                for (const Range& r : rangeSet)
                    if (r.matches(version))
                        return true;
            }
            else
                return hasWithinAnyRangeBounds(version);
        }

        return false;
    }


    std::string Query::toString() const
    {
        std::string result;

        if (rangeSet.size() == 0)
        {
           return "<0.0.0-0";
        }
        else
        {
            std::for_each(rangeSet.cbegin(), rangeSet.cend() - 1,
                [&result](const Range& bounds)
                {
                    result += bounds.toString() + " || ";
                }
            );

            result += rangeSet.back().toString();
        }

        return result;
    }


    SemverParseResult Range::add(ComparatorPrefix prefix, std::string_view version_str)
    {
        switch (prefix)
        {

        case ComparatorPrefix::TIL:
            return addTildeComparator(version_str);

        case ComparatorPrefix::CAR:
            return addCaretComparator(version_str);


        case ComparatorPrefix::GT:
            return addMinimumAllowWildcards(version_str, Bound::Included::NO);
 
        case ComparatorPrefix::LT:
            return addMaximumAllowWildcards(version_str, Bound::Included::NO);

        case ComparatorPrefix::GTE:
            return addMinimumAllowWildcards(version_str, Bound::Included::YES);

        case ComparatorPrefix::LTE:
            return addMaximumAllowWildcards(version_str, Bound::Included::YES);

        }

        return addExactOrWildcardComparator(version_str);

    }

    static bool rangeVersionParseResultOK(SemverParseResult parseResult)
    {
        switch (parseResult)
        {
        case SEMVER_PARSE_SUCCESS:
        case SEMVER_PARSE_MAJOR_WILDCARD:
        case SEMVER_PARSE_MINOR_WILDCARD:
        case SEMVER_PARSE_PATCH_WILDCARD:
        case SEMVER_PARSE_TOO_FEW_PARTS:
            // builds are just metadata and play no role in ranges
        case SEMVER_PARSE_BUILD_EMPTY_IDENTIFIER:
        case SEMVER_PARSE_BUILD_UNSUPPORTED_CHARACTER:

            //    case SEMVER_QUERY_PARSE_PRERELEASE_SUB_RANGE_IDENTIFIER:
            return true;

        }

        return false;
    }

    void Range::setToNone()
    {
        lower.juncture.clear();
        upper.juncture.clear();
        lower.juncture.flags = upper.juncture.flags = Version::Flags::MANAGED; // clear also cleared the managed flag
        upper.included = lower.included = Bound::Included::NO;

    }

    void Range::setToAll()
    {
        lower.juncture.major = lower.juncture.minor = lower.juncture.patch = 0;
        lower.juncture.setPrerelease(SEMVER_LOWEST_PRERELEASE, 1);

        upper.juncture.major = upper.juncture.minor = upper.juncture.patch = SEMVER_MAX_NUMERIC_IDENTIFIER;
        upper.juncture.deletePrerelease();

        upper.included = lower.included = Bound::Included::YES;
    }


    SemverParseResult Range::addTildeComparator(std::string_view version_str)
    {
        
        SemverParseResult result = addMinimumAllowWildcards(version_str, Bound::Included::YES);

        if (result == SEMVER_PARSE_MAJOR_WILDCARD)
        {
            setToAll();
            return result;
        }

        if (result == SEMVER_PARSE_MINOR_WILDCARD /*|| result == SEMVER_PARSE_MINOR_EMPTY*/)
        {
            upper.juncture.major = lower.juncture.major + 1;
            upper.juncture.minor = 0;
        }
        else
        {
            upper.juncture.major = lower.juncture.major;
            upper.juncture.minor = lower.juncture.minor + 1;
        }

        upper.juncture.patch = 0;
        upper.juncture.setPrerelease(SEMVER_LOWEST_PRERELEASE, 1);
        upper.matchPreReleases = Bound::MatchPreReleases::NO;
        upper.included = Bound::Included::NO;

        return result;

    }

    SemverParseResult Range::addCaretComparator(std::string_view version_str)
    {

        SemverParseResult result = addMinimumAllowWildcards(version_str, Bound::Included::YES);

        if (result == SEMVER_PARSE_MAJOR_WILDCARD)
        {
            setToAll();
            return result;
        }
     
        if (result == SEMVER_PARSE_MINOR_WILDCARD || lower.juncture.major > 0)
        {
            upper.juncture.major = lower.juncture.major + 1;
            upper.juncture.minor = upper.juncture.patch = 0;
        }
        else if (result == SEMVER_PARSE_PATCH_WILDCARD || lower.juncture.minor > 0)
        {
            upper.juncture.major = lower.juncture.major;
            upper.juncture.minor = lower.juncture.minor + 1;
            upper.juncture.patch = 0;
        }
        else
        {
            upper.juncture.major = lower.juncture.major;
            upper.juncture.minor = lower.juncture.minor;
            upper.juncture.patch = lower.juncture.patch + 1;
        }

        upper.juncture.setPrerelease(SEMVER_LOWEST_PRERELEASE, 1);
        upper.matchPreReleases = Bound::MatchPreReleases::NO;
        upper.included = Bound::Included::NO;

        return result;

    }

    SemverParseResult Range::addMinimumAllowWildcards(std::string_view version_str, Bound::Included included)
    {

        SemverParseResult result = lower.juncture.parseIgnoreBuild(version_str.data(), version_str.size(), Version::UninitializedDefault::WILD);
        

        if (!rangeVersionParseResultOK(result))
        {
            setToNone();
            return result;
        }

        
        if (lower.juncture.majorIsWild())
        {
            if (included == Bound::Included::YES)
                setToAll(); // >= * implies all
            else
                setToNone(); // > * implies nothing

            return result;
        }

        bool minorWild = false;
        bool patchWild = false;

        if ((minorWild = lower.juncture.minorIsWild() || lower.juncture.minorIsUndefined()) || 
            (patchWild = lower.juncture.patchIsWild() || lower.juncture.patchIsUndefined()))
        {

            if (minorWild)
            {
                result = SEMVER_PARSE_MINOR_WILDCARD;
                lower.juncture.minor = 0;
                if (included == Bound::Included::NO)  // > 2.x  :=  >= 3.0.0-0
                     ++lower.juncture.major;
                   
            }
            else // patch is wild
            {
                result = SEMVER_PARSE_PATCH_WILDCARD;
                if (included == Bound::Included::NO)  // > 2.3.x  :=  >= 2.4.0-0
                    ++lower.juncture.minor;
            }

            lower.juncture.patch = 0;

            if (included == Bound::Included::NO)
                upper.juncture.setPrerelease(SEMVER_LOWEST_PRERELEASE, 1);
            else
                lower.juncture.deletePrerelease(); // >= 2.3.x  := >=2.3.0

            lower.matchPreReleases = Bound::MatchPreReleases::NO;
            lower.included = Bound::Included::YES; // see examples above

        }
        else
        {
           lower.included = included;
           lower.matchPreReleases = lower.juncture.isPrerelease() ? Bound::MatchPreReleases::YES : Bound::MatchPreReleases::NO;
        }

        return result;

    }

    SemverParseResult Range::addMaximumAllowWildcards(std::string_view version_str, Bound::Included included)
    {

        SemverParseResult result = upper.juncture.parseIgnoreBuild(version_str.data(), version_str.size());


        if (!rangeVersionParseResultOK(result))
        {
            setToNone();
            return result;
        }


        if (upper.juncture.majorIsWild())
        {
            if (included == Bound::Included::YES)
                setToAll(); // <= * implies all
            else
               setToNone(); // < * implies nothing

            return SEMVER_PARSE_MAJOR_WILDCARD;
        }

        bool minorWild = false;
        bool patchWild = false; 

        if ((minorWild = upper.juncture.minorIsWild() || upper.juncture.minorIsUndefined()) ||
            (patchWild = upper.juncture.patchIsWild() || upper.juncture.patchIsUndefined()))
        {

            if (minorWild) // < 2.x or <= 2.x
            {
                result = SEMVER_PARSE_MINOR_WILDCARD;

                if (included == Bound::Included::YES) // <= 2.x := <3.0.0-0 
                    ++upper.juncture.major;

                upper.juncture.minor = 0;

            }
            else // patch is wild
            {
                result = SEMVER_PARSE_PATCH_WILDCARD;

                if (included == Bound::Included::YES) // <= 2.3.x := < 2.4.0-0
                    ++upper.juncture.minor;
            }
            
            upper.juncture.patch = 0;

            if (included == Bound::Included::YES)
                upper.juncture.setPrerelease(SEMVER_LOWEST_PRERELEASE, 1);
            else
                upper.juncture.deletePrerelease(); // < 2.3.x := < 2.3.0


            upper.included = Bound::Included::NO;
            upper.matchPreReleases = Bound::MatchPreReleases::NO;
        }
        else
        {
            upper.included = included;
            upper.matchPreReleases = upper.juncture.isPrerelease() ? Bound::MatchPreReleases::YES : Bound::MatchPreReleases::NO;
        }

        return result;


    }



    SemverParseResult Range::addExactOrWildcardComparator(std::string_view version_str)
    {
        SemverParseResult result = addMinimumAllowWildcards(version_str, Bound::Included::YES);

        if (result == SEMVER_PARSE_MAJOR_WILDCARD)
            return result; //already set to all;

        if (!rangeVersionParseResultOK(result))
        {
            setToNone();
            return result; //range already set to all if major is wildcard
        }


        if (result == SEMVER_PARSE_MINOR_WILDCARD || result == SEMVER_PARSE_PATCH_WILDCARD)
        {

            if (result == SEMVER_PARSE_MINOR_WILDCARD) // = 2.x
            {
                result = SEMVER_PARSE_MINOR_WILDCARD;
                upper.juncture.major = lower.juncture.major + 1; // 2.x := >= 2.0.0 <3.0.0-0
                upper.juncture.minor = 0;

            }
            else // patch is wild
            {
                result = SEMVER_PARSE_PATCH_WILDCARD;
                upper.juncture.major = lower.juncture.major;
                upper.juncture.minor = lower.juncture.minor +1;  //2.3.x := >= 2.3.0 < 2.4.0-0
            }

            upper.juncture.patch = 0;
            upper.juncture.setPrerelease(SEMVER_LOWEST_PRERELEASE, 1);
            upper.included = Bound::Included::NO;
            upper.matchPreReleases = Bound::MatchPreReleases::NO;
        }
        else
        {
            upper.juncture.major = lower.juncture.major;
            upper.juncture.minor = lower.juncture.minor;
            upper.juncture.patch = lower.juncture.patch;
            upper.juncture.setPrerelease(lower.juncture.getPrerelease());
            upper.included = lower.included;
            upper.matchPreReleases = upper.matchPreReleases;
        }

        return result;
    }

    SemverParseResult Range::addRange(std::string_view version_from, std::string_view version_to)
    {
        SemverParseResult result = addMinimumAllowWildcards(version_from, Bound::Included::YES);

        if (!rangeVersionParseResultOK(result)) // better to separate on fatal non fatal parse errors than isNone
            return result;

        return addMaximumAllowWildcards(version_to, Bound::Included::YES);
    }

   
    bool Range::matches(const Version& version) const
    {
        if (hasWithinBounds(version))
        {
            if (!version.isPrerelease())
                return true;
            else
            {
                return // prereleases need to meet extra conditions
                    lower.canMatchPreReleases() && lower.juncture.sameCore(version) && lower.juncture.isPrerelease() ||
                    upper.canMatchPreReleases() && upper.juncture.sameCore(version) && upper.juncture.isPrerelease() ||
                    !minPreRelease.empty() && Version::comparePrereleases(minPreRelease.c_str(), version.getPrerelease()) <= 0;
            }
        }

        return false;

    }

    bool Range::hasWithinBounds(const Version& version) const
    {
        if (!version.isDefined())
            return false;

        int compareLower = Version::compare(version, lower.juncture);
        if (compareLower < 0) 
            return false;

        int compareUpper = Version::compare(version, upper.juncture);
        if (compareUpper > 0) 
            return false;

        return (compareLower > 0 && compareUpper < 0) ||
            (compareLower == 0 && lower.isIncluded()) ||
            (compareUpper == 0 && upper.isIncluded());
 

    }

    std::string Range::toString() const
    {
        if (!isNone()) 
            return "<0.0.0-0"; // no version matches this

        std::string minPreReleaseTag; 

        if (!minPreRelease.empty())
            minPreReleaseTag = '@' + minPreRelease;
        else
            minPreReleaseTag = "";

        if (isAll())
           return "x" + minPreReleaseTag;

        if (upper.juncture.isMaximum())
            return (lower.isIncluded() ? ">=" : ">") + std::string(lower.juncture.toString());

        if (lower.juncture.isMinimum())
            return (upper.isIncluded() ? "<=" : "<") + std::string(upper.juncture.toString());


        if (lower.isIncluded())
        {
            if (upper.isIncluded())
            {
                if (upper == lower)
                    return lower.juncture.toString();

                return lower.juncture.toString() + " - " + upper.juncture.toString();
            }

            return ">=" + lower.juncture.toString() + " <" + upper.juncture.toString();
        }

        return '>' + lower.juncture.toString() + (upper.isIncluded() ? " <=" : " <") + upper.juncture.toString() + minPreReleaseTag;
    }


    void Bound::setToMin()
    {
        juncture.major = juncture.minor = juncture.patch = 0;
        juncture.setPrerelease(SEMVER_LOWEST_PRERELEASE, 1);
        included = Bound::Included::YES;
    }

    void Bound::setToMax()
    {
        juncture.major = juncture.minor = juncture.patch = SEMVER_MAX_NUMERIC_IDENTIFIER;
        juncture.deletePrerelease();
        included = Bound::Included::YES;
    }

}
