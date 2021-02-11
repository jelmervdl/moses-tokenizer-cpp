#ifndef NONBREAKING_PREFIX_SET_HH
#define NONBREAKING_PREFIX_SET_HH

#include <string>
#include <unordered_set>
#include <regex>

namespace moses { namespace tokenizer {

class NonbreakingPrefixSet {
public:
	template <std::size_t Size> NonbreakingPrefixSet(unsigned char (&data)[Size])
	: NonbreakingPrefixSet(std::string(data, data + Size)) {
		// 
	}

	NonbreakingPrefixSet(std::string const &data);

	bool IsNonbreakingPrefix(std::string const &token) const;

	bool IsNumericNonbreakingPrefix(std::string const &token) const;

	static NonbreakingPrefixSet const &get(std::string const &language);
private:
	std::unordered_set<std::string> text_prefixes;
	std::unordered_set<std::string> numeric_prefixes;

	static const std::regex ONLY_NUMERIC_REGEX;
	static const std::regex EMPTY_OR_COMMENT_REGEX;
};

} } // end namespace

#endif
