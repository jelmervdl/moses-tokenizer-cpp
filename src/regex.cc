#include "regex.hh"
#include <iostream>

namespace moses { namespace tokenizer {

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

void ReplaceOp::operator()(std::string &text, std::string &out) const {
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

bool SearchOp::operator()(std::string const &text) const {
	return boost::u32regex_search(text, regex_);
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
