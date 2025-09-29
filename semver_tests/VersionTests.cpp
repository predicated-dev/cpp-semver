// Copyright 2025 Jasper Schellingerhout. All rights reserved.

#include "pch.h"
#include "gtest/gtest.h"
#include "API/semver.h"

TEST(SemverVersion, ParseValidVersion)
{
    HSemverVersion version = semver_version_create();

    auto result = semver_version_parse(version, "1.2.3-alpha+build");
    EXPECT_EQ(result, SEMVER_PARSE_SUCCESS);

    EXPECT_EQ(semver_get_version_major(version), 1);
    EXPECT_EQ(semver_get_version_minor(version), 2);
    EXPECT_EQ(semver_get_version_patch(version), 3);
    EXPECT_STREQ(semver_get_version_prerelease(version), "alpha");
    EXPECT_STREQ(semver_get_version_build(version), "build");

    result = semver_version_parse(version, "1.0.0-alpha");
    EXPECT_EQ(result, SEMVER_PARSE_SUCCESS);
    EXPECT_EQ(semver_get_version_major(version), 1);
    EXPECT_EQ(semver_get_version_minor(version), 0);
    EXPECT_EQ(semver_get_version_patch(version), 0);
    EXPECT_STREQ(semver_get_version_prerelease(version), "alpha");
    EXPECT_STREQ(semver_get_version_build(version), "");

    result = semver_version_parse(version, "1.0.0-alpha.1");
    EXPECT_EQ(result, SEMVER_PARSE_SUCCESS);
    EXPECT_EQ(semver_get_version_major(version), 1);
    EXPECT_EQ(semver_get_version_minor(version), 0);
    EXPECT_EQ(semver_get_version_patch(version), 0);
    EXPECT_STREQ(semver_get_version_prerelease(version), "alpha.1");
    EXPECT_STREQ(semver_get_version_build(version), "");

    result = semver_version_parse(version, "1.0.0-0.3.7");
    EXPECT_EQ(result, SEMVER_PARSE_SUCCESS);
    EXPECT_EQ(semver_get_version_major(version), 1);
    EXPECT_EQ(semver_get_version_minor(version), 0);
    EXPECT_EQ(semver_get_version_patch(version), 0);
    EXPECT_STREQ(semver_get_version_prerelease(version), "0.3.7");
    EXPECT_STREQ(semver_get_version_build(version), "");

    result = semver_version_parse(version, "1.0.0-x.7.z.92");
    EXPECT_EQ(result, SEMVER_PARSE_SUCCESS);
    EXPECT_EQ(semver_get_version_major(version), 1);
    EXPECT_EQ(semver_get_version_minor(version), 0);
    EXPECT_EQ(semver_get_version_patch(version), 0);
    EXPECT_STREQ(semver_get_version_prerelease(version), "x.7.z.92");
    EXPECT_STREQ(semver_get_version_build(version), "");

    result = semver_version_parse(version, "1.0.0-x-y-z.--");
    EXPECT_EQ(result, SEMVER_PARSE_SUCCESS);
    EXPECT_EQ(semver_get_version_major(version), 1);
    EXPECT_EQ(semver_get_version_minor(version), 0);
    EXPECT_EQ(semver_get_version_patch(version), 0);
    EXPECT_STREQ(semver_get_version_prerelease(version), "x-y-z.--");
    EXPECT_STREQ(semver_get_version_build(version), "");

    result = semver_version_parse(version, "1.0.0-alpha+001");
    EXPECT_EQ(result, SEMVER_PARSE_SUCCESS);
    EXPECT_EQ(semver_get_version_major(version), 1);
    EXPECT_EQ(semver_get_version_minor(version), 0);
    EXPECT_EQ(semver_get_version_patch(version), 0);
    EXPECT_STREQ(semver_get_version_prerelease(version), "alpha");
    EXPECT_STREQ(semver_get_version_build(version), "001");


    result = semver_version_parse(version, "1.0.0+20130313144700");
    EXPECT_EQ(result, SEMVER_PARSE_SUCCESS);
    EXPECT_EQ(semver_get_version_major(version), 1);
    EXPECT_EQ(semver_get_version_minor(version), 0);
    EXPECT_EQ(semver_get_version_patch(version), 0);
    EXPECT_STREQ(semver_get_version_prerelease(version), "");
    EXPECT_STREQ(semver_get_version_build(version), "20130313144700");


    result = semver_version_parse(version, "1.0.0-beta+exp.sha.5114f85");
    EXPECT_EQ(result, SEMVER_PARSE_SUCCESS);
    EXPECT_EQ(semver_get_version_major(version), 1);
    EXPECT_EQ(semver_get_version_minor(version), 0);
    EXPECT_EQ(semver_get_version_patch(version), 0);
    EXPECT_STREQ(semver_get_version_prerelease(version), "beta");
    EXPECT_STREQ(semver_get_version_build(version), "exp.sha.5114f85");


    result = semver_version_parse(version, "1.0.0+21AF26D3----117B344092BD");
    EXPECT_EQ(result, SEMVER_PARSE_SUCCESS);
    EXPECT_EQ(semver_get_version_major(version), 1);
    EXPECT_EQ(semver_get_version_minor(version), 0);
    EXPECT_EQ(semver_get_version_patch(version), 0);
    EXPECT_STREQ(semver_get_version_prerelease(version), "");
    EXPECT_STREQ(semver_get_version_build(version), "21AF26D3----117B344092BD"); // - must be before + to denote prerelease
    semver_version_dispose(version);
}

TEST(SemverVersion, ParseValidVersionNullSeparated)
{
    HSemverVersions versions = semver_versions_from_string("1.2.3-alpha+build", nullptr, SEMVER_ORDER_AS_GIVEN);
    EXPECT_EQ(semver_versions_count(versions), 1);
    HSemverVersion version = semver_versions_get_version_at_index(versions, 0);
   
    EXPECT_EQ(semver_get_version_major(version), 1);
    EXPECT_EQ(semver_get_version_minor(version), 2);
    EXPECT_EQ(semver_get_version_patch(version), 3);
    EXPECT_STREQ(semver_get_version_prerelease(version), "alpha");
    EXPECT_STREQ(semver_get_version_build(version), "build");

    semver_versions_dispose(versions);

    const char buffer[] =
        "2.3.4-alpha+build\0"
        "5.6.7-1234567890abcdefghij+1234567890ABCDEFGHIJ\0"
        "1.2.3\0\0";

    versions = semver_versions_from_string(buffer, nullptr, SEMVER_ORDER_AS_GIVEN);
    EXPECT_EQ(semver_versions_count(versions), 3);
    version = semver_versions_get_version_at_index(versions, 0);

    EXPECT_EQ(semver_get_version_major(version), 2);
    EXPECT_EQ(semver_get_version_minor(version), 3);
    EXPECT_EQ(semver_get_version_patch(version), 4);
    EXPECT_STREQ(semver_get_version_prerelease(version), "alpha");
    EXPECT_STREQ(semver_get_version_build(version), "build");

    version = semver_versions_get_version_at_index(versions, 1);

    EXPECT_EQ(semver_get_version_major(version), 5);
    EXPECT_EQ(semver_get_version_minor(version), 6);
    EXPECT_EQ(semver_get_version_patch(version), 7);
    EXPECT_STREQ(semver_get_version_prerelease(version), "1234567890abcdefghij");
    EXPECT_STREQ(semver_get_version_build(version), "1234567890ABCDEFGHIJ");

    version = semver_versions_get_version_at_index(versions, 2);
   
    EXPECT_EQ(semver_get_version_major(version), 1);
    EXPECT_EQ(semver_get_version_minor(version), 2);
    EXPECT_EQ(semver_get_version_patch(version), 3);
    EXPECT_STREQ(semver_get_version_prerelease(version), "");
    EXPECT_STREQ(semver_get_version_build(version), "");

    semver_versions_dispose(versions);
 

}

TEST(SemverVersion, ParseValidVersionCSV)
{
    HSemverVersions versions = semver_versions_from_string("1.2.3-alpha+build", ",", SEMVER_ORDER_AS_GIVEN);
    EXPECT_EQ(semver_versions_count(versions), 1);
    HSemverVersion version = semver_versions_get_version_at_index(versions, 0);

    
    EXPECT_EQ(semver_get_version_major(version), 1);
    EXPECT_EQ(semver_get_version_minor(version), 2);
    EXPECT_EQ(semver_get_version_patch(version), 3);
    EXPECT_STREQ(semver_get_version_prerelease(version), "alpha");
    EXPECT_STREQ(semver_get_version_build(version), "build");

    semver_versions_dispose(versions);

    const char buffer[] =
        "2.3.4-alpha+build"
        ","
        "5.6.7-1234567890abcdefghij+1234567890ABCDEFGHIJ"
        ","
        "1.2.3\0";

    versions = semver_versions_from_string(buffer, ",", SEMVER_ORDER_AS_GIVEN);
    EXPECT_EQ(semver_versions_count(versions), 3);
    version = semver_versions_get_version_at_index(versions, 0);

    
    EXPECT_EQ(semver_get_version_major(version), 2);
    EXPECT_EQ(semver_get_version_minor(version), 3);
    EXPECT_EQ(semver_get_version_patch(version), 4);
    EXPECT_STREQ(semver_get_version_prerelease(version), "alpha");
    EXPECT_STREQ(semver_get_version_build(version), "build");

    version = semver_versions_get_version_at_index(versions, 1);

    
    EXPECT_EQ(semver_get_version_major(version), 5);
    EXPECT_EQ(semver_get_version_minor(version), 6);
    EXPECT_EQ(semver_get_version_patch(version), 7);
    EXPECT_STREQ(semver_get_version_prerelease(version), "1234567890abcdefghij");
    EXPECT_STREQ(semver_get_version_build(version), "1234567890ABCDEFGHIJ");

    version = semver_versions_get_version_at_index(versions, 2);

    
    EXPECT_EQ(semver_get_version_major(version), 1);
    EXPECT_EQ(semver_get_version_minor(version), 2);
    EXPECT_EQ(semver_get_version_patch(version), 3);
    EXPECT_STREQ(semver_get_version_prerelease(version), "");
    EXPECT_STREQ(semver_get_version_build(version), "");

    semver_versions_dispose(versions);


}

TEST(SemverVersion, CreateVersionBlock)
{
    HSemverVersions versions = semver_versions_create(3);
    EXPECT_EQ(semver_versions_count(versions), 3);
    HSemverVersion version = semver_versions_get_version_at_index(versions, 0);

    
    EXPECT_EQ(semver_get_version_major(version), 0);
    EXPECT_EQ(semver_get_version_minor(version), 0);
    EXPECT_EQ(semver_get_version_patch(version), 0);
    EXPECT_STREQ(semver_get_version_prerelease(version), "");
    EXPECT_STREQ(semver_get_version_build(version), "");

    version = semver_versions_get_version_at_index(versions, 1);

    
    EXPECT_EQ(semver_get_version_major(version), 0);
    EXPECT_EQ(semver_get_version_minor(version), 0);
    EXPECT_EQ(semver_get_version_patch(version), 0);
    EXPECT_STREQ(semver_get_version_prerelease(version), "");
    EXPECT_STREQ(semver_get_version_build(version), "");

    version = semver_versions_get_version_at_index(versions, 2);

    
    EXPECT_EQ(semver_get_version_major(version), 0);
    EXPECT_EQ(semver_get_version_minor(version), 0);
    EXPECT_EQ(semver_get_version_patch(version), 0);
    EXPECT_STREQ(semver_get_version_prerelease(version), "");
    EXPECT_STREQ(semver_get_version_build(version), "");

    semver_versions_dispose(versions);
}

TEST(SemverVersion, ParseLeadingWhitespaceVersion)
{
    HSemverVersion version = semver_version_create();
    auto result = semver_version_parse(version, " 1.2.3-alpha+build");
    EXPECT_EQ(result, SEMVER_PARSE_LEADING_WHITESPACE);


    result = semver_version_parse(version, "  1.2.3+build.1");
    EXPECT_EQ(result, SEMVER_PARSE_LEADING_WHITESPACE);


    result = semver_version_parse(version, " "); //leading and trailing are non fatal other errors can superceded
    EXPECT_EQ(result, SEMVER_PARSE_EMPTY_VERSION_STRING);


    // leading_whitespace is low priority and replaced by other errors

    result = semver_version_parse(version, " ?<>[]()");
    EXPECT_EQ(result, SEMVER_PARSE_MAJOR_NOT_NUMERIC);
    semver_version_dispose(version);
}

TEST(SemverVersion, ParseTrailingWhitespaceVersion)
{
    HSemverVersion version = semver_version_create();
    auto result = semver_version_parse(version, "1.2.3-alpha+build ");
    EXPECT_EQ(result, SEMVER_PARSE_TRAILING_WHITESPACE);


    result = semver_version_parse(version, "1.2.3+build.1 ");
    EXPECT_EQ(result, SEMVER_PARSE_TRAILING_WHITESPACE);


    result = semver_version_parse(version, "1.2.3-alpha ");
    EXPECT_EQ(result, SEMVER_PARSE_TRAILING_WHITESPACE);


    result = semver_version_parse(version, "1.2.3 ");
    EXPECT_EQ(result, SEMVER_PARSE_TRAILING_WHITESPACE);
    semver_version_dispose(version);
}

TEST(SemverVersion, ParseEmpty)
{
    HSemverVersion version = semver_version_create();
    auto result = semver_version_parse(version, "");
    EXPECT_EQ(result, SEMVER_PARSE_EMPTY_VERSION_STRING);
    semver_version_dispose(version);
}

TEST(SemverVersion, ParseMajorEmpty)
{
    HSemverVersion version = semver_version_create();
    auto result = semver_version_parse(version, ".2.3");
    EXPECT_EQ(result, SEMVER_PARSE_MAJOR_EMPTY);

    result = semver_version_parse(version, "..3");
    EXPECT_EQ(result, SEMVER_PARSE_MAJOR_EMPTY);

    result = semver_version_parse(version, "..");
    EXPECT_EQ(result, SEMVER_PARSE_MAJOR_EMPTY);
    semver_version_dispose(version);
}

TEST(SemverVersion, ParseMinorEmpty)
{
    HSemverVersion version = semver_version_create();
    auto result = semver_version_parse(version, "1..3");
    EXPECT_EQ(result, SEMVER_PARSE_MINOR_EMPTY);

    result = semver_version_parse(version, "1..");
    EXPECT_EQ(result, SEMVER_PARSE_MINOR_EMPTY);
    semver_version_dispose(version);
}

TEST(SemverVersion, ParsePatchEmpty)
{
    HSemverVersion version = semver_version_create();
    auto result = semver_version_parse(version, "1.2.");
    EXPECT_EQ(result, SEMVER_PARSE_PATCH_EMPTY);
    semver_version_dispose(version);
}

TEST(SemverVersion, ParseTooFewParts)
{
    HSemverVersion version = semver_version_create();
    auto result = semver_version_parse(version, "1.2");
    EXPECT_EQ(result, SEMVER_PARSE_TOO_FEW_PARTS);


    result = semver_version_parse(version, "2");
    EXPECT_EQ(result, SEMVER_PARSE_TOO_FEW_PARTS);
    semver_version_dispose(version);
}

TEST(SemverVersion, ParseTooManyParts)
{
    HSemverVersion version = semver_version_create();
    auto result = semver_version_parse(version, "1.2.3.4");
    EXPECT_EQ(result, SEMVER_PARSE_TOO_MANY_PARTS);
    semver_version_dispose(version);
}

TEST(SemverVersion, ParseMajorLeadingzero)
{
    HSemverVersion version = semver_version_create();
    auto result = semver_version_parse(version, "01.2.3-alpha+build");
    EXPECT_EQ(result, SEMVER_PARSE_MAJOR_LEADING_ZERO);


    result = semver_version_parse(version, "01.2.3+build.1");
    EXPECT_EQ(result, SEMVER_PARSE_MAJOR_LEADING_ZERO);


    result = semver_version_parse(version, "01.2.3-alpha");
    EXPECT_EQ(result, SEMVER_PARSE_MAJOR_LEADING_ZERO);


    result = semver_version_parse(version, "01.2.3");
    EXPECT_EQ(result, SEMVER_PARSE_MAJOR_LEADING_ZERO);


    result = semver_version_parse(version, "01");
    EXPECT_EQ(result, SEMVER_PARSE_MAJOR_LEADING_ZERO);


    // 0 by itself is fine
    result = semver_version_parse(version, "0.0.0");
    EXPECT_EQ(result, SEMVER_PARSE_SUCCESS);


    result = semver_version_parse(version, "0.");
    EXPECT_EQ(result, SEMVER_PARSE_MINOR_EMPTY);
    semver_version_dispose(version);
}

TEST(SemverVersion, ParseMinorLeadingzero)
{
    HSemverVersion version = semver_version_create();
    auto result = semver_version_parse(version, "1.02.3-alpha+build");
    EXPECT_EQ(result, SEMVER_PARSE_MINOR_LEADING_ZERO);


    result = semver_version_parse(version, "120.002.3+build.1");
    EXPECT_EQ(result, SEMVER_PARSE_MINOR_LEADING_ZERO);


    result = semver_version_parse(version, "1.0?.3-alpha");
    EXPECT_EQ(result, SEMVER_PARSE_MINOR_LEADING_ZERO);


    result = semver_version_parse(version, "1.02.3");
    EXPECT_EQ(result, SEMVER_PARSE_MINOR_LEADING_ZERO);


    result = semver_version_parse(version, "1.02.3.4");
    EXPECT_EQ(result, SEMVER_PARSE_MINOR_LEADING_ZERO);


    // We check number for parts before checking for leading zeros
    result = semver_version_parse(version, "1.01");
    EXPECT_EQ(result, SEMVER_PARSE_MINOR_LEADING_ZERO);
    semver_version_dispose(version);
}

TEST(SemverVersion, ParseMajorWildcard)
{
    HSemverVersion version = semver_version_create();
    auto result = semver_version_parse(version, "X");
    EXPECT_EQ(result, SEMVER_PARSE_MAJOR_WILDCARD);


    result = semver_version_parse(version, "x");
    EXPECT_EQ(result, SEMVER_PARSE_MAJOR_WILDCARD);


    result = semver_version_parse(version, "*");
    EXPECT_EQ(result, SEMVER_PARSE_MAJOR_WILDCARD);


    result = semver_version_parse(version, "X.1");
    EXPECT_EQ(result, SEMVER_PARSE_MAJOR_WILDCARD);


    result = semver_version_parse(version, "x.1");
    EXPECT_EQ(result, SEMVER_PARSE_MAJOR_WILDCARD);


    result = semver_version_parse(version, "*.1");
    EXPECT_EQ(result, SEMVER_PARSE_MAJOR_WILDCARD);


    result = semver_version_parse(version, "X.1.2");
    EXPECT_EQ(result, SEMVER_PARSE_MAJOR_WILDCARD);


    result = semver_version_parse(version, "x.1.2");
    EXPECT_EQ(result, SEMVER_PARSE_MAJOR_WILDCARD);


    result = semver_version_parse(version, "*.1.2");
    EXPECT_EQ(result, SEMVER_PARSE_MAJOR_WILDCARD);


    result = semver_version_parse(version, "*.1.2+build.1");
    EXPECT_EQ(result, SEMVER_PARSE_MAJOR_WILDCARD);


    result = semver_version_parse(version, "*.1.2-prerelease.a");
    EXPECT_EQ(result, SEMVER_PARSE_MAJOR_WILDCARD);


    result = semver_version_parse(version, "x!,[]()<>");
    EXPECT_EQ(result, SEMVER_PARSE_MAJOR_NOT_NUMERIC);
    semver_version_dispose(version);
}

TEST(SemverVersion, ParseMinorWildcard)
{
    HSemverVersion version = semver_version_create();
    auto result = semver_version_parse(version, "1.X");
    EXPECT_EQ(result, SEMVER_PARSE_MINOR_WILDCARD);


    result = semver_version_parse(version, "2.x");
    EXPECT_EQ(result, SEMVER_PARSE_MINOR_WILDCARD);


    result = semver_version_parse(version, "123.*");
    EXPECT_EQ(result, SEMVER_PARSE_MINOR_WILDCARD);


    result = semver_version_parse(version, "2.x.2");
    EXPECT_EQ(result, SEMVER_PARSE_MINOR_WILDCARD);


    result = semver_version_parse(version, "34.X.2");
    EXPECT_EQ(result, SEMVER_PARSE_MINOR_WILDCARD);


    result = semver_version_parse(version, "3.*.2");
    EXPECT_EQ(result, SEMVER_PARSE_MINOR_WILDCARD);


    result = semver_version_parse(version, "1.x.2+build.1");
    EXPECT_EQ(result, SEMVER_PARSE_MINOR_WILDCARD);


    result = semver_version_parse(version, "4.*.2-prerelease.a");
    EXPECT_EQ(result, SEMVER_PARSE_MINOR_WILDCARD);


    result = semver_version_parse(version, "34.XX.2");
    EXPECT_EQ(result, SEMVER_PARSE_MINOR_NOT_NUMERIC);
    semver_version_dispose(version);
}

TEST(SemverVersion, ParseMajorNotNumeric)
{
    HSemverVersion version = semver_version_create();
    auto result = semver_version_parse(version, "B");
    EXPECT_EQ(result, SEMVER_PARSE_MAJOR_NOT_NUMERIC);


    result = semver_version_parse(version, "c");
    EXPECT_EQ(result, SEMVER_PARSE_MAJOR_NOT_NUMERIC);


    result = semver_version_parse(version, "3d");
    EXPECT_EQ(result, SEMVER_PARSE_MAJOR_NOT_NUMERIC);


    result = semver_version_parse(version, "1a.1");
    EXPECT_EQ(result, SEMVER_PARSE_MAJOR_NOT_NUMERIC);


    result = semver_version_parse(version, "2$.1");
    EXPECT_EQ(result, SEMVER_PARSE_MAJOR_NOT_NUMERIC);


    // leading zeros detected before bad characters
    result = semver_version_parse(version, "0/.1");
    EXPECT_EQ(result, SEMVER_PARSE_MAJOR_LEADING_ZERO);


    result = semver_version_parse(version, "O.1.2");
    EXPECT_EQ(result, SEMVER_PARSE_MAJOR_NOT_NUMERIC);


    result = semver_version_parse(version, "A.1.2+build.1");
    EXPECT_EQ(result, SEMVER_PARSE_MAJOR_NOT_NUMERIC);


    result = semver_version_parse(version, "C.1.2-prerelease.a");
    EXPECT_EQ(result, SEMVER_PARSE_MAJOR_NOT_NUMERIC);


    result = semver_version_parse(version, "release");
    EXPECT_EQ(result, SEMVER_PARSE_MAJOR_NOT_NUMERIC);
    semver_version_dispose(version);
}

TEST(SemverVersion, ParseMinorNotNumeric)
{
    HSemverVersion version = semver_version_create();
    auto result = semver_version_parse(version, "22.B.C");
    EXPECT_EQ(result, SEMVER_PARSE_MINOR_NOT_NUMERIC);


    result = semver_version_parse(version, "7.c.0");
    EXPECT_EQ(result, SEMVER_PARSE_MINOR_NOT_NUMERIC);


    result = semver_version_parse(version, "0.3d.1");
    EXPECT_EQ(result, SEMVER_PARSE_MINOR_NOT_NUMERIC);


    result = semver_version_parse(version, "11.1a.1");
    EXPECT_EQ(result, SEMVER_PARSE_MINOR_NOT_NUMERIC);


    result = semver_version_parse(version, "2.2$.1");
    EXPECT_EQ(result, SEMVER_PARSE_MINOR_NOT_NUMERIC);


    // leading zeros detected before bad characters
    result = semver_version_parse(version, "0.10/.1");
    EXPECT_EQ(result, SEMVER_PARSE_MINOR_NOT_NUMERIC);


    result = semver_version_parse(version, "9.O.2");
    EXPECT_EQ(result, SEMVER_PARSE_MINOR_NOT_NUMERIC);


    result = semver_version_parse(version, "1.A.2+build.1");
    EXPECT_EQ(result, SEMVER_PARSE_MINOR_NOT_NUMERIC);


    result = semver_version_parse(version, "11.C.2-prerelease.a");
    EXPECT_EQ(result, SEMVER_PARSE_MINOR_NOT_NUMERIC);


    result = semver_version_parse(version, "2.release.a");
    EXPECT_EQ(result, SEMVER_PARSE_MINOR_NOT_NUMERIC);
    semver_version_dispose(version);
}

TEST(SemverVersion, ParseTooLarge)
{
    HSemverVersion version = semver_version_create();
    auto result = semver_version_parse(version, "9007199254740992.2.3-alpha+build");
    EXPECT_EQ(result, SEMVER_PARSE_MAJOR_TOO_LARGE);


    result = semver_version_parse(version, "900719925474099.9999999999999999.0");
    EXPECT_EQ(result, SEMVER_PARSE_MINOR_TOO_LARGE);


    result = semver_version_parse(version, "1.2.15555555555555555555");
    EXPECT_EQ(result, SEMVER_PARSE_PATCH_TOO_LARGE);
    semver_version_dispose(version);
}

TEST(SemverVersion, ParseTooLargeSpecial)
{
    HSemverVersion version = semver_version_create();
    // 16045725885737590445 is returned for unitialized 0xDEAD'DEAD'DEAD'DEAD
    // but still reported too large for input

    auto result = semver_version_parse(version, "16045725885737590445.2.3-alpha+build");
    EXPECT_EQ(result, SEMVER_PARSE_MAJOR_TOO_LARGE);


    result = semver_version_parse(version, "900719925474099.16045725885737590445.0");
    EXPECT_EQ(result, SEMVER_PARSE_MINOR_TOO_LARGE);


    result = semver_version_parse(version, "1.2.16045725885737590445");
    EXPECT_EQ(result, SEMVER_PARSE_PATCH_TOO_LARGE);


    // 18367622093421514989 is wildcard = 0xFEED'FEED'FEED'FEED
    // only used for output when wildcards are passed for parsing
    // for input still reports too large
    result = semver_version_parse(version, "18367622093421514989.2.3-alpha+build");
    EXPECT_EQ(result, SEMVER_PARSE_MAJOR_TOO_LARGE);


    result = semver_version_parse(version, "900719925474099.18367622093421514989.0");
    EXPECT_EQ(result, SEMVER_PARSE_MINOR_TOO_LARGE);


    result = semver_version_parse(version, "1.2.18367622093421514989");
    EXPECT_EQ(result, SEMVER_PARSE_PATCH_TOO_LARGE);
    semver_version_dispose(version);
}

TEST(SemverVersion, ParseReleaseEmptyIdentifier)
{
    HSemverVersion version = semver_version_create();
    auto result = semver_version_parse(version, "1.2.3-.alpha.12+build");
    EXPECT_EQ(result, SEMVER_PARSE_PRERELEASE_EMPTY_IDENTIFIER);
    const char* prerelease = semver_get_version_prerelease(version);
    EXPECT_STREQ(prerelease, "?"); // first character is .  (meaning starts with empty identifier)
    const char* build = semver_get_version_build(version);
    EXPECT_STREQ(build, "?");



    result = semver_version_parse(version, "1.9999999999999.0-prerelease.5d..2");
    EXPECT_EQ(result, SEMVER_PARSE_PRERELEASE_EMPTY_IDENTIFIER);
    prerelease = semver_get_version_prerelease(version);
    EXPECT_STREQ(prerelease, "?"); 
    build = semver_get_version_build(version);
    EXPECT_STREQ(build, "?");



    result = semver_version_parse(version, "1.2.155555555-beta..ssa.12");
    EXPECT_EQ(result, SEMVER_PARSE_PRERELEASE_EMPTY_IDENTIFIER);
    prerelease = semver_get_version_prerelease(version);
    EXPECT_STREQ(prerelease, "?");
    build = semver_get_version_build(version);
    EXPECT_STREQ(build, "?");



    result = semver_version_parse(version, "1.2.155555555-beta.a..12");
    EXPECT_EQ(result, SEMVER_PARSE_PRERELEASE_EMPTY_IDENTIFIER);
    prerelease = semver_get_version_prerelease(version);
    EXPECT_STREQ(prerelease, "?");
    build = semver_get_version_build(version);
    EXPECT_STREQ(build, "?");



    result = semver_version_parse(version, "1.2.155555555-beta.a.12.");
    EXPECT_EQ(result, SEMVER_PARSE_PRERELEASE_EMPTY_IDENTIFIER);
    prerelease = semver_get_version_prerelease(version);
    EXPECT_STREQ(prerelease, "?");
    build = semver_get_version_build(version);
    EXPECT_STREQ(build, "?");
    semver_version_dispose(version);
}

TEST(SemverVersion, ParseReleaseUnsupportedCharacter)
{
    HSemverVersion version = semver_version_create();
    auto result = semver_version_parse(version, "1.2.3-alpha.(2)+build");
    const char* prerelease;
    const char* build;

    EXPECT_EQ(result, SEMVER_PARSE_PRERELEASE_UNSUPPORTED_CHARACTER);
    prerelease = semver_get_version_prerelease(version);
    EXPECT_STREQ(prerelease, "?");
    build = semver_get_version_build(version);
    EXPECT_STREQ(build, "?");


    result = semver_version_parse(version, "1.9999999999999.0-prerelease.5d.t_3.2");
    prerelease = semver_get_version_prerelease(version);
    EXPECT_STREQ(prerelease, "?");
    build = semver_get_version_build(version);
    EXPECT_STREQ(build, "?");



    result = semver_version_parse(version, "1.2.155555555-beta.=f.ssa.12");
    EXPECT_EQ(result, SEMVER_PARSE_PRERELEASE_UNSUPPORTED_CHARACTER);
    EXPECT_STREQ(semver_get_version_prerelease(version), "?");
    EXPECT_STREQ(semver_get_version_build(version), "?");


    result = semver_version_parse(version, "1.2.155555555-beta.a.f>4.12");
    EXPECT_EQ(result, SEMVER_PARSE_PRERELEASE_UNSUPPORTED_CHARACTER);
    prerelease = semver_get_version_prerelease(version);
    EXPECT_STREQ(prerelease, "?");
    build = semver_get_version_build(version);
    EXPECT_STREQ(semver_get_version_build(version), "?");



    result = semver_version_parse(version, "1.2.155555555-beta.a.12.?");
    EXPECT_EQ(result, SEMVER_PARSE_PRERELEASE_UNSUPPORTED_CHARACTER);
    prerelease = semver_get_version_prerelease(version);
    EXPECT_STREQ(prerelease, "?");
    build = semver_get_version_build(version);
    EXPECT_STREQ(semver_get_version_build(version), "?");



    // dashes are fine
    result = semver_version_parse(version, "1.2.155555555-beta.a.12.--this-is-fine");
    EXPECT_EQ(result, SEMVER_PARSE_SUCCESS);
    prerelease = semver_get_version_prerelease(version);
    EXPECT_STREQ(prerelease, "beta.a.12.--this-is-fine");
    build = semver_get_version_build(version);
    EXPECT_STREQ(semver_get_version_build(version), "");
    semver_version_dispose(version);


}

TEST(SemverVersion, ParseReleaseLeadingZeroOnNumeric)
{
    HSemverVersion version = semver_version_create();

    auto result = semver_version_parse(version, "1.2.3-alpha.04+build");
    EXPECT_EQ(result, SEMVER_PARSE_PRERELEASE_LEADING_ZERO_ON_NUMERIC_IDENTIFIER);
    EXPECT_STREQ(semver_get_version_prerelease(version), "?");
    EXPECT_STREQ(semver_get_version_build(version), "?");


    result = semver_version_parse(version, "1.2.0-prerelease.5.04.2");
    EXPECT_EQ(result, SEMVER_PARSE_PRERELEASE_LEADING_ZERO_ON_NUMERIC_IDENTIFIER);
    EXPECT_STREQ(semver_get_version_prerelease(version), "?");
    EXPECT_STREQ(semver_get_version_build(version), "?");


    result = semver_version_parse(version, "1.2.155555555-beta.f.ssa.012");
    EXPECT_EQ(result, SEMVER_PARSE_PRERELEASE_LEADING_ZERO_ON_NUMERIC_IDENTIFIER);
    EXPECT_STREQ(semver_get_version_prerelease(version), "?");
    EXPECT_STREQ(semver_get_version_build(version), "?");


    result = semver_version_parse(version, "1.2.155555555-beta.a.00.12");
    EXPECT_EQ(result, SEMVER_PARSE_PRERELEASE_LEADING_ZERO_ON_NUMERIC_IDENTIFIER);
    EXPECT_STREQ(semver_get_version_prerelease(version), "?");
    EXPECT_STREQ(semver_get_version_build(version), "?");


    result = semver_version_parse(version, "1.2.155555555-beta.a.12.01");
    EXPECT_EQ(result, SEMVER_PARSE_PRERELEASE_LEADING_ZERO_ON_NUMERIC_IDENTIFIER);
    EXPECT_STREQ(semver_get_version_prerelease(version), "?");
    EXPECT_STREQ(semver_get_version_build(version), "?");


    // leading zeros on non-numeric is fine
    result = semver_version_parse(version, "1.2.155555555-beta.a.12.0this-is-fine");
    EXPECT_EQ(result, SEMVER_PARSE_SUCCESS);
    EXPECT_STREQ(semver_get_version_prerelease(version), "beta.a.12.0this-is-fine");
    EXPECT_STREQ(semver_get_version_build(version), "");
    semver_version_dispose(version);
}

TEST(SemverVersion, ParseBuildEmptyIdentifier)
{
    HSemverVersion version = semver_version_create();
    auto result = semver_version_parse(version, "1.2.3-alpha.12+.bad");
    EXPECT_STREQ(semver_get_version_prerelease(version), "alpha.12");
    EXPECT_STREQ(semver_get_version_build(version), "?");
    EXPECT_EQ(result, SEMVER_PARSE_BUILD_EMPTY_IDENTIFIER);


    result = semver_version_parse(version, "1.9999999999999.0-prerelease.5d+2.");
    EXPECT_EQ(result, SEMVER_PARSE_BUILD_EMPTY_IDENTIFIER);
    EXPECT_STREQ(semver_get_version_prerelease(version), "prerelease.5d");
    EXPECT_STREQ(semver_get_version_build(version), "?");


    result = semver_version_parse(version, "1.2.155555555+build..ssa.12");
    EXPECT_EQ(result, SEMVER_PARSE_BUILD_EMPTY_IDENTIFIER);
    EXPECT_STREQ(semver_get_version_prerelease(version), "");
    EXPECT_STREQ(semver_get_version_build(version), "?");


    result = semver_version_parse(version, "1.2.155555555+build.a..12");
    EXPECT_EQ(result, SEMVER_PARSE_BUILD_EMPTY_IDENTIFIER);
    EXPECT_STREQ(semver_get_version_prerelease(version), "");
    EXPECT_STREQ(semver_get_version_build(version), "?");


    result = semver_version_parse(version, "1.2.155555555+build.a.12.");
    EXPECT_EQ(result, SEMVER_PARSE_BUILD_EMPTY_IDENTIFIER);
    EXPECT_STREQ(semver_get_version_prerelease(version), "");
    EXPECT_STREQ(semver_get_version_build(version), "?");
    semver_version_dispose(version);
}

TEST(SemverVersion, ParseBuildUnsupportedCharacter)
{
    HSemverVersion version = semver_version_create();
    auto result = semver_version_parse(version, "1.2.3-alpha.--2+build+.ba.c.");
    EXPECT_STREQ(semver_get_version_prerelease(version), "alpha.--2");
    EXPECT_STREQ(semver_get_version_build(version), "?");
    EXPECT_EQ(result, SEMVER_PARSE_BUILD_UNSUPPORTED_CHARACTER);



    result = semver_version_parse(version, "1.9999999999999.0+build.5d.t_3.2");
    EXPECT_EQ(result, SEMVER_PARSE_BUILD_UNSUPPORTED_CHARACTER);
    EXPECT_STREQ(semver_get_version_prerelease(version), "");
    EXPECT_STREQ(semver_get_version_build(version), "?");


    result = semver_version_parse(version, "1.2.155555555+=f.ssa.12");
    EXPECT_EQ(result, SEMVER_PARSE_BUILD_UNSUPPORTED_CHARACTER);
    EXPECT_STREQ(semver_get_version_prerelease(version), "");
    EXPECT_STREQ(semver_get_version_build(version), "?");


    result = semver_version_parse(version, "1.2.155555555+a.f>4.12");
    EXPECT_EQ(result, SEMVER_PARSE_BUILD_UNSUPPORTED_CHARACTER);
    EXPECT_STREQ(semver_get_version_prerelease(version), "");
    EXPECT_STREQ(semver_get_version_build(version), "?");


    result = semver_version_parse(version, "1.2.155555555+ssa.12.?");
    EXPECT_EQ(result, SEMVER_PARSE_BUILD_UNSUPPORTED_CHARACTER);
    EXPECT_STREQ(semver_get_version_prerelease(version), "");
    EXPECT_STREQ(semver_get_version_build(version), "?");


    // dashes are fine
    result = semver_version_parse(version, "1.2.155555555-beta.a.12+--this-is-fine");
    EXPECT_EQ(result, SEMVER_PARSE_SUCCESS);
    EXPECT_STREQ(semver_get_version_prerelease(version), "beta.a.12");
    EXPECT_STREQ(semver_get_version_build(version), "--this-is-fine");
    semver_version_dispose(version);
}

TEST(SemverVersion, ReturnVersionString)
{

    struct VersionPair
    {
        const char *v1;
        const char *v2;
    };

    const char *test_equality[] = {
        {"2.0.0"},
        {"2.1.0"},
        {"2.1.1"},
        {"1.0.0-alpha"},
        {"1.0.0-alpha.1"},
        {"1.0.0-alpha.beta"},
        {"1.0.0-beta"},
        {"1.0.0-beta.2"},
        {"1.0.0-beta.11"},
        {"1.0.0-rc.1"},
        {"0.0.0-foo"},
        {"0.0.1"},
        {"0.9.9"},
        {"0.9.0"},
        {"0.99.0"},
        {"1.2.3"},
        {"1.2.3-asdf"},
        {"1.2.3-4"},
        {"1.2.3-4-foo"},
        {"2.7.2+asdf"},
        {"1.2.3-a.10"},
        {"1.2.3-a.5"},
        {"1.2.3-a.b"},
        {"1.2.3-a.b.c.10.d.5"},
        {"1.2.3-a.b.c.5.d.100"},
        {"1.2.3-r2"},
        {"1.2.3-r100"},
        {"1.2.3-r100"},
        {"1.2.3-R2"},
        {"1.2.3-alpha+build"},
        {"11.222.3333-beta"},
        {"1.2.3+build.456"},
        {"1.2.3+---build.456"},
        {"x"},
        {"2.x"},
        {"2.3.x"},
        {"1.2.155555555-beta.a.12+--this-is-fine"}};

    HSemverVersion version = semver_version_create();
    for (const char *v : test_equality)
    {
        SemverParseResult parseresult = semver_version_parse(version, v);
        char * ret = semver_get_version_string(version);

        EXPECT_STREQ(ret, v);
     
        semver_free_string(ret);
    }
    semver_version_dispose(version);
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

TEST(SemverVersion, ReturnBlockVersionString)
{

    const char versions_str[] = 
        "2.0.0\0"
        "2.1.0\0"
        "2.1.1\0"
        "1.0.0-alpha\0"
        "1.0.0-alpha.1\0"
        "1.0.0-alpha.beta\0"
        "1.0.0-beta\0"
        "1.0.0-beta.2\0"
        "1.0.0-beta.11\0"
        "1.0.0-rc.1\0"
        "0.0.0-foo\0"
        "0.0.1\0"
        "0.9.9\0"
        "0.9.0\0"
        "0.99.0\0"
        "1.2.3\0"
        "1.2.3-asdf\0"
        "1.2.3-4\0"
        "1.2.3-4-foo\0"
        "2.7.2+asdf\0"
        "1.2.3-a.10\0"
        "1.2.3-a.5\0"
        "1.2.3-a.b\0"
        "1.2.3-a.b.c.10.d.5\0"
        "1.2.3-a.b.c.5.d.100\0"
        "1.2.3-r2\0"
        "1.2.3-r100\0"
        "1.2.3-r100\0"
        "1.2.3-R2\0"
        "1.2.3-alpha+build\0"
        "11.222.3333-beta\0"
        "1.2.3+build.456\0"
        "1.2.3+---build.456\0"
        "x\0"
        "2.x\0"
        "2.3.x\0"
        "1.2.155555555-beta.a.12+--this-is-fine\0"
        "\0";

    HSemverVersions versions = semver_versions_from_string(versions_str, nullptr, SEMVER_ORDER_AS_GIVEN);
    std::vector<std::string_view> versionstrings = splitMultistringBuffer(versions_str);

    size_t localSplitCount = versionstrings.size();
    size_t librarySplitCount = semver_versions_count(versions);
    EXPECT_EQ(localSplitCount, librarySplitCount);

    for (size_t i = 0; i < librarySplitCount; ++i)
    {
        HSemverVersion version = semver_versions_get_version_at_index(versions, i);
        const char*  readstr = semver_get_version_string(version);
        EXPECT_EQ(versionstrings[i], readstr);

    }

    semver_versions_dispose(versions);


}


static std::vector<std::pair<HSemverVersion, HSemverVersion>> GetTestVersionHandles()
{
    std::vector<std::pair<HSemverVersion, HSemverVersion>> handles;

    // List of test version string pairs and loose flag
    struct VersionPair
    {
        const char *v1;
        const char *v2;
    };

    // 1.0.0 < 2.0.0 < 2.1.0 < 2.1.1
    // 1.0.0-alpha < 1.0.0
    // 1.0.0-alpha < 1.0.0-alpha.1 < 1.0.0-alpha.beta < 1.0.0-beta < 1.0.0-beta.2 < 1.0.0-beta.11 < 1.0.0-rc.1 < 1.0.0
    const VersionPair testPairs[] = {
        {"2.0.0", "1.0.0"},
        {"2.1.0", "2.0.0"},
        {"2.1.1", "2.1.0"},
        {"1.0.0", "1.0.0-alpha"},
        {"1.0.0-alpha.1", "1.0.0-alpha"},
        {"1.0.0-alpha.beta", "1.0.0-alpha.1"},
        {"1.0.0-beta", "1.0.0-alpha.beta"},
        {"1.0.0-beta.2", "1.0.0-beta"},
        {"1.0.0-beta.11", "1.0.0-beta.2"},
        {"1.0.0-rc.1", "1.0.0-beta.11"},
        {"1.0.0", "1.0.0-rc.1"},

        {"0.0.0", "0.0.0-foo"},
        {"0.0.1", "0.0.0"},
        {"1.0.0", "0.9.9"},
        {"0.10.0", "0.9.0"},
        {"0.99.0", "0.10.0"},
        {"2.0.0", "1.2.3"},
        {"1.2.3", "1.2.3-asdf"},
        {"1.2.3", "1.2.3-4"},
        {"1.2.3", "1.2.3-4-foo"},
        {"1.2.3-5-foo", "1.2.3-5"},
        {"1.2.3-5", "1.2.3-4"},
        {"1.2.3-5-foo", "1.2.3-5-Foo"},
        {"3.0.0", "2.7.2+asdf"},
        {"1.2.3-a.10", "1.2.3-a.5"},
        {"1.2.3-a.b", "1.2.3-a.5"},
        {"1.2.3-a.b", "1.2.3-a"},
        {"1.2.3-a.b.c.10.d.5", "1.2.3-a.b.c.5.d.100"},
        {"1.2.3-r2", "1.2.3-r100"},
        {"1.2.3-r100", "1.2.3-R2"}};

    for (const auto &pair : testPairs)
    {
        HSemverVersion h1 = semver_version_create();
        semver_version_parse(h1, pair.v1);
        HSemverVersion h2 = semver_version_create();
        semver_version_parse(h2, pair.v2);
        handles.push_back(std::pair<HSemverVersion, HSemverVersion>(h1, h2));
    }

    return handles;
}

static void DisposeTestVersionHandles(std::vector<std::pair<HSemverVersion, HSemverVersion>> &(handles))
{
    for (auto &pair : handles)
    {
        semver_version_dispose(pair.first);
        semver_version_dispose(pair.second);
    }
    handles.clear();
}

TEST(SemverVersion, Comparisons)
{

    auto testversions = GetTestVersionHandles();

    for (const auto &pair : testversions)
    {
        //first is greater than second
        int first_second = semver_compare(pair.first, pair.second);
        int second_first = semver_compare(pair.second, pair.first);
        int first_first = semver_compare(pair.first, pair.first);
        int second_second = semver_compare(pair.second, pair.second);

        EXPECT_EQ(first_second, 1);
        EXPECT_EQ(second_first, -1);
        EXPECT_EQ(first_first, 0);
        EXPECT_EQ(second_second, 0);
    }

    DisposeTestVersionHandles(testversions);
}

TEST(SemverVersion, EditValidVersionMajor) //all these read back as given as shown in other tests
{
    HSemverVersion version = semver_version_create();

    SemverParseResult versionParseResult = semver_version_parse(version, "1.2.3-alpha+build");

    auto result = semver_set_version_major(version, 5);
    EXPECT_EQ(result, SEMVER_PARSE_SUCCESS);

    EXPECT_EQ(semver_get_version_major(version), 5);
    EXPECT_EQ(semver_get_version_minor(version), 2);
    EXPECT_EQ(semver_get_version_patch(version), 3);
    EXPECT_STREQ(semver_get_version_prerelease(version), "alpha");
    EXPECT_STREQ(semver_get_version_build(version), "build");



    versionParseResult = semver_version_parse(version, "1.0.0-alpha");
    
    result = semver_set_version_major(version, 555555);
    EXPECT_EQ(result, SEMVER_PARSE_SUCCESS);

    EXPECT_EQ(semver_get_version_major(version), 555555);
    EXPECT_EQ(semver_get_version_minor(version), 0);
    EXPECT_EQ(semver_get_version_patch(version), 0);
    EXPECT_STREQ(semver_get_version_prerelease(version), "alpha");
    EXPECT_STREQ(semver_get_version_build(version), "");


    versionParseResult = semver_version_parse(version, "1.0.0-alpha.1");

    result = semver_set_version_major(version, 221);
    EXPECT_EQ(result, SEMVER_PARSE_SUCCESS);

    EXPECT_EQ(semver_get_version_major(version), 221);

    EXPECT_EQ(semver_get_version_minor(version), 0);
    EXPECT_EQ(semver_get_version_patch(version), 0);
    EXPECT_STREQ(semver_get_version_prerelease(version), "alpha.1");
    EXPECT_STREQ(semver_get_version_build(version), "");


    versionParseResult = semver_version_parse(version, "1.0.0-0.3.7");
    result = semver_set_version_major(version, 1234567890);

    EXPECT_EQ(result, SEMVER_PARSE_SUCCESS);


    EXPECT_EQ(semver_get_version_major(version), 1234567890);
    EXPECT_EQ(semver_get_version_minor(version), 0);
    EXPECT_EQ(semver_get_version_patch(version), 0);
    EXPECT_STREQ(semver_get_version_prerelease(version), "0.3.7");
    EXPECT_STREQ(semver_get_version_build(version), "");


    versionParseResult = semver_version_parse(version, "1.0.0-x.7.z.92");
    result = semver_set_version_major(version, 2);

    EXPECT_EQ(result, SEMVER_PARSE_SUCCESS);
    EXPECT_EQ(semver_get_version_major(version), 2);
    EXPECT_EQ(semver_get_version_minor(version), 0);
    EXPECT_EQ(semver_get_version_patch(version), 0);
    EXPECT_STREQ(semver_get_version_prerelease(version), "x.7.z.92");
    EXPECT_STREQ(semver_get_version_build(version), "");


    versionParseResult = semver_version_parse(version, "1.0.0-x-y-z.--");
    result = semver_set_version_major(version, 7);

    EXPECT_EQ(result, SEMVER_PARSE_SUCCESS);
    EXPECT_EQ(semver_get_version_major(version), 7);
    EXPECT_EQ(semver_get_version_minor(version), 0);
    EXPECT_EQ(semver_get_version_patch(version), 0);
    EXPECT_STREQ(semver_get_version_prerelease(version), "x-y-z.--");
    EXPECT_STREQ(semver_get_version_build(version), "");


    versionParseResult = semver_version_parse(version, "1.0.0-alpha+001");

    result = semver_set_version_major(version, 0);

    EXPECT_EQ(result, SEMVER_PARSE_SUCCESS);
    EXPECT_EQ(semver_get_version_major(version), 0);
    EXPECT_EQ(semver_get_version_minor(version), 0);
    EXPECT_EQ(semver_get_version_patch(version), 0);
    EXPECT_STREQ(semver_get_version_prerelease(version), "alpha");
    EXPECT_STREQ(semver_get_version_build(version), "001");


    versionParseResult = semver_version_parse(version, "1.0.0+20130313144700");

    result = semver_set_version_major(version, 8);

    EXPECT_EQ(result, SEMVER_PARSE_SUCCESS);
    EXPECT_EQ(semver_get_version_major(version), 8);

    EXPECT_EQ(semver_get_version_minor(version), 0);
    EXPECT_EQ(semver_get_version_patch(version), 0);
    EXPECT_STREQ(semver_get_version_prerelease(version), "");
    EXPECT_STREQ(semver_get_version_build(version), "20130313144700");


    versionParseResult = semver_version_parse(version, "1.0.0-beta+exp.sha.5114f85");
    result = semver_set_version_major(version, 8888888888);

    EXPECT_EQ(result, SEMVER_PARSE_SUCCESS);
    EXPECT_EQ(semver_get_version_major(version), 8888888888);
    EXPECT_EQ(semver_get_version_minor(version), 0);
    EXPECT_EQ(semver_get_version_patch(version), 0);
    EXPECT_STREQ(semver_get_version_prerelease(version), "beta");
    EXPECT_STREQ(semver_get_version_build(version), "exp.sha.5114f85");


    versionParseResult = semver_version_parse(version, "1.0.0+21AF26D3----117B344092BD");
    result = semver_set_version_major(version, 2000000000);

    EXPECT_EQ(result, SEMVER_PARSE_SUCCESS);
    EXPECT_EQ(semver_get_version_major(version), 2000000000);
    EXPECT_EQ(semver_get_version_minor(version), 0);
    EXPECT_EQ(semver_get_version_patch(version), 0);
    EXPECT_STREQ(semver_get_version_prerelease(version), "");
    EXPECT_STREQ(semver_get_version_build(version), "21AF26D3----117B344092BD"); // - must be before + to denote prerelease
    semver_version_dispose(version);
}


TEST(SemverVersion, EditValidVersionMinor) //all these read back as given as shown in other tests
{
    HSemverVersion version = semver_version_create();
    SemverParseResult versionParseResult = semver_version_parse(version, "1.2.3-alpha+build");

    auto result = semver_set_version_minor(version, 5);
    EXPECT_EQ(result, SEMVER_PARSE_SUCCESS);

    EXPECT_EQ(semver_get_version_major(version), 1);
    EXPECT_EQ(semver_get_version_minor(version), 5);
    EXPECT_EQ(semver_get_version_patch(version), 3);
    EXPECT_STREQ(semver_get_version_prerelease(version), "alpha");
    EXPECT_STREQ(semver_get_version_build(version), "build");



    versionParseResult = semver_version_parse(version, "1.0.0-alpha");

    result = semver_set_version_minor(version, 555555);
    EXPECT_EQ(result, SEMVER_PARSE_SUCCESS);

    EXPECT_EQ(semver_get_version_major(version), 1);
    EXPECT_EQ(semver_get_version_minor(version), 555555);
    EXPECT_EQ(semver_get_version_patch(version), 0);
    EXPECT_STREQ(semver_get_version_prerelease(version), "alpha");
    EXPECT_STREQ(semver_get_version_build(version), "");


    versionParseResult = semver_version_parse(version, "1.0.0-alpha.1");

    result = semver_set_version_minor(version, 221);
    EXPECT_EQ(result, SEMVER_PARSE_SUCCESS);

    EXPECT_EQ(semver_get_version_major(version), 1);

    EXPECT_EQ(semver_get_version_minor(version), 221);
    EXPECT_EQ(semver_get_version_patch(version), 0);
    EXPECT_STREQ(semver_get_version_prerelease(version), "alpha.1");
    EXPECT_STREQ(semver_get_version_build(version), "");


    versionParseResult = semver_version_parse(version, "1.0.0-0.3.7");
    result = semver_set_version_minor(version, 1234567890);

    EXPECT_EQ(result, SEMVER_PARSE_SUCCESS);


    EXPECT_EQ(semver_get_version_major(version), 1);
    EXPECT_EQ(semver_get_version_minor(version), 1234567890);
    EXPECT_EQ(semver_get_version_patch(version), 0);
    EXPECT_STREQ(semver_get_version_prerelease(version), "0.3.7");
    EXPECT_STREQ(semver_get_version_build(version), "");


    versionParseResult = semver_version_parse(version, "1.0.0-x.7.z.92");
    result = semver_set_version_minor(version, 2);

    EXPECT_EQ(result, SEMVER_PARSE_SUCCESS);
    EXPECT_EQ(semver_get_version_major(version), 1);
    EXPECT_EQ(semver_get_version_minor(version), 2);
    EXPECT_EQ(semver_get_version_patch(version), 0);
    EXPECT_STREQ(semver_get_version_prerelease(version), "x.7.z.92");
    EXPECT_STREQ(semver_get_version_build(version), "");


    versionParseResult = semver_version_parse(version, "1.0.0-x-y-z.--");
    result = semver_set_version_minor(version, 7);

    EXPECT_EQ(result, SEMVER_PARSE_SUCCESS);
    EXPECT_EQ(semver_get_version_major(version), 1);
    EXPECT_EQ(semver_get_version_minor(version), 7);
    EXPECT_EQ(semver_get_version_patch(version), 0);
    EXPECT_STREQ(semver_get_version_prerelease(version), "x-y-z.--");
    EXPECT_STREQ(semver_get_version_build(version), "");


    versionParseResult = semver_version_parse(version, "1.0.0-alpha+001");

    result = semver_set_version_minor(version, 1);

    EXPECT_EQ(result, SEMVER_PARSE_SUCCESS);
    EXPECT_EQ(semver_get_version_major(version), 1);
    EXPECT_EQ(semver_get_version_minor(version), 1);
    EXPECT_EQ(semver_get_version_patch(version), 0);
    EXPECT_STREQ(semver_get_version_prerelease(version), "alpha");
    EXPECT_STREQ(semver_get_version_build(version), "001");


    versionParseResult = semver_version_parse(version, "1.0.0+20130313144700");

    result = semver_set_version_minor(version, 8);

    EXPECT_EQ(result, SEMVER_PARSE_SUCCESS);
    EXPECT_EQ(semver_get_version_major(version), 1);

    EXPECT_EQ(semver_get_version_minor(version), 8);
    EXPECT_EQ(semver_get_version_patch(version), 0);
    EXPECT_STREQ(semver_get_version_prerelease(version), "");
    EXPECT_STREQ(semver_get_version_build(version), "20130313144700");


    versionParseResult = semver_version_parse(version, "1.0.0-beta+exp.sha.5114f85");
    result = semver_set_version_minor(version, 8888888888);

    EXPECT_EQ(result, SEMVER_PARSE_SUCCESS);
    EXPECT_EQ(semver_get_version_major(version), 1);
    EXPECT_EQ(semver_get_version_minor(version), 8888888888);
    EXPECT_EQ(semver_get_version_patch(version), 0);
    EXPECT_STREQ(semver_get_version_prerelease(version), "beta");
    EXPECT_STREQ(semver_get_version_build(version), "exp.sha.5114f85");


    versionParseResult = semver_version_parse(version, "1.0.0+21AF26D3----117B344092BD");
    result = semver_set_version_minor(version, 2000000000);

    EXPECT_EQ(result, SEMVER_PARSE_SUCCESS);
    EXPECT_EQ(semver_get_version_major(version), 1);
    EXPECT_EQ(semver_get_version_minor(version), 2000000000);
    EXPECT_EQ(semver_get_version_patch(version), 0);
    EXPECT_STREQ(semver_get_version_prerelease(version), "");
    EXPECT_STREQ(semver_get_version_build(version), "21AF26D3----117B344092BD"); // - must be before + to denote prerelease
    semver_version_dispose(version);
}


TEST(SemverVersion, EditValidVersionPatch) //all these read back as given as shown in other tests
{
    HSemverVersion version = semver_version_create();
    SemverParseResult versionParseResult = semver_version_parse(version, "1.2.3-alpha+build");

    auto result = semver_set_version_patch(version, 5);
    EXPECT_EQ(result, SEMVER_PARSE_SUCCESS);

    EXPECT_EQ(semver_get_version_major(version), 1);
    EXPECT_EQ(semver_get_version_minor(version), 2);
    EXPECT_EQ(semver_get_version_patch(version), 5);
    EXPECT_STREQ(semver_get_version_prerelease(version), "alpha");
    EXPECT_STREQ(semver_get_version_build(version), "build");



    versionParseResult = semver_version_parse(version, "1.0.0-alpha");

    result = semver_set_version_patch(version, 555555);
    EXPECT_EQ(result, SEMVER_PARSE_SUCCESS);

    EXPECT_EQ(semver_get_version_major(version), 1);
    EXPECT_EQ(semver_get_version_minor(version), 0);
    EXPECT_EQ(semver_get_version_patch(version), 555555);
    EXPECT_STREQ(semver_get_version_prerelease(version), "alpha");
    EXPECT_STREQ(semver_get_version_build(version), "");


    versionParseResult = semver_version_parse(version, "1.0.0-alpha.1");

    result = semver_set_version_patch(version, 221);
    EXPECT_EQ(result, SEMVER_PARSE_SUCCESS);

    EXPECT_EQ(semver_get_version_major(version), 1);

    EXPECT_EQ(semver_get_version_minor(version), 0);
    EXPECT_EQ(semver_get_version_patch(version), 221);
    EXPECT_STREQ(semver_get_version_prerelease(version), "alpha.1");
    EXPECT_STREQ(semver_get_version_build(version), "");


    versionParseResult = semver_version_parse(version, "1.0.0-0.3.7");
    result = semver_set_version_patch(version, 1234567890);

    EXPECT_EQ(result, SEMVER_PARSE_SUCCESS);


    EXPECT_EQ(semver_get_version_major(version), 1);
    EXPECT_EQ(semver_get_version_minor(version), 0);
    EXPECT_EQ(semver_get_version_patch(version), 1234567890);
    EXPECT_STREQ(semver_get_version_prerelease(version), "0.3.7");
    EXPECT_STREQ(semver_get_version_build(version), "");


    versionParseResult = semver_version_parse(version, "1.0.0-x.7.z.92");
    result = semver_set_version_patch(version, 2);

    EXPECT_EQ(result, SEMVER_PARSE_SUCCESS);
    EXPECT_EQ(semver_get_version_major(version), 1);
    EXPECT_EQ(semver_get_version_minor(version), 0);
    EXPECT_EQ(semver_get_version_patch(version), 2);
    EXPECT_STREQ(semver_get_version_prerelease(version), "x.7.z.92");
    EXPECT_STREQ(semver_get_version_build(version), "");


    versionParseResult = semver_version_parse(version, "1.0.0-x-y-z.--");
    result = semver_set_version_patch(version, 7);

    EXPECT_EQ(result, SEMVER_PARSE_SUCCESS);
    EXPECT_EQ(semver_get_version_major(version), 1);
    EXPECT_EQ(semver_get_version_minor(version), 0);
    EXPECT_EQ(semver_get_version_patch(version), 7);
    EXPECT_STREQ(semver_get_version_prerelease(version), "x-y-z.--");
    EXPECT_STREQ(semver_get_version_build(version), "");


    versionParseResult = semver_version_parse(version, "1.0.0-alpha+001");

    result = semver_set_version_patch(version, 1);

    EXPECT_EQ(result, SEMVER_PARSE_SUCCESS);
    EXPECT_EQ(semver_get_version_major(version), 1);
    EXPECT_EQ(semver_get_version_minor(version), 0);
    EXPECT_EQ(semver_get_version_patch(version), 1);
    EXPECT_STREQ(semver_get_version_prerelease(version), "alpha");
    EXPECT_STREQ(semver_get_version_build(version), "001");


    versionParseResult = semver_version_parse(version, "1.0.0+20130313144700");

    result = semver_set_version_patch(version, 8);

    EXPECT_EQ(result, SEMVER_PARSE_SUCCESS);
    EXPECT_EQ(semver_get_version_major(version), 1);

    EXPECT_EQ(semver_get_version_minor(version), 0);
    EXPECT_EQ(semver_get_version_patch(version), 8);
    EXPECT_STREQ(semver_get_version_prerelease(version), "");
    EXPECT_STREQ(semver_get_version_build(version), "20130313144700");


    versionParseResult = semver_version_parse(version, "1.0.0-beta+exp.sha.5114f85");
    result = semver_set_version_patch(version, 8888888888);

    EXPECT_EQ(result, SEMVER_PARSE_SUCCESS);
    EXPECT_EQ(semver_get_version_major(version), 1);
    EXPECT_EQ(semver_get_version_minor(version), 0);
    EXPECT_EQ(semver_get_version_patch(version), 8888888888);
    EXPECT_STREQ(semver_get_version_prerelease(version), "beta");
    EXPECT_STREQ(semver_get_version_build(version), "exp.sha.5114f85");


    versionParseResult = semver_version_parse(version, "1.0.0+21AF26D3----117B344092BD");
    result = semver_set_version_patch(version, 2000000000);

    EXPECT_EQ(result, SEMVER_PARSE_SUCCESS);
    EXPECT_EQ(semver_get_version_major(version), 1);
    EXPECT_EQ(semver_get_version_minor(version), 0);
    EXPECT_EQ(semver_get_version_patch(version), 2000000000);
    EXPECT_STREQ(semver_get_version_prerelease(version), "");
    EXPECT_STREQ(semver_get_version_build(version), "21AF26D3----117B344092BD"); // - must be before + to denote prerelease
    semver_version_dispose(version);
}


TEST(SemverVersion, EditValidVersionPrerelease) //all these read back as given as shown in other tests
{
    HSemverVersion version = semver_version_create();
    SemverParseResult versionParseResult = semver_version_parse(version, "1.2.3-alpha+build");

    auto result = semver_set_version_prerelease(version, "beta");
    EXPECT_EQ(result, SEMVER_PARSE_SUCCESS);

    EXPECT_EQ(semver_get_version_major(version), 1);
    EXPECT_EQ(semver_get_version_minor(version), 2);
    EXPECT_EQ(semver_get_version_patch(version), 3);
    EXPECT_STREQ(semver_get_version_prerelease(version), "beta");
    EXPECT_STREQ(semver_get_version_build(version), "build");



    versionParseResult = semver_version_parse(version, "1.0.0-alpha");

    result = semver_set_version_prerelease(version, nullptr);
    EXPECT_EQ(result, SEMVER_PARSE_SUCCESS);

    EXPECT_EQ(semver_get_version_major(version), 1);
    EXPECT_EQ(semver_get_version_minor(version), 0);
    EXPECT_EQ(semver_get_version_patch(version), 0);
    EXPECT_STREQ(semver_get_version_prerelease(version), "");
    EXPECT_STREQ(semver_get_version_build(version), "");


    versionParseResult = semver_version_parse(version, "1.0.0-alpha.1");

    result = semver_set_version_prerelease(version, "alpha.2.3");
    EXPECT_EQ(result, SEMVER_PARSE_SUCCESS);

    EXPECT_EQ(semver_get_version_major(version), 1);

    EXPECT_EQ(semver_get_version_minor(version), 0);
    EXPECT_EQ(semver_get_version_patch(version), 0);
    EXPECT_STREQ(semver_get_version_prerelease(version), "alpha.2.3");
    EXPECT_STREQ(semver_get_version_build(version), "");


    versionParseResult = semver_version_parse(version, "1.0.0-0.3.7");
    result = semver_set_version_prerelease(version, "1234567890");

    EXPECT_EQ(result, SEMVER_PARSE_SUCCESS);


    EXPECT_EQ(semver_get_version_major(version), 1);
    EXPECT_EQ(semver_get_version_minor(version), 0);
    EXPECT_EQ(semver_get_version_patch(version), 0);
    EXPECT_STREQ(semver_get_version_prerelease(version), "1234567890");
    EXPECT_STREQ(semver_get_version_build(version), "");


    versionParseResult = semver_version_parse(version, "1.0.0-x.7.z.92");
    result = semver_set_version_prerelease(version, "2");

    EXPECT_EQ(result, SEMVER_PARSE_SUCCESS);
    EXPECT_EQ(semver_get_version_major(version), 1);
    EXPECT_EQ(semver_get_version_minor(version), 0);
    EXPECT_EQ(semver_get_version_patch(version), 0);
    EXPECT_STREQ(semver_get_version_prerelease(version), "2");
    EXPECT_STREQ(semver_get_version_build(version), "");


    versionParseResult = semver_version_parse(version, "1.0.0-x-y-z.--");
    result = semver_set_version_prerelease(version, "");

    EXPECT_EQ(result, SEMVER_PARSE_SUCCESS);
    EXPECT_EQ(semver_get_version_major(version), 1);
    EXPECT_EQ(semver_get_version_minor(version), 0);
    EXPECT_EQ(semver_get_version_patch(version), 0);
    EXPECT_STREQ(semver_get_version_prerelease(version), "");
    EXPECT_STREQ(semver_get_version_build(version), "");


    versionParseResult = semver_version_parse(version, "1.0.0-alpha+001");

    result = semver_set_version_prerelease(version, "x-y-z.--");

    EXPECT_EQ(result, SEMVER_PARSE_SUCCESS);
    EXPECT_EQ(semver_get_version_major(version), 1);
    EXPECT_EQ(semver_get_version_minor(version), 0);
    EXPECT_EQ(semver_get_version_patch(version), 0);
    EXPECT_STREQ(semver_get_version_prerelease(version), "x-y-z.--");
    EXPECT_STREQ(semver_get_version_build(version), "001");


    versionParseResult = semver_version_parse(version, "1.0.0+20130313144700");

    result = semver_set_version_prerelease(version, "Start----------------------------------------End");

    EXPECT_EQ(result, SEMVER_PARSE_SUCCESS);
    EXPECT_EQ(semver_get_version_major(version), 1);

    EXPECT_EQ(semver_get_version_minor(version), 0);
    EXPECT_EQ(semver_get_version_patch(version), 0);
    EXPECT_STREQ(semver_get_version_prerelease(version), "Start----------------------------------------End");
    EXPECT_STREQ(semver_get_version_build(version), "20130313144700");


    versionParseResult = semver_version_parse(version, "1.0.0-beta+exp.sha.5114f85");
    result = semver_set_version_prerelease(version, "do.re.me.fa.so.la.ti.12.34.a1");

    EXPECT_EQ(result, SEMVER_PARSE_SUCCESS);
    EXPECT_EQ(semver_get_version_major(version), 1);
    EXPECT_EQ(semver_get_version_minor(version), 0);
    EXPECT_EQ(semver_get_version_patch(version), 0);
    EXPECT_STREQ(semver_get_version_prerelease(version), "do.re.me.fa.so.la.ti.12.34.a1");
    EXPECT_STREQ(semver_get_version_build(version), "exp.sha.5114f85");


    versionParseResult = semver_version_parse(version, "1.0.0+21AF26D3----117B344092BD");
    result = semver_set_version_prerelease(version, "1-2-3-4-5-6-7-8-9-0");

    EXPECT_EQ(result, SEMVER_PARSE_SUCCESS);
    EXPECT_EQ(semver_get_version_major(version), 1);
    EXPECT_EQ(semver_get_version_minor(version), 0);
    EXPECT_EQ(semver_get_version_patch(version), 0);
    EXPECT_STREQ(semver_get_version_prerelease(version), "1-2-3-4-5-6-7-8-9-0");
    EXPECT_STREQ(semver_get_version_build(version), "21AF26D3----117B344092BD"); // - must be before + to denote prerelease
    semver_version_dispose(version);
}



TEST(SemverVersion, EditValidVersionBuild) //all these read back as given as shown in other tests
{
    HSemverVersion version = semver_version_create();
    SemverParseResult versionParseResult = semver_version_parse(version, "1.2.3-alpha+build");

    auto result = semver_set_version_build(version, "no");
    EXPECT_EQ(result, SEMVER_PARSE_SUCCESS);

    EXPECT_EQ(semver_get_version_major(version), 1);
    EXPECT_EQ(semver_get_version_minor(version), 2);
    EXPECT_EQ(semver_get_version_patch(version), 3);
    EXPECT_STREQ(semver_get_version_prerelease(version), "alpha");
    EXPECT_STREQ(semver_get_version_build(version), "no");



    versionParseResult = semver_version_parse(version, "1.0.0-alpha");

    result = semver_set_version_build(version, "hi");
    EXPECT_EQ(result, SEMVER_PARSE_SUCCESS);

    EXPECT_EQ(semver_get_version_major(version), 1);
    EXPECT_EQ(semver_get_version_minor(version), 0);
    EXPECT_EQ(semver_get_version_patch(version), 0);
    EXPECT_STREQ(semver_get_version_prerelease(version), "alpha");
    EXPECT_STREQ(semver_get_version_build(version), "hi");


    versionParseResult = semver_version_parse(version, "1.0.0-alpha.1");

    result = semver_set_version_build(version, "1.2.3");
    EXPECT_EQ(result, SEMVER_PARSE_SUCCESS);

    EXPECT_EQ(semver_get_version_major(version), 1);

    EXPECT_EQ(semver_get_version_minor(version), 0);
    EXPECT_EQ(semver_get_version_patch(version), 0);
    EXPECT_STREQ(semver_get_version_prerelease(version), "alpha.1");
    EXPECT_STREQ(semver_get_version_build(version), "1.2.3");


    versionParseResult = semver_version_parse(version, "1.0.0-0.3.7");
    result = semver_set_version_build(version, "1234567890");

    EXPECT_EQ(result, SEMVER_PARSE_SUCCESS);


    EXPECT_EQ(semver_get_version_major(version), 1);
    EXPECT_EQ(semver_get_version_minor(version), 0);
    EXPECT_EQ(semver_get_version_patch(version), 0);
    EXPECT_STREQ(semver_get_version_prerelease(version), "0.3.7");
    EXPECT_STREQ(semver_get_version_build(version), "1234567890");


    versionParseResult = semver_version_parse(version, "1.0.0-x.7.z.92");
    result = semver_set_version_build(version, "2");

    EXPECT_EQ(result, SEMVER_PARSE_SUCCESS);
    EXPECT_EQ(semver_get_version_major(version), 1);
    EXPECT_EQ(semver_get_version_minor(version), 0);
    EXPECT_EQ(semver_get_version_patch(version), 0);
    EXPECT_STREQ(semver_get_version_prerelease(version), "x.7.z.92");
    EXPECT_STREQ(semver_get_version_build(version), "2");


    versionParseResult = semver_version_parse(version, "1.0.0-x-y-z.--");
    result = semver_set_version_build(version, "");

    EXPECT_EQ(result, SEMVER_PARSE_SUCCESS);
    EXPECT_EQ(semver_get_version_major(version), 1);
    EXPECT_EQ(semver_get_version_minor(version), 0);
    EXPECT_EQ(semver_get_version_patch(version), 0);
    EXPECT_STREQ(semver_get_version_prerelease(version), "x-y-z.--");
    EXPECT_STREQ(semver_get_version_build(version), "");


    versionParseResult = semver_version_parse(version, "1.0.0-alpha+001");

    result = semver_set_version_build(version, "x-y-z.--");

    EXPECT_EQ(result, SEMVER_PARSE_SUCCESS);
    EXPECT_EQ(semver_get_version_major(version), 1);
    EXPECT_EQ(semver_get_version_minor(version), 0);
    EXPECT_EQ(semver_get_version_patch(version), 0);
    EXPECT_STREQ(semver_get_version_prerelease(version), "alpha");
    EXPECT_STREQ(semver_get_version_build(version), "x-y-z.--");


    versionParseResult = semver_version_parse(version, "1.0.0+20130313144700");

    result = semver_set_version_build(version, "Start----------------------------------------End");

    EXPECT_EQ(result, SEMVER_PARSE_SUCCESS);
    EXPECT_EQ(semver_get_version_major(version), 1);

    EXPECT_EQ(semver_get_version_minor(version), 0);
    EXPECT_EQ(semver_get_version_patch(version), 0);
    EXPECT_STREQ(semver_get_version_prerelease(version), "");
    EXPECT_STREQ(semver_get_version_build(version), "Start----------------------------------------End");


    versionParseResult = semver_version_parse(version, "1.0.0-beta+exp.sha.5114f85");
    result = semver_set_version_build(version, "01.do.re.me.fa.so.la.ti.12.34.a1");

    EXPECT_EQ(result, SEMVER_PARSE_SUCCESS);
    EXPECT_EQ(semver_get_version_major(version), 1);
    EXPECT_EQ(semver_get_version_minor(version), 0);
    EXPECT_EQ(semver_get_version_patch(version), 0);
    EXPECT_STREQ(semver_get_version_prerelease(version), "beta");
    EXPECT_STREQ(semver_get_version_build(version), "01.do.re.me.fa.so.la.ti.12.34.a1");


    versionParseResult = semver_version_parse(version, "1.0.0+21AF26D3----117B344092BD");
    result = semver_set_version_build(version, "1-2-3-4-5-6-7-8-9-0");

    EXPECT_EQ(result, SEMVER_PARSE_SUCCESS);
    EXPECT_EQ(semver_get_version_major(version), 1);
    EXPECT_EQ(semver_get_version_minor(version), 0);
    EXPECT_EQ(semver_get_version_patch(version), 0);
    EXPECT_STREQ(semver_get_version_prerelease(version), "");
    EXPECT_STREQ(semver_get_version_build(version), "1-2-3-4-5-6-7-8-9-0"); // - must be before + to denote prerelease
    semver_version_dispose(version);
}

TEST(SemverVersion, BuildSetValidFields)
{
    HSemverVersion version = semver_version_create();
    auto result = semver_version_parse(version, "4.5.6");

    result = semver_set_version_values(version, 1, 2, 3, "alpha", "build");
    EXPECT_EQ(result, SEMVER_PARSE_SUCCESS);

    EXPECT_EQ(semver_get_version_major(version), 1);
    EXPECT_EQ(semver_get_version_minor(version), 2);
    EXPECT_EQ(semver_get_version_patch(version), 3);
    EXPECT_STREQ(semver_get_version_prerelease(version), "alpha");
    EXPECT_STREQ(semver_get_version_build(version), "build");

    result = semver_set_version_values(version, 1, 0, 0, "alpha", "");
    EXPECT_EQ(result, SEMVER_PARSE_SUCCESS);
    EXPECT_EQ(semver_get_version_major(version), 1);
    EXPECT_EQ(semver_get_version_minor(version), 0);
    EXPECT_EQ(semver_get_version_patch(version), 0);
    EXPECT_STREQ(semver_get_version_prerelease(version), "alpha");
    EXPECT_STREQ(semver_get_version_build(version), "");

    result = semver_set_version_values(version, 1, 0, 0, "alpha.1", nullptr);
    EXPECT_EQ(result, SEMVER_PARSE_SUCCESS);
    EXPECT_EQ(semver_get_version_major(version), 1);
    EXPECT_EQ(semver_get_version_minor(version), 0);
    EXPECT_EQ(semver_get_version_patch(version), 0);
    EXPECT_STREQ(semver_get_version_prerelease(version), "alpha.1");
    EXPECT_STREQ(semver_get_version_build(version), "");

    result = semver_set_version_values(version, 1, 0, 0, "0.3.5", nullptr);
    EXPECT_EQ(result, SEMVER_PARSE_SUCCESS);
    EXPECT_EQ(semver_get_version_major(version), 1);
    EXPECT_EQ(semver_get_version_minor(version), 0);
    EXPECT_EQ(semver_get_version_patch(version), 0);
    EXPECT_STREQ(semver_get_version_prerelease(version), "0.3.5");
    EXPECT_STREQ(semver_get_version_build(version), "");

    result = semver_set_version_values(version, 2, 3, 4, "x.7.z.92", nullptr);
    EXPECT_EQ(result, SEMVER_PARSE_SUCCESS);
    EXPECT_EQ(semver_get_version_major(version), 2);
    EXPECT_EQ(semver_get_version_minor(version), 3);
    EXPECT_EQ(semver_get_version_patch(version), 4);
    EXPECT_STREQ(semver_get_version_prerelease(version), "x.7.z.92");
    EXPECT_STREQ(semver_get_version_build(version), "");

    result = semver_set_version_values(version, 1, 0, 0, "x-y-z.--", nullptr);
    EXPECT_EQ(result, SEMVER_PARSE_SUCCESS);
    EXPECT_EQ(semver_get_version_major(version), 1);
    EXPECT_EQ(semver_get_version_minor(version), 0);
    EXPECT_EQ(semver_get_version_patch(version), 0);
    EXPECT_STREQ(semver_get_version_prerelease(version), "x-y-z.--");
    EXPECT_STREQ(semver_get_version_build(version), "");

    result = semver_set_version_values(version, 1, 0, 0, "alpha", "001");

    EXPECT_EQ(result, SEMVER_PARSE_SUCCESS);
    EXPECT_EQ(semver_get_version_major(version), 1);
    EXPECT_EQ(semver_get_version_minor(version), 0);
    EXPECT_EQ(semver_get_version_patch(version), 0);
    EXPECT_STREQ(semver_get_version_prerelease(version), "alpha");
    EXPECT_STREQ(semver_get_version_build(version), "001");

    result = semver_set_version_values(version, 1, 0, 0, nullptr, "20130313144700");
    EXPECT_EQ(result, SEMVER_PARSE_SUCCESS);
    EXPECT_EQ(semver_get_version_major(version), 1);
    EXPECT_EQ(semver_get_version_minor(version), 0);
    EXPECT_EQ(semver_get_version_patch(version), 0);
    EXPECT_STREQ(semver_get_version_prerelease(version), "");
    EXPECT_STREQ(semver_get_version_build(version), "20130313144700");

    result = semver_set_version_values(version, 1, 0, 0 ,"beta" , "exp.sha.5114f85");
    EXPECT_EQ(result, SEMVER_PARSE_SUCCESS);
    EXPECT_EQ(semver_get_version_major(version), 1);
    EXPECT_EQ(semver_get_version_minor(version), 0);
    EXPECT_EQ(semver_get_version_patch(version), 0);
    EXPECT_STREQ(semver_get_version_prerelease(version), "beta");
    EXPECT_STREQ(semver_get_version_build(version), "exp.sha.5114f85");

    result = semver_set_version_values(version, 1, 0, 0, "", "21AF26D3----117B344092BD");
    EXPECT_EQ(result, SEMVER_PARSE_SUCCESS);
    EXPECT_EQ(semver_get_version_major(version), 1);
    EXPECT_EQ(semver_get_version_minor(version), 0);
    EXPECT_EQ(semver_get_version_patch(version), 0);
    EXPECT_STREQ(semver_get_version_prerelease(version), "");
    EXPECT_STREQ(semver_get_version_build(version), "21AF26D3----117B344092BD"); // - must be before + to denote prerelease
    semver_version_dispose(version);
}
