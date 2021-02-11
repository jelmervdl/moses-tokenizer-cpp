#include "regex.hh"
#include <unordered_map>
#include <iostream>

namespace {

template <std::size_t Size> std::string BinToStr(unsigned char (&&data)[Size]) {
	return std::string(data, data + Size);
}

std::unordered_map<std::string, std::string> const &PerlCategories() {
	static std::unordered_map<std::string, std::string> categories;

	if (categories.empty()) {
		categories.emplace("IsAlpha", ::BinToStr((unsigned char[]){
			#include "../data/perluniprops/IsAlpha.txt.hex"
		}));
		categories.emplace("IsAlnum", ::BinToStr((unsigned char[]){
			#include "../data/perluniprops/IsAlnum.txt.hex"
		}));
		categories.emplace("IsLower", ::BinToStr((unsigned char[]){
			#include "../data/perluniprops/IsLower.txt.hex"
		}));
		categories.emplace("IsN", ::BinToStr((unsigned char[]){
			#include "../data/perluniprops/IsN.txt.hex"
		}));
		categories.emplace("Ll", ::BinToStr((unsigned char[]){
			#include "../data/perluniprops/Lowercase_Letter.txt.hex"
		}));
	}

	return categories;
}

}

namespace moses { namespace tokenizer {

ReplaceOp::ReplaceOp(std::string const &pattern, std::string const &replacement, std::string const &original_pattern)
: pattern_(original_pattern),
  regex_(pattern),
  replacement_(replacement) {
  	//
}

ReplaceOp::ReplaceOp(std::string const &pattern, std::string const &replacement)
: pattern_(pattern),
  regex_(pattern),
  replacement_(replacement) {
  	//
}

std::string &ReplaceOp::operator()(std::string &text) const {
	std::string out;
	out.reserve(text.size() * 1.2);
	std::regex_replace(std::back_inserter(out), text.begin(), text.end(), regex_, replacement_);
	text.swap(out);
	return text;
}

SearchOp::SearchOp(std::string const &pattern)
: regex_(pattern) {
	//
}

bool SearchOp::operator()(std::string const &text) const {
	return std::regex_search(text, regex_);
}

/**
 * Shortcuts
 */

SearchOp Search(const std::string &pattern) {
	try {
		return SearchOp(FormatPerlRegex(pattern));
	} catch (std::exception const &e) {
		std::cerr << "Could not compile expression: " << e.what() << "\n"
		             "Expression`: " << pattern << std::endl;
		std::abort();
	}
}

ReplaceOp Replace(const std::string &pattern, const std::string &replacement) {
	try {
		return ReplaceOp(FormatPerlRegex(pattern), replacement, pattern);
	} catch (std::exception const &e) {
		std::cerr << "Could not compile expression: " << e.what() << "\n"
		             "Expression`: " << pattern << std::endl;
		std::abort();
	}
}

std::string FormatPerlRegex(std::string const &pattern) {
	std::string out;
	out.reserve(pattern.size()); // At least!

	std::regex placeholder_pattern("\\\\p\\{([a-zA-Z]+)\\}");
	std::smatch match;
	std::string remain(pattern);
	for (std::sregex_iterator it(pattern.begin(), pattern.end(), placeholder_pattern), end; it != end; ++it) {
		out.append(it->prefix());
		remain = it->suffix();
		auto category = ::PerlCategories().find(it->str(1));
		if (category == ::PerlCategories().end())
			throw std::runtime_error("Unknown category '" + it->str(1) + "' in expression.");
		out.append(category->second);
	}
	out.append(remain);
	return out;
}

} } // end namespace
