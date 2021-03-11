#include "nonbreaking_prefix_set.hh"
#include "regex.hh"
#include <unordered_map>
#include <sstream>

namespace moses { namespace tokenizer {

const std::regex NonbreakingPrefixSet::ONLY_NUMERIC_REGEX = std::regex("^(.+)\\s+#NUMERIC_ONLY#\\s*");

const std::regex NonbreakingPrefixSet::EMPTY_OR_COMMENT_REGEX = std::regex("^#.*|^\\s*$");

NonbreakingPrefixSet::NonbreakingPrefixSet(std::string const &data) {
	std::istringstream data_stream(data);

	std::smatch match;
	std::string line;
	while (std::getline(data_stream, line)) {
		if (std::regex_match(line, EMPTY_OR_COMMENT_REGEX))
			continue;

		if (std::regex_match(line, match, ONLY_NUMERIC_REGEX))
			numeric_prefixes.insert(StrToUChar(match.str(1)));
		else
			text_prefixes.insert(StrToUChar(line));
	}
}

} } // end namespace

namespace {

using moses::tokenizer::NonbreakingPrefixSet;

std::unordered_map<std::string, NonbreakingPrefixSet> NONBREAKING_PREFIX_SETS{
	{
		"ca",
		NonbreakingPrefixSet{(unsigned char[]){
			#include "../data/nonbreaking_prefixes/nonbreaking_prefix.ca.hex"
		}}
	},
	{
		"cs",
		NonbreakingPrefixSet{(unsigned char[]){
			#include "../data/nonbreaking_prefixes/nonbreaking_prefix.cs.hex"
		}}
	},
	{
		"de",
		NonbreakingPrefixSet{(unsigned char[]){
			#include "../data/nonbreaking_prefixes/nonbreaking_prefix.de.hex"
		}}
	},
	{
		"el",
		NonbreakingPrefixSet{(unsigned char[]){
			#include "../data/nonbreaking_prefixes/nonbreaking_prefix.el.hex"
		}}
	},
	{
		"en",
		NonbreakingPrefixSet{(unsigned char[]){
			#include "../data/nonbreaking_prefixes/nonbreaking_prefix.en.hex"
		}}
	},
	{
		"es",
		NonbreakingPrefixSet{(unsigned char[]){
			#include "../data/nonbreaking_prefixes/nonbreaking_prefix.es.hex"
		}}
	},
	{
		"fi",
		NonbreakingPrefixSet{(unsigned char[]){
			#include "../data/nonbreaking_prefixes/nonbreaking_prefix.fi.hex"
		}}
	},
	{
		"fr",
		NonbreakingPrefixSet{(unsigned char[]){
			#include "../data/nonbreaking_prefixes/nonbreaking_prefix.fr.hex"
		}}
	},
	{
		"ga",
		NonbreakingPrefixSet{(unsigned char[]){
			#include "../data/nonbreaking_prefixes/nonbreaking_prefix.ga.hex"
		}}
	},
	{
		"hu",
		NonbreakingPrefixSet{(unsigned char[]){
			#include "../data/nonbreaking_prefixes/nonbreaking_prefix.hu.hex"
		}}
	},
	{
		"is",
		NonbreakingPrefixSet{(unsigned char[]){
			#include "../data/nonbreaking_prefixes/nonbreaking_prefix.is.hex"
		}}
	},
	{
		"it",
		NonbreakingPrefixSet{(unsigned char[]){
			#include "../data/nonbreaking_prefixes/nonbreaking_prefix.it.hex"
		}}
	},
	{
		"lt",
		NonbreakingPrefixSet{(unsigned char[]){
			#include "../data/nonbreaking_prefixes/nonbreaking_prefix.lt.hex"
		}}
	},
	{
		"lv",
		NonbreakingPrefixSet{(unsigned char[]){
			#include "../data/nonbreaking_prefixes/nonbreaking_prefix.lv.hex"
		}}
	},
	{
		"nl",
		NonbreakingPrefixSet{(unsigned char[]){
			#include "../data/nonbreaking_prefixes/nonbreaking_prefix.nl.hex"
		}}
	},
	{
		"pl",
		NonbreakingPrefixSet{(unsigned char[]){
			#include "../data/nonbreaking_prefixes/nonbreaking_prefix.pl.hex"
		}}
	},
	{
		"pt",
		NonbreakingPrefixSet{(unsigned char[]){
			#include "../data/nonbreaking_prefixes/nonbreaking_prefix.pt.hex"
		}}
	},
	{
		"ro",
		NonbreakingPrefixSet{(unsigned char[]){
			#include "../data/nonbreaking_prefixes/nonbreaking_prefix.ro.hex"
		}}
	},
	{
		"ru",
		NonbreakingPrefixSet{(unsigned char[]){
			#include "../data/nonbreaking_prefixes/nonbreaking_prefix.ru.hex"
		}}
	},
	{
		"sk",
		NonbreakingPrefixSet{(unsigned char[]){
			#include "../data/nonbreaking_prefixes/nonbreaking_prefix.sk.hex"
		}}
	},
	{
		"sl",
		NonbreakingPrefixSet{(unsigned char[]){
			#include "../data/nonbreaking_prefixes/nonbreaking_prefix.sl.hex"
		}}
	},
	{
		"sv",
		NonbreakingPrefixSet{(unsigned char[]){
			#include "../data/nonbreaking_prefixes/nonbreaking_prefix.sv.hex"
		}}
	},
	{
		"ta",
		NonbreakingPrefixSet{(unsigned char[]){
			#include "../data/nonbreaking_prefixes/nonbreaking_prefix.ta.hex"
		}}
	},
	{
		"yue",
		NonbreakingPrefixSet{(unsigned char[]){
			#include "../data/nonbreaking_prefixes/nonbreaking_prefix.yue.hex"
		}}
	},
	{
		"zh",
		NonbreakingPrefixSet{(unsigned char[]){
			#include "../data/nonbreaking_prefixes/nonbreaking_prefix.zh.hex"
		}}
	}
};

}

namespace moses { namespace tokenizer {

bool NonbreakingPrefixSet::IsNonbreakingPrefix(string_type const &token) const {
	return text_prefixes.find(token) != text_prefixes.end();
}

bool NonbreakingPrefixSet::IsNumericNonbreakingPrefix(string_type const &token) const {
	return numeric_prefixes.find(token) != numeric_prefixes.end();
}

NonbreakingPrefixSet const &NonbreakingPrefixSet::get(std::string const &language) {
	auto it = NONBREAKING_PREFIX_SETS.find(language);
	if (it != NONBREAKING_PREFIX_SETS.end())
		return it->second;
	else
		return NONBREAKING_PREFIX_SETS.find("en")->second;
}

} } // end namespace
