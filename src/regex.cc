#include "regex.hh"
#include <iostream>

namespace moses { namespace tokenizer {

/**
 * Helpers
 */

void StrToUChar(std::string const &str, string_type &vec) {
	typedef boost::u8_to_u32_iterator<std::string::const_iterator, char_type> conv_type;
	vec.clear();
	vec.reserve(str.size());
	std::copy(conv_type(str.begin(), str.begin(), str.end()),
	          conv_type(str.end(),   str.begin(), str.end()),
	          std::back_inserter(vec));
}

void UCharToStr(string_type const &vec, std::string &str) {
	typedef boost::u32_to_u8_iterator<string_type::const_iterator> conv_type;
	str.clear();
	str.reserve(vec.size());
	std::copy(conv_type(vec.begin()), conv_type(vec.end()), std::back_inserter(str));
}

string_type StrToUChar(std::string const &str) {
	string_type vec;
	StrToUChar(str, vec);
	return vec;
}

std::ostream &operator <<(std::ostream &out, string_type const &vec) {
	std::string str;
	UCharToStr(vec, str);
	return out << str;
}

ReplaceOp::ReplaceOp(std::string const &pattern, std::string const &replacement, std::string const &original_pattern)
: pattern_(original_pattern),
  regex_(boost::make_u32regex(pattern, boost::regex::perl)),
  replacement_(replacement) {
  	//
}

ReplaceOp::ReplaceOp(std::string const &pattern, std::string const &replacement)
: pattern_(pattern),
  regex_(boost::make_u32regex(pattern, boost::regex::perl)),
  replacement_(replacement) {
  	//
}

void ReplaceOp::operator()(string_type &text, string_type &out) const {
	out.clear();
	boost::u32regex_replace(std::back_inserter(out), text.begin(), text.end(), regex_, replacement_);
	// std::cerr << "Pattern: s/" << pattern_ << "/" << replacement_ << "/g\n"
	//           << "     In: " << text << "\n"
	//           << "    Out: " << out << std::endl;
}

SearchOp::SearchOp(std::string const &pattern)
: regex_(boost::make_u32regex(pattern, boost::regex::perl)) {
	//
}

bool SearchOp::operator()(string_type const &text) const {
	return boost::u32regex_search(text.begin(), text.end(), regex_);
}

/**
 * Shortcuts
 */

SearchOp Search(const std::string &pattern) {
	try {
		return SearchOp(pattern);
	} catch (std::exception const &e) {
		std::cerr << "Could not compile expression: " << e.what() << "\n"
		             "Expression`: " << pattern << std::endl;
		std::abort();
	}
}

ReplaceOp Replace(const std::string &pattern, const std::string &replacement) {
	try {
		return ReplaceOp(pattern, replacement, pattern);
	} catch (std::exception const &e) {
		std::cerr << "Could not compile expression: " << e.what() << "\n"
		             "Expression`: " << pattern << std::endl;
		std::abort();
	}
}

} } // end namespace
