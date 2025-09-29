// Copyright 2025 Jasper Schellingerhout. All rights reserved.

#include "version.h"
#include <cstring>
#include <unordered_map>

namespace semver
{

	inline static bool strEmpty(const char* str)
	{
		return !(str && *str);
	}


	static size_t getCharPos(char c, const char* buffer, size_t len)
	{
		for (size_t i = 0; i < len; ++i)
			if (buffer[i] == c)
				return i;

		return len; // consider vs -1
	}
	
	static size_t getCharPosEx(char c, const char* buffer, size_t len, size_t start)
	{
		for (size_t i = start; i < len; ++i)
			if (buffer[i] == c)
				return i;

		return len; // consider vs -1
	}

	static size_t getDotPos(const char* buffer, size_t len)
	{
		for (size_t i = 0; i < len; ++i)
			if (buffer[i] == '.')
				return i;

		return len; // consider vs -1
	}

	static size_t getDotPosEx(const char* buffer, size_t len, size_t start)
	{
		for (size_t i = start; i < len; ++i)
			if (buffer[i] == '.')
				return i;

		return len; // consider vs -1
	}

	char* cloneStr(const char* src, size_t len)
	{
		if (len == 0)
			return nullptr;

		char* copy = new char[len + 1];
		std::memcpy(copy, src, len);
		copy[len] = '\0'; // set the null character manually because src may have been a slice
		return copy;
	}

	static std::unordered_map<const Version*, const char*> sbuild_metadata;

	inline static bool isWildcardCharacter(char c)
	{
		return (c == 'X' || c == 'x' || c == '*');
	}

	enum class NumericIdentifierParseResult : uint8_t
	{
		SUCCESS,
		EMPTY_IDENTIFIER,
		LEADING_ZERO,
		NON_DIGIT_CHARACTER,
		WILDCARD
	};

	static NumericIdentifierParseResult parseNumericIdentifier(const char* identifier, size_t len, uint64_t& value)
	{

		if (!identifier || len == 0)
			return NumericIdentifierParseResult::EMPTY_IDENTIFIER;

		// single digits are common
		if (len == 1)
		{
			if (isWildcardCharacter(identifier[0]))
			{
				value = SEMVER_WILDCARD_IDENTIFIER;
				return NumericIdentifierParseResult::WILDCARD; // single character wildcard
			}

			if ((identifier[0] < '0') || (identifier[0] > '9'))
				return NumericIdentifierParseResult::NON_DIGIT_CHARACTER;

			value = static_cast<uint64_t>(identifier[0]) - '0'; // Convert single digit character to numeric value
			return NumericIdentifierParseResult::SUCCESS; // Single digit is valid
		}

		// At this point more than one character, so no wildcards
		if (identifier[0] == '0')
			return NumericIdentifierParseResult::LEADING_ZERO; // Leading zero is not allowed for numeric identifiers with more than one digit

		value = 0; // Initialize value to zero
		for (size_t i = 0; i < len; ++i)
		{
			char c = identifier[i];
			if ((c < '0') || (c > '9'))
				return NumericIdentifierParseResult::NON_DIGIT_CHARACTER; // If any character is not a digit, return NON_DIGIT_CHARACTER. 

			value = value * 10 + (static_cast<uint64_t>(c) - '0');
		}

		return NumericIdentifierParseResult::SUCCESS;
	}

	enum class IDENTIFIER_PART_TYPE : uint8_t
	{
		UNKNOWN,
		EMPTY,
		ALPHANUMERIC,
		DIGITS,
		NUMERIC
	};


	static IDENTIFIER_PART_TYPE getIdentifierType(const char* identifier, size_t len)
	{
		if (!identifier)
			return IDENTIFIER_PART_TYPE::EMPTY;

		IDENTIFIER_PART_TYPE result = IDENTIFIER_PART_TYPE::DIGITS;

		for (size_t i = 0; i < len; ++i)
		{
			char c = identifier[i];
			bool isDigit = (c >= '0') && (c <= '9');

			bool isNonDigit = ((c >= 'A') && (c <= 'Z'))
				|| ((c >= 'a') && (c <= 'z'))
				|| (c == '-');

			if (!isDigit && !isNonDigit)
				return IDENTIFIER_PART_TYPE::UNKNOWN;

			if (!isDigit && result == IDENTIFIER_PART_TYPE::DIGITS) // we initilized result to DIGITS
				result = IDENTIFIER_PART_TYPE::ALPHANUMERIC;
		}

		// if we still have digits, we check if it start with zero. If not, or if it is a single digit, we return NUMERIC
		if ((result == IDENTIFIER_PART_TYPE::DIGITS) && ((len == 1) || (identifier[0] != '0')))
			result = IDENTIFIER_PART_TYPE::NUMERIC;

		return result;
	}


	void Version::clear()
	{
		deletePrerelease();
		deleteBuild();
		std::memset(this, 0, sizeof(Version));
	}

	SemverParseResult Version::parseInternal(const char* str, size_t len, bool ignoreBuild, // I only approve of boolean arguments on internal methods
		UninitializedDefault uninitializedDefault = UninitializedDefault::TAG)
	{

		SemverParseResult result = SEMVER_PARSE_SUCCESS;

		if (flags & Flags::MANAGED)
		{
			clear();
			flags |= Flags::MANAGED; //restore the flag
		}
		else
			clear();

		if (uninitializedDefault == UninitializedDefault::TAG)
		{
			major = minor = patch = SEMVER_UNINITIALIZED_IDENTIFIER;
			inline_prerelease[0] = SEMVER_UNINITIALIZED_LABEL[0]; // memset made the rest \0

			flags |= BUILD_UNDEFINED;
		}
		else if (uninitializedDefault == UninitializedDefault::WILD)
		{
			major = minor = patch = SEMVER_WILDCARD_IDENTIFIER;
		}

		size_t start = 0;

		if (strEmpty(str))
			return  SEMVER_PARSE_EMPTY_VERSION_STRING; //terminal


		if (str[start] <= ' ')
		{
			result = SEMVER_PARSE_LEADING_WHITESPACE; // non terminal

			while (str[start] <= ' ' && start < len - 1)
				++start;

			if (start == len - 1)
				return SEMVER_PARSE_EMPTY_VERSION_STRING; //only whitespace, but seems appropriate
		}

		const char* versionstr = str + start; // versionstr starts after whitespace
		len -= start;
		// beyond this point use versionstr instead of str

		if (versionstr[len - 1] <= ' ')
		{
			result = SEMVER_PARSE_TRAILING_WHITESPACE; // non terminal

			while (versionstr[len - 1] <= ' ' && len > 1)
				--len;
		}

		// end is just past end of version or at first whitespace

		if (isWildcardCharacter(versionstr[0])) // If the first character is a wildcard, we don't check the rest of the string 
		{

			if (len == 1 || versionstr[1] == '.')
			{
				major = SEMVER_WILDCARD_IDENTIFIER;
				return SEMVER_PARSE_MAJOR_WILDCARD;
			}

			return SEMVER_PARSE_MAJOR_NOT_NUMERIC; // we are sure if wild becase it was not a single widcard char
		}


		size_t dot1 = getDotPos(versionstr, len);

		switch (parseNumericIdentifier(versionstr, dot1, major)) // dot1 is also majorLength
		{
		case NumericIdentifierParseResult::SUCCESS:
			if (major > SEMVER_MAX_NUMERIC_IDENTIFIER)
				return SEMVER_PARSE_MAJOR_TOO_LARGE;

			break;

		case NumericIdentifierParseResult::EMPTY_IDENTIFIER:
			return SEMVER_PARSE_MAJOR_EMPTY;

		case NumericIdentifierParseResult::LEADING_ZERO:
			return SEMVER_PARSE_MAJOR_LEADING_ZERO;

		case NumericIdentifierParseResult::NON_DIGIT_CHARACTER:
			return SEMVER_PARSE_MAJOR_NOT_NUMERIC;

		case NumericIdentifierParseResult::WILDCARD:
			return SEMVER_PARSE_MAJOR_WILDCARD; // if we do not allow wildcards, return an error
		}

		if (dot1 == len)
			return SEMVER_PARSE_TOO_FEW_PARTS;


		size_t dot2 = getDotPosEx(versionstr, len, dot1 + 1); // cannot have two adjacent dots
		size_t minorLength = dot2 - dot1 - 1;

		switch (parseNumericIdentifier(versionstr + dot1 + 1, minorLength, minor))
		{
		case NumericIdentifierParseResult::SUCCESS:
			if (minor > SEMVER_MAX_NUMERIC_IDENTIFIER) // wildcards also satisfy this criteria, but parsing reports wildcard not success in that case
				return SEMVER_PARSE_MINOR_TOO_LARGE;

			if (dot2 == len) //never saw a second dot, so the number is 1.2 form
				return(SEMVER_PARSE_TOO_FEW_PARTS);

			break;

		case NumericIdentifierParseResult::EMPTY_IDENTIFIER:
			return SEMVER_PARSE_MINOR_EMPTY;

		case NumericIdentifierParseResult::LEADING_ZERO:
			return SEMVER_PARSE_MINOR_LEADING_ZERO;

		case NumericIdentifierParseResult::NON_DIGIT_CHARACTER:
			return SEMVER_PARSE_MINOR_NOT_NUMERIC;

		case NumericIdentifierParseResult::WILDCARD:
			return SEMVER_PARSE_MINOR_WILDCARD; // if we do not allow wildcards, return an error
		}

		//const char* versionremainder = versionstr + dot2 + 1;
		//size_t remainderlen = len - dot2 - 1;


		size_t buildPos = getCharPosEx('+', versionstr, len, dot2 + 1); // + is followed by build
		bool hasBuild = (buildPos != len);

		size_t prereleasePos = getCharPosEx('-', versionstr, len, dot2 + 1); // followed by pre-release and optionally a + and the build
		bool hasPrerelease = prereleasePos < buildPos; // buildpos is len if not found


		size_t patchEnd;

		if (hasPrerelease)
			patchEnd = prereleasePos;
		else if (hasBuild)
			patchEnd = buildPos;
		else
		{
			patchEnd = len;

			if (getDotPosEx(versionstr, len, dot2 + 1) != len)
				return SEMVER_PARSE_TOO_MANY_PARTS;
		}

		switch (parseNumericIdentifier(versionstr + dot2 + 1, patchEnd - dot2 - 1, patch))
		{
		case NumericIdentifierParseResult::SUCCESS:
			if (patch > SEMVER_MAX_NUMERIC_IDENTIFIER)
				return SEMVER_PARSE_PATCH_TOO_LARGE;
			break;

		case NumericIdentifierParseResult::EMPTY_IDENTIFIER:
			return SEMVER_PARSE_PATCH_EMPTY;

		case NumericIdentifierParseResult::LEADING_ZERO:
			return SEMVER_PARSE_PATCH_LEADING_ZERO;

		case NumericIdentifierParseResult::NON_DIGIT_CHARACTER:
			return SEMVER_PARSE_PATCH_NOT_NUMERIC;

		case NumericIdentifierParseResult::WILDCARD:
			return SEMVER_PARSE_PATCH_WILDCARD; // if we do not allow wildcards, return an error
		}


		if (hasPrerelease)
		{
			size_t pre_release_len = buildPos - prereleasePos - 1; // if no build, builtPos is set to the remainderLength

			PreleaseParseResult preReleaseParseResult = trySetPrerelease(versionstr + prereleasePos + 1, pre_release_len);

			if (preReleaseParseResult != PreleaseParseResult::SUCCESS)
				return static_cast<SemverParseResult>(preReleaseParseResult);

		}
		else
			inline_prerelease[0] = '\0'; // if we set first char to ? rest it to \0

		if (hasBuild && !ignoreBuild)
		{
			size_t build_len = len - buildPos - 1;
			BuildParseResult buildParseResult = trySetBuild(versionstr + buildPos + 1, build_len);

			if (buildParseResult != BuildParseResult::SUCCESS)
				return static_cast<SemverParseResult>(buildParseResult);

		}
		else
			flags &= ~(BUILD_UNDEFINED | BUILD_IN_MAP);

		return result;
	}


	Version::PreleaseParseResult Version::trySetPrerelease(const char* str, size_t len)
	{
		auto result = Version::parsePrerelease(str, len);

		if (result == PreleaseParseResult::SUCCESS)
			setPrerelease(str, len);
		else
		    setPrerelease(SEMVER_UNINITIALIZED_LABEL, 1);
	
		return result;
	}

	Version::PreleaseParseResult Version::parsePrerelease(const char* buffer, size_t len)
	{
		if (len == 0)
			return PreleaseParseResult::SUCCESS; // empty prerelease is valid

		size_t start = 0;
		do
		{
			size_t remaining_len = len - start;
			size_t dotpos = getDotPos(buffer + start, remaining_len);


			if (dotpos == 0 || dotpos == remaining_len - 1) // last character is a dot
				return PreleaseParseResult::EMPTY_IDENTIFIER;

			size_t identifierlen = dotpos == -1 ? remaining_len : dotpos;

			// must be alphanumeric, but if all digits it must be numeric (cannot start with 0 if more than one character)
			auto identifierType = getIdentifierType(buffer + start, identifierlen);
			switch (identifierType)
			{
			case IDENTIFIER_PART_TYPE::ALPHANUMERIC:
			case IDENTIFIER_PART_TYPE::NUMERIC: // exludes digits with leading zero
				break;

			case IDENTIFIER_PART_TYPE::EMPTY:
				return PreleaseParseResult::EMPTY_IDENTIFIER;

			case IDENTIFIER_PART_TYPE::DIGITS: // we distinguish between digits and numeric when parsing (digits without a leading zero are numeric)
				return PreleaseParseResult::DIGITS_WITH_LEADING_ZERO;


			default: //case IDENTIFIER_PART_TYPE::UNKNOWN:
				return PreleaseParseResult::UNSUPPORTED_CHARACTER;
			}


			start += identifierlen + 1; //also skip the dot
		} while (start < len);

		return PreleaseParseResult::SUCCESS;
	}

	void Version::setPrerelease(const char* str, size_t len)
	{
		deletePrerelease(); // also sets as inline
		if (len < inline_prerelease_len)
		{
			if (len > 0)
				memcpy(inline_prerelease, str, len);

			inline_prerelease[len] = '\0';
		}
		else
		{
			setHeapPrerelease(cloneStr(str, len));
		}
	}


	void Version::deletePrerelease()
	{
		if (isPrereleaseOnHeap())
		{
			delete[] get_heap_prerelease();

			flags &= ~PRERELEASE_ON_HEAP;
		}

		std::memset(inline_prerelease, 0, inline_prerelease_len);

	}

	Version::BuildParseResult Version::trySetBuild(const char* str, size_t len)
	{
		BuildParseResult result = Version::parseBuild(str, len);

		if (result == BuildParseResult::SUCCESS)
			setBuild(str, len);
		else
		{
			deleteBuild();
			flags |= BUILD_UNDEFINED; //undefined := '?'
		}

		return result;
	}

	Version::BuildParseResult Version::parseBuild(const char* buffer, size_t len)
	{
		if (len == 0)
			return BuildParseResult::SUCCESS; // empty prerelease is valid

		size_t start = 0;
		do
		{
			size_t remaining_len = len - start;
			size_t dotpos = getDotPos(buffer + start, remaining_len);


			if (dotpos == 0 || dotpos == remaining_len - 1) // first character of identifier is dot (two dots in a row) or last character is a dot
				return BuildParseResult::EMPTY_IDENTIFIER;


			size_t identifierlen = dotpos == -1 ? remaining_len : dotpos;

			// must be alphanumeric, but if all digits it must be numeric (cannot start with 0 if more than one character)
			auto identifierType = getIdentifierType(buffer + start, identifierlen);
			switch (identifierType)
			{
			case IDENTIFIER_PART_TYPE::ALPHANUMERIC:
			case IDENTIFIER_PART_TYPE::DIGITS:
			case IDENTIFIER_PART_TYPE::NUMERIC:
				break;


			case IDENTIFIER_PART_TYPE::EMPTY:
				return BuildParseResult::EMPTY_IDENTIFIER;

			default: //case IDENTIFIER_PART_TYPE::UNKNOWN:
				return BuildParseResult::UNSUPPORTED_CHARACTER;
			}


			start += identifierlen + 1; //also skip the dot
		} while (start < len);

		return BuildParseResult::SUCCESS;

	}

	void Version::setBuild(const char* str, size_t len)
	{
		flags &= ~BUILD_UNDEFINED;

		if (len == 0 || (len == 1 && str[0] == SEMVER_UNINITIALIZED_LABEL[0]))
		{
			if (hasBuild())
			{
				sbuild_metadata.erase(this);
				flags &= ~BUILD_IN_MAP;
			}

			if (len > 0) // SEMVER_UNINITIALIZED_LABEL
				flags |= BUILD_UNDEFINED;

			return;
		}

		if (hasBuild())
			delete[] sbuild_metadata[this]; //data not the entry

		sbuild_metadata[this] = cloneStr(str, len);


		flags |= BUILD_IN_MAP;
	}

	void Version::deleteBuild()
	{
		if (hasBuild())
		{
			delete[] sbuild_metadata[this];
			sbuild_metadata.erase(this);

			flags &= ~BUILD_IN_MAP;
		};
	}

	 bool Version::hasPrerelease() const
	{
		 return isPrereleaseOnHeap() ||
			 inline_prerelease[0] != SEMVER_UNINITIALIZED_LABEL[0] &&
			 inline_prerelease[0] != '\0';

	}

	const char* Version::getBuild() const
	{
		if (hasBuild())
			return sbuild_metadata[this];
		else if (buildIsUndefined())
			return SEMVER_UNINITIALIZED_LABEL;
		else
			return SEMVER_EMPTY_LABEL;
	}

	static bool addNumericIdentifierToString(std::string& str, uint64_t identifier)
	{
		if (!str.empty())
			str += '.';

		if (identifier > SEMVER_MAX_NUMERIC_IDENTIFIER)
		{
			str += (identifier == SEMVER_WILDCARD_IDENTIFIER ? 'x' : SEMVER_UNINITIALIZED_LABEL[0]);
			return false;
		}

		str += std::to_string(identifier);
		return true;
	}

	std::string Version::toString() const
	{
		std::string s;
		if (addNumericIdentifierToString(s, major))
			if (addNumericIdentifierToString(s, minor))
				addNumericIdentifierToString(s, patch);


		if (hasPrerelease())
			s += '-' + std::string(getPrerelease());

		if (hasBuild())
			s += '+' + std::string(getBuild());

		return s;

	}

	inline static bool isValidNumericIdentifier (const char* identifier, size_t len, uint64_t& value)
	{
		return parseNumericIdentifier(identifier, len, value) == NumericIdentifierParseResult::SUCCESS;
	}

	static int compareNonNumericIdentifiers(const char* lhs, size_t lhsLen, const char* rhs, size_t rhsLen)
	{
		size_t compare_len = lhsLen <= rhsLen ? lhsLen : rhsLen;

		for (size_t i = 0; i < compare_len; ++i)
		{
			if (lhs[i] != rhs[i])
				return (lhs[i] < rhs[i]) ? -1 : 1;
		}

		if (lhsLen != rhsLen) 
			return (lhsLen < rhsLen) ? -1 : 1;
	
		return 0;

	}


	int Version::comparePrereleases(const char* lhs, const char* rhs)
	{

		if (lhs == rhs)
			return 0;

		if (!lhs || !(*lhs)) // no pre-release > pre-release
			return -1;

		if (!rhs || !(*rhs))
			return 1;

		size_t lhs_start = 0;
		size_t lhs_len = strlen(lhs);
		size_t rhs_start = 0;
		size_t rhs_len = strlen(rhs);

		do
		{
			size_t lhs_remaining_len = lhs_len - lhs_start;
			size_t rhs_remaining_len = rhs_len - rhs_start;


			size_t lhs_dotpos = getDotPos(lhs + lhs_start, lhs_remaining_len);
			size_t rhs_dotpos = getDotPos(rhs + rhs_start, rhs_remaining_len);


			size_t lhs_identiferlen = lhs_dotpos == -1 ? lhs_remaining_len : lhs_dotpos;
			size_t rhs_identiferlen = rhs_dotpos == -1 ? rhs_remaining_len : rhs_dotpos;

			uint64_t lhs_numeric_identifier;
			bool lhs_numeric = isValidNumericIdentifier(lhs + lhs_start, lhs_identiferlen, lhs_numeric_identifier);

			uint64_t rhs_numeric_identifier;
			bool rhs_numeric = isValidNumericIdentifier(rhs + rhs_start, rhs_identiferlen, rhs_numeric_identifier);


			if (lhs_numeric && rhs_numeric)
			{
				if (lhs_numeric_identifier != rhs_numeric_identifier)
					return lhs_numeric_identifier < rhs_numeric_identifier ? -1 : 1;
			}
			else
			{
				if (lhs_numeric) // if one is numeric it has lower precendence
					return -1;

				if (rhs_numeric)
					return 1;

				int comparison = compareNonNumericIdentifiers(lhs + lhs_start, lhs_identiferlen,
															  rhs + rhs_start, rhs_identiferlen);

				if (comparison != 0)
					return comparison; // else we need to check for more identifiers e.g. alpha.1 vs alpha.2 should not exit with 0 here
			}


			lhs_start += lhs_identiferlen + 1; //also skip the dot with +1
			rhs_start += rhs_identiferlen + 1;


		} while ((lhs_start < lhs_len) && (rhs_start < rhs_len));

		if (lhs_len != rhs_len)
			return lhs_len < rhs_len ? -1 : 1; // longer chains have higher precedence if all earlier sections match

		return 0;
	}


	int Version::compare(const Version& lhs, const Version& rhs)
	{
		if (lhs.major != rhs.major)
			return (lhs.major < rhs.major) ? -1 : 1;

		if (lhs.minor != rhs.minor)
			return (lhs.minor < rhs.minor) ? -1 : 1;

		if (lhs.patch != rhs.patch)
			return (lhs.patch < rhs.patch) ? -1 : 1;


		if (memcmp(lhs.inline_prerelease, rhs.inline_prerelease, inline_prerelease_len) == 0) //this means that if we set the heap prerelease we need to also clear the characters in front of the pointer
			return 0;

		// prerelease have lower precedence than non-prerelease
		// consider pre-releases a negative offset to the version to the left of them

		if (lhs.isPrerelease() && !rhs.isPrerelease())
			return -1; // lhs is prerelease, rhs is not, so lhs < rhs

		if (!lhs.isPrerelease() && rhs.isPrerelease())
			return 1; // lhs is not prerelease, rhs is, so lhs > rhs


		return comparePrereleases(lhs.getPrerelease(), rhs.getPrerelease()); // versions are equal
	}





}


