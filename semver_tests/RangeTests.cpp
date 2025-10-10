// Copyright 2025 Jasper Schellingerhout. All rights reserved.

#include "pch.h"
#include "gtest/gtest.h"
#include "API/semver.h"

// TEST(SemverRange, ParseValidRange) {
//     HSemverQuery query = semver_query_create("1.2.3");
//
//     HSemverVersion version = semver_version_create("1.2.3");
//     EXPECT_TRUE(semver_version_in_range(version, query));
//     
//
//     version = semver_version_create("1.2.2");
//     EXPECT_FALSE(semver_version_in_range(version, query));
//     
//
//     version = semver_version_create("1.2.4");
//     EXPECT_FALSE(semver_version_in_range(version, query));
//     semver_version_dispose(version);
//
//     semver_range_dispose(query);
//
//
//
///*    HSemverVersion version = nullptr;
//    auto result = semver_version_parse("1.2.3", &version);
//    EXPECT_EQ(result, SEMVER_QUERY_PARSE_SUCCESS)*/;
//}

TEST(SemverRange, ParseGTERange)
{
	HSemverQuery query = semver_query_create();
	semver_query_parse(query, ">=1.2.3");

	HSemverVersion version = semver_version_create();
	
	semver_version_parse(version, "1.2.3-pre");
	BOOL result = semver_query_matches_version(query, version);
	EXPECT_FALSE(result); // pre has rank than 1.2.3


	semver_version_parse(version, "1.2.3");
	result = semver_query_matches_version(query, version);
	EXPECT_TRUE(result);

	semver_version_parse(version, "1.2.2");
	result = semver_query_matches_version(query, version);
	EXPECT_FALSE(result);

	semver_version_parse(version, "1.2.4");
	result = semver_query_matches_version(query, version);
	EXPECT_TRUE(result);

	semver_version_parse(version, "10.11.12");
	result = semver_query_matches_version(query, version);
	EXPECT_TRUE(result);

	
	semver_version_dispose(version);

	semver_query_dispose(query);
}

TEST(SemverRange, ParseGTPReRange)
{
	HSemverQuery query = semver_query_create();
		
	semver_query_parse(query, ">1.2.3  @alpha"); // ">1.2.3@0")
	HSemverVersion version = semver_version_create();
	
	semver_version_parse(version, "1.2.3-0"); 
	EXPECT_FALSE(semver_query_matches_version(query, version));
	
	semver_version_parse(version, "1.2.3-alpha");
	EXPECT_FALSE(semver_query_matches_version(query, version));

	semver_version_parse(version, "1.2.3-beta");
	EXPECT_FALSE(semver_query_matches_version(query, version));

	semver_version_parse(version, "1.2.4-0");
	EXPECT_FALSE(semver_query_matches_version(query, version));

	semver_version_parse(version, "1.2.4-alpha");
	EXPECT_TRUE(semver_query_matches_version(query, version));

	semver_query_parse(query, ">1.2.2 @0"); 
	semver_version_parse(version, "1.2.3-0"); 
	EXPECT_TRUE(semver_query_matches_version(query, version));



	semver_version_dispose(version);
}





TEST(SemverRange, ParseGTRange)
{
	HSemverQuery query = semver_query_create();
	semver_query_parse(query, ">1.2.3");

	HSemverVersion version = semver_version_create();

	semver_version_parse(version,"1.2.3");
	EXPECT_FALSE(semver_query_matches_version(query, version));
	

	semver_version_parse(version,"1.2.2");
	EXPECT_FALSE(semver_query_matches_version(query, version));
	

	semver_version_parse(version,"1.2.4");
	EXPECT_TRUE(semver_query_matches_version(query, version));
	

	semver_version_parse(version,"1.2.4-pre");
//	semver_set_include_prerelease(true);
//	EXPECT_TRUE(semver_query_matches_version(query, version));

//	semver_set_include_prerelease(false);
	EXPECT_FALSE(semver_query_matches_version(query, version));
	

	semver_version_parse(version,"10.11.12");
	EXPECT_TRUE(semver_query_matches_version(query, version));
	semver_version_dispose(version);

	semver_query_dispose(query);
}


struct RangeEquivalence
{
	const char* range_str;
	const char* read_range_str;
	//SemverQueryParseResult expected_result = SEMVER_QUERY_PARSE_SUCCESS;
	
};



TEST(SemverRange, ReadMeRanges)
{
	const RangeEquivalence equivalent[] =
	{
		//{">3.2.1", ">3.2.1"},
		//{"<=4.5.2","<=4.5.2"},

		{">=1.2.3-alpha", ">=1.2.3-alpha"},
		{">2.1", ">=2.2.0-0"},
		{">=x", "x"},
		{"<=x", "x"},
		{"<0.0.0-0", "<0.0.0-0"},
		{"<5.1.3 >5.1.3", "<0.0.0-0"},
		{">1.2.3-beta <1.2.3-alpha", "<0.0.0-0"},
		{">3.0.0 <=2.0.0","<0.0.0-0"},
		{">x","<0.0.0-0"},
		{"<x","<0.0.0-0"},
		{"=2.3.4", "2.3.4"},
		{ ">=2", ">=2.0.0" },
		{ ">=4.5", ">=4.5.0" },
		{">2", ">=3.0.0-0"},
		{">4.5", ">=4.6.0-0"},
		{"<3", "<3.0.0"},
		{"<5.2", "<5.2.0"},
		{"<=3", "<4.0.0-0"},
		{"<=2.5", "<2.6.0-0"},
		{"2", ">=2.0.0 <3.0.0-0"},
		{"2.x", ">=2.0.0 <3.0.0-0"},
		{"1.2.3 - 4.5.6", ">=1.2.3 <=4.5.6"},
		{"1.2.3-0 - 4.5.6", ">=1.2.3-0 <=4.5.6"},
		{"1 - 2.3", "1.0.0 - 2.3.x"},
		{"1 - 2.3", ">=1.0.0 <2.4.0-0"},
		{"~1.2.3", ">=1.2.3 <1.3.0-0"},
		{"~1.2.3-0", ">=1.2.3-0 <1.3.0-0"},
		{"~1.2", ">=1.2.0 <1.3.0-0"},
		{"~3", "3"},
		{"3", ">=3.0.0 <4.0.0-0"},
		{"^1.2.3", ">=1.2.3 <2.0.0-0"},
		{"^0.2.4", ">=0.2.4 <0.3.0-0"},
		{"^0.0.5", ">=0.0.5 <0.0.6-0"},
		{"^1.2.3-0", ">=1.2.3-0 <2.0.0-0"},
		{"^1.2", "^1.2.0"}, 
		{"^1.2", ">=1.2.0 <2.0.0-0"},
		{"^0.2", "^0.2.0"},
		{"^0.2",">=0.2.0 <0.3.0-0"},
		{"^3", "^3.0.0"},
	    {"^3", ">=3.0.0 <4.0.0-0" },
		{"3.1.x", ">=3.1.0 <3.2.0-0"},
		{"~0.2.x", "0.2.x"},
		{"^4.x", "4.x"},
		 {"1.2.3||>=2.3.4 <2.3.6", "1.2.3 || >=2.3.4 <2.3.6" }, // problem
		{"> 3.2.1 <= 4.5.2",">3.2.1 <=4.5.2" },

		{">=1.2.3 <1.3.2 || 1.4.0", ">=1.2.3 <1.3.2 || 1.4.0"},
		// unlike node-semver these are included



	};

	HSemverQuery query = semver_query_create();
	HSemverQuery query2 = semver_query_create();

	for (const auto& rv : equivalent)
	{
		
		
		SemverQueryParseResult parseresult = semver_query_parse(query, rv.range_str);
		SemverQueryParseResult parseresult2 = semver_query_parse(query2, rv.read_range_str);
		std::string string1 = semver_get_query_string(query);
		std::string string2 = semver_get_query_string(query2);
		
		if (string1 != string2)
			string1 = string2; // place breakpoint here to see if differences are signficant
	/*	EXPECT_STREQ(string1, string2;*/

		

		//
	}

	semver_query_dispose(query);
	semver_query_dispose(query2);
}


struct RangeVersion
{
	const char* range_str;
	const char* version_str;

};



TEST(SemverRange, RangeExcludes)
{
	const RangeVersion excluded[] =
		{

			{">2.0.0", "2.0.0-pre"},
			{"2.x.x", "1.1.3"},
			{"0.1.20 || 1.2.4", "1.2.3"},
			{"2.x.x", "3.1.3"},
			{">=0.2.3 || <0.0.1", "0.0.3"},
			{">=0.2.3 || <0.0.1", "0.2.2"},
			{"1.0.0 - 2.0.0", "2.2.3"},
			{"1.2.3+asdf - 2.4.3+asdf @rc", "1.2.3-pre.2"},
			{"1.2.3+asdf - 2.4.3+asdf @alpha.2", "2.4.3-alpha"},
			{"^1.2.3+build", "2.0.0"},
			{"^1.2.3+build", "1.2.0"},
			{"^1.2.3", "1.2.3-pre"},
			{"^1.2", "1.2.0-pre"},
			{">1.2", "1.3.0-beta"},
			{"<=1.2.3", "1.2.3-beta"},
			{"^1.2.3", "1.2.3-beta"},
			{"<=0.7.x", "0.7.0-asdf"},
			{"1.0.0", "1.0.1"},
			{">=1.0.0", "0.0.0"},
			{">=1.0.0", "0.0.1"},
			{">=1.0.0", "0.1.0"},
			{">1.0.0", "0.0.1"},
			{">1.0.0", "0.1.0"},
			{"<=2.0.0", "3.0.0"},

			{"<=2.0.0", "2.9999.9999"},
			{"<=2.0.0", "2.2.9"},
			{"<2.0.0", "2.9999.9999"},
			{"<2.0.0", "2.2.9"},
			{">=0.1.97", "0.1.93"},
			{"1.2.x", "1.3.3"},
			{"1.2.x || 2.x", "3.1.3"},
			{"1.2.x || 2.x", "1.1.3"},
			{"2.*.*", "1.1.3"},
			{"2.*.*", "3.1.3"},
			{"1.2.*", "1.3.3"},
			{"1.2.* || 2.*", "3.1.3"},
			{"1.2.* || 2.*", "1.1.3"},
			{"2", "1.1.2"},
			{"2.3", "2.4.1"},
			{"~0.0.1", "0.1.0-alpha"},
			{"~0.0.1", "0.1.0"},
			{"~2.4", "2.5.0"}, // >=2.4.0 <2.5.0
			{"~2.4", "2.3.9"},
			{"~1", "0.2.3"},	  // >=1.0.0 <2.0.0
			{"~1.0", "1.1.0"}, // >=1.0.0 <1.1.0
			{"<1", "1.0.0"},
			{">=1.2", "1.1.1"},
			{"~v0.5.4-beta @0", "0.5.4-alpha"},
			{"=0.7.x", "0.8.2"},
			{">=0.7.x", "0.6.2"},
			{"<0.7.x", "0.7.2"},
			{"<1.2.3", "1.2.3-beta"},
			{"=1.2.3", "1.2.3-beta"},
			{">1.2", "1.2.8"},
			{"^0.0.1", "0.0.2-alpha"},
			{"^0.0.1", "0.0.2"},
			{"^1.2.3", "2.0.0-alpha"},
			{"^1.2.3", "1.2.2"},
			{"^1.2", "1.1.9"},
			{"*", "1.2.3-foo"},
			
	
			{"1 - 2", "1.0.0-pre"},
			{"1.0 - 2", "1.0.0-pre"},

			// invalid versions never satisfy
			{"*", "not a version"},
			{">=2", "glorp"},

			{"2.x @0", "3.0.0-pre.0"},
			{"^1.0.0", "1.0.0-rc1"},
			{"^1.0.0", "2.0.0-rc1"},
			{"^1.2.3-rc2", "2.0.0"},
			{"^1.0.0", "2.0.0-rc1"},

			{"1 - 2 @alpha", "3.0.0-pre"},
			{"1 - 2", "2.0.0-pre"},
			{"1 - 2", "1.0.0-pre"},
			{"1.0 - 2", "1.0.0-pre"},


			{"1.1.x @0", "1.2.0-a"},
			{"1.1.x @0", "1.0.0-a"},

			{"1.x @0", "0.0.0-a"},
			{"1.x @0", "2.0.0-a"},

			{">=1.0.0 <1.1.0", "1.1.0"},
			{">=1.0.0 <1.1.0 @0", "1.1.0"},
			{">=1.0.0 <1.1.0", "1.1.0-pre"},
			{">=1.0.0 <1.1.0-pre", "1.1.0-pre"},

			{"== 1.0.0 || foo", "2.0.0"},

			{ "=0.7.x", "0.7.0-asdf" },
			{ ">=0.7.x", "0.7.0-asdf" },
			{ "1.1.x", "1.0.0-a" },
			{ "1.1.x", "1.1.0-a" },
			{ "1.1.x", "1.2.0-a" },
			{ "1.x", "1.0.0-a" },
			{ "1.x", "1.1.0-a" },
			{ "1.x", "1.2.0-a" },

			{ "=0.7.x @b", "0.7.0-asdf" },
			{ ">=0.7.x @asdf.2", "0.7.0-asdf" },
			{ "1.1.x @beta", "1.1.0-a" },
			{ "1.x @ab", "1.0.0-a" },
			{ "1.x @rc", "1.1.0-a" },
			{ "1.x @alpha", "1.2.0-a" },

		};


	HSemverQuery query = semver_query_create();
	HSemverVersion version = semver_version_create();



	for (const auto &rv : excluded)
	{

		SemverParseResult versionParseresult = semver_version_parse(version, rv.version_str);

		SemverQueryParseResult parseresult = semver_query_parse(query, rv.range_str);

		std::string range_str = semver_get_query_string(query);

		if (rv.range_str != range_str)
			range_str = rv.range_str;


		bool in_range = semver_query_matches_version(query, version);
		if (in_range)
			in_range = true;

		EXPECT_FALSE(in_range);

		//
	}


	semver_version_dispose(version);
	semver_query_dispose(query);

}

TEST(SemverRange, RangeIncludes)
{
	const RangeVersion included[]{

		// These test cases are from the node semver test fixture
	    {">2.1.0 @0", "2.1.1-pre"},

		{"~2", "2.0.9"},
		{"1.0.0 - 2.0.0", "1.2.3"},
		{"^1.2.3+build", "1.2.3"},
		{"^1.2.3+build", "1.3.0"},
		{"1.2.3-pre+asdf - 2.4.3-pre+asdf", "1.2.3"},
		{"1.2.3-pre+asdf - 2.4.3-pre+asdf", "1.2.3-pre.2"},
		{"1.2.3-pre+asdf - 2.4.3-pre+asdf", "2.4.3-alpha"},
		{"1.2.3+asdf - 2.4.3+asdf", "1.2.3"},
		{"1.0.0", "1.0.0"},
		{">=*", "0.2.4"},
		{"", "1.0.0"},
		{"*", "1.2.3"},
		{">=1.0.0", "1.0.0"},
		{">=1.0.0", "1.0.1"},
		{">=1.0.0", "1.1.0"},
		{">1.0.0", "1.0.1"},
		{">1.0.0", "1.1.0"},
		{"<=2.0.0", "2.0.0"},
		{"<=2.0.0", "1.9999.9999"},
		{"<=2.0.0", "0.2.9"},
		{"<2.0.0", "1.9999.9999"},
		{"<2.0.0", "0.2.9"},
		{">=0.1.97", "0.1.97"},
		{"0.1.20 || 1.2.4", "1.2.4"},
		{">=0.2.3 || <0.0.1", "0.0.0"},
		{">=0.2.3 || <0.0.1", "0.2.3"},
		{">=0.2.3 || <0.0.1", "0.2.4"},
		{"||", "1.3.4"},

		{"2.x.x", "2.1.3"},
		{"1.2.x", "1.2.3"},
		{"1.2.x || 2.x", "2.1.3"},
		{"1.2.x || 2.x", "1.2.3"},
		{"x", "1.2.3"},
		{"2.*.*", "2.1.3"},
		{"1.2.*", "1.2.3"},
		{"1.2.* || 2.*", "2.1.3"},
		{"1.2.* || 2.*", "1.2.3"},
		{"*", "1.2.3"},
		{"2", "2.1.2"},
		{"2.3", "2.3.1"},
		{"~0.0.1", "0.0.1"},
		{"~0.0.1", "0.0.2"},
		{"< 1.2", "1.1.1"},

		{"~x", "0.0.9"},   
		{"~2", "2.0.9"},   
		{"~2.4", "2.4.0"}, // >=2.4.0 <2.5.0
		{"~2.4", "2.4.5"},
		{"~1", "1.2.3"},	  // >=1.0.0 <2.0.0
		{"~1.0", "1.0.2"},	  // >=1.0.0 <1.1.0,
		{">=1", "1.0.0"},
		{"=0.7.x", "0.7.2"},
		{"<=0.7.x", "0.7.2"},
		{">=0.7.x", "0.7.2"},
		{"<=0.7.x", "0.6.2"},
		{"~1.2.1 >=1.2.3", "1.2.3"},
		{"~1.2.1 =1.2.3", "1.2.3"},
		{"~1.2.1 1.2.3", "1.2.3"},
		{"~1.2.1 >=1.2.3 1.2.3", "1.2.3"},
		{"~1.2.1 1.2.3 >=1.2.3", "1.2.3"},
		{">=1.2.1 1.2.3", "1.2.3"},
		{"1.2.3 >=1.2.1", "1.2.3"},
		{">=1.2.3 >=1.2.1", "1.2.3"},
		{">=1.2.1 >=1.2.3", "1.2.3"},
		{">=1.2", "1.2.8"},
		{"^1.2.3", "1.8.1"},
		{"^0.1.2", "0.1.2"},
		{"^0.1", "0.1.2"},
		{"^0.0.1", "0.0.1"},
		{"^1.2", "1.4.2"},
		{"^1.2 ^1", "1.4.2"},
		{"^1.2.3-alpha", "1.2.3-pre"},
		{"^1.2.0-alpha", "1.2.0-pre"},
		{"^0.0.1-alpha", "0.0.1-beta"},
		{"^0.0.1-alpha", "0.0.1"},
		{"^0.1.1-alpha", "0.1.1-beta"},
		{"^x", "1.2.3"},
		{"x - 1.0.0", "0.9.7"},
		{"x - 1.x", "0.9.7"},
		{"1.0.0 - x", "1.9.7"},
		{"1.x - x", "1.9.7"},
		{"<=7.x", "7.9.9"},
		{"2.x @0", "2.0.1-pre.0"},
		{"2.x @pre", "2.1.0-pre.0"},
		{"1.1.x @0", "1.1.2-a"},
		{"1.1.x @9", "1.1.3-a"},
		{"* @rc", "1.0.0-rc1"},
		{"^1.0.0-0 @0", "1.0.1-rc1"},
		{"^1.0.0-rc2 @rc1", "1.0.1-rc1"},
		{"^1.0.0 @rc1", "1.0.1-rc1"},
		{"^1.0.0 @beta", "1.1.0-rc1"},
		{"1 - 2 @0", "2.0.0-pre"},

		{"=0.7.x @a", "0.7.1-asdf"},
		{">=0.7.x @asd", "0.7.2-asdf"},
		{"<=0.7.x @asde", "0.7.3-asdf"},

		{">=1.0.0 <=1.1.0 @pre", "1.1.0-pre"},

	
	
	};

	HSemverQuery query = semver_query_create();
	HSemverVersion version = semver_version_create();
	for (const auto &rv : included)
	{
		SemverQueryParseResult parseresult = semver_query_parse(query, rv.range_str);
		std::string range_str = semver_get_query_string(query);
		

		if (rv.range_str != range_str)
			range_str = rv.range_str; // breakpoint here to test
				
		SemverParseResult versionParseresult =	semver_version_parse(version ,rv.version_str);

		bool in_range = semver_query_matches_version(query, version);

		if (!in_range)
			in_range = false; //breakpoint here to test

		EXPECT_TRUE(in_range);

	}


	semver_version_dispose(version);
	semver_query_dispose(query); 
}



TEST(SemverRange, QueryVersionBlock)
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
	size_t librarySplitCount = semver_versions_count(versions);

	HSemverQuery query = semver_query_create();
	semver_query_parse(query, "<1.0.0");

	HSemverVersions results = semver_query_match_versions(query, versions);
	size_t resultsCount = semver_versions_count(results);

	EXPECT_EQ(resultsCount, 4);
	const char* matches1[4] =
	{
		"0.0.1",
		"0.9.9",
		"0.9.0",
		"0.99.0",
	};



	for (size_t i = 0; i < resultsCount; ++i)
	{
		HSemverVersion version = semver_versions_get_version_at_index(results, i);
		char* readstr = semver_get_version_string(version);
		EXPECT_STREQ(matches1[i], readstr);
		semver_free_string(readstr);
	}

	semver_versions_dispose(results);

	semver_query_parse(query, ">1.2.3 @alpha");
	results = semver_query_match_versions(query, versions);

	resultsCount = semver_versions_count(results);
	EXPECT_EQ(resultsCount, 6);
	const char* matches2[6] =
	{ 
	    "2.0.0",
		"2.1.0",
		"2.1.1",
		"2.7.2+asdf",
		"11.222.3333-beta",
		"1.2.155555555-beta.a.12+--this-is-fine",
	};

	for (size_t i = 0; i < resultsCount; ++i)
	{
	    HSemverVersion version = semver_versions_get_version_at_index(results, i);
	    char* readstr = semver_get_version_string(version);
	    EXPECT_STREQ(matches2[i], readstr);
	    semver_free_string(readstr);
	}

	semver_query_parse(query, "< 2.5.0");

	HSemverVersions results2 = semver_query_match_versions(query, results); // query second level
	resultsCount = semver_versions_count(results2);
	EXPECT_EQ(resultsCount, 3);
	const char* matches3[3] =
	{
		"2.0.0",
		"2.1.0",
		"2.1.1",
	};

	for (size_t i = 0; i < resultsCount; ++i)
	{
		HSemverVersion version = semver_versions_get_version_at_index(results2, i);
		char* readstr = semver_get_version_string(version);
		EXPECT_STREQ(matches3[i], readstr);
		semver_free_string(readstr);
	}


	// since results and results2 are references to Version objects in versions, it also will dispoe them  
	// so the next two lines are optional
//	semver_versions_dispose(results2);
//	semver_versions_dispose(results);



	semver_versions_dispose(versions);


}

TEST(SemverRange, QueryASCSortedVersionBlock)
{

	const char versions_str[] =
		"1.2.2, 1.2.4, 1.2.3-alpha, 1.3.0, 0.7.1, 0.7.2, 1.2.3, 0.7.3, 0.8.0, 0.7.2-beta, 1.2.5, 1.2.3-alpha, 0.0.0, "
        "1.0.0, 2.0.0-alpha, 1.2.6, 999.999.999, 1.2.3-rc.1, 1.2.5-rc, 1.2.3-beta, 1.2.4-rc, 1.2.5-alpha, "
		"1.0.0-alpha, 1.0.1, 2.0.0-alpha, 1.2.4-beta, 2.0.0, 2.0.1, 2.1.0, 1.2.5-rc, 2.1.1"	;

	HSemverVersions versions = semver_versions_from_string(versions_str, ", ", SEMVER_ORDER_ASC);
	size_t librarySplitCount = semver_versions_count(versions);

	HSemverQuery query = semver_query_create();

	semver_query_parse(query, "~1.2.3"); // >=1.2.3 <1.3.0-0`

	HSemverVersions results = semver_query_match_versions(query, versions);
	size_t resultsCount = semver_versions_count(results);

	EXPECT_EQ(resultsCount, 4);
	const char* matches1[4] =
	{
		"1.2.3",
		"1.2.4",
		"1.2.5",
		"1.2.6",
	};


	for (size_t i = 0; i < resultsCount; ++i)
	{
		HSemverVersion version = semver_versions_get_version_at_index(results, i);
		char* readstr = semver_get_version_string(version);
		EXPECT_STREQ(matches1[i], readstr);
		semver_free_string(readstr);
	}

	semver_query_parse(query, "^0.7.2"); // `>=0.7.2 <0.8.0-0`
	results = semver_query_match_versions(query, versions);
	resultsCount = semver_versions_count(results);


	EXPECT_EQ(resultsCount, 2);
	const char* matches2[2] =
	{
		"0.7.2",
		"0.7.3",
	};


	for (size_t i = 0; i < resultsCount; ++i)
	{
		HSemverVersion version = semver_versions_get_version_at_index(results, i);
		char* readstr = semver_get_version_string(version);
		EXPECT_STREQ(matches2[i], readstr);
		semver_free_string(readstr);
	}

	semver_query_parse(query, "1.2.3 - 1.2.5"); // `>=1.2.3 <=1.2.5` ///
	results = semver_query_match_versions(query, versions);
	resultsCount = semver_versions_count(results);


	EXPECT_EQ(resultsCount, 3);
	const char* matches3[3] =
	{
		"1.2.3",
		"1.2.4",
		"1.2.5",
	};


	for (size_t i = 0; i < resultsCount; ++i)
	{
		HSemverVersion version = semver_versions_get_version_at_index(results, i);
		char* readstr = semver_get_version_string(version);
		EXPECT_STREQ(matches1[i], readstr);
		semver_free_string(readstr);
	}


	semver_query_parse(query, "*");
	results = semver_query_match_versions(query, versions);
	resultsCount = semver_versions_count(results);


	EXPECT_EQ(resultsCount, 18);
	const char* matches4[18] =
	{
		"0.0.0",
		"0.7.1",
		"0.7.2",
		"0.7.3",
		"0.8.0",
		"1.0.0",
		"1.0.1",
		"1.2.2",
		"1.2.3",
		"1.2.4",
		"1.2.5",
		"1.2.6",
		"1.3.0",
		"2.0.0",
		"2.0.1",
		"2.1.0",
		"2.1.1",
		"999.999.999"
	};


	for (size_t i = 0; i < resultsCount; ++i)
	{
		HSemverVersion version = semver_versions_get_version_at_index(results, i);
		char* readstr = semver_get_version_string(version);
		EXPECT_STREQ(matches4[i], readstr);
		semver_free_string(readstr);
	}


	semver_versions_dispose(versions);
}