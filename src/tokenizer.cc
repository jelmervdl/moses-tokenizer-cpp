#include "tokenizer.hh"
#include "regex.hh"
#include "nonbreaking_prefix_set.hh"
#include <iostream>
#include <string>
#include <regex>
#include <boost/algorithm/string.hpp>
#include <boost/algorithm/string/split.hpp>
#include <boost/algorithm/string/classification.hpp>

namespace {

using moses::tokenizer::Search;
using moses::tokenizer::Replace;
using moses::tokenizer::Chain;
using moses::tokenizer::Loop;
using moses::tokenizer::Noop;

auto DeduplicateSpace = Replace("[\\s]+", " ");

auto RemoveASCIIJunk = Replace("[\\x00-\\x1F]", "");

auto PadNonAlphanumeric = Replace("([^[:alnum:]\\s\\.'`,-])", " $1 ");

auto FiSvPadNonAlphanumeric = Chain(
	// in Finnish and Swedish, the colon can be used inside words as an apostrophe-like character:
	// USA:n, 20:een, EU:ssa, USA:s, S:t
	Replace("([^[:alnum:]\\s\\.:'`,-])", " $1 "),
	// if a colon is not immediately followed by lower-case characters, separate it out anyway
	Replace("(:)(?=$|[^[:Ll:]])", " $1 ")
);

auto CaPadNonAlphanumeric = Chain(
	// in Catalan, the middle dot can be used inside words: il�lusio
	Replace("([^[:alnum:]\\s\\.·'`,-])", " $1 "),
	// if a middot is not immediately followed by lower-case characters, separate it out anyway
	Replace("(·)(?=$|[^[:Ll:]])", " $1 ")
);

auto AggressiveHyphenSplit = Replace("([[:alnum:]])\\-(?=[[:alnum:]])", "$1 @-@ ");

auto SeparateCommaInNumbers = Chain(
	// separate out "," except if within numbers (5,300)
   // previous "global" application skips some:  A,B,C,D,E > A , B,C , D,E
   // first application uses up B so rule can't see B,C
   // two-step version here may create extra spaces but these are removed later
   // will also space digit,letter or letter,digit forms (redundant with next section)
	Replace("([^[:Number:]])[,]", "$1 , "),
	Replace("[,]([^[:Number:]])", ", $1"),
	// Separate "," after a number if it's the end of a sentence
	Replace("([[:Number:]])[,]$", "$1 , ")
);

auto EnSpecificApostrophe = Chain(
	// Split contractions right
	Replace("([^[:alpha:]])[']([^[:alpha:]])", "$1 ' $2"),
	Replace("([^[:alpha:][:Number:]])[']([[:alpha:]])", "$1 ' $2"),
	Replace("([[:alpha:]])[']([^[:alpha:]])", "$1 ' $2"),
	Replace("([[:alpha:]])[']([[:alpha:]])", "$1 '$2"),
	// Special case for "1990's"
	Replace("([[:Number:]])[']([s])", "$1 '$2")
);

auto FrItGaCaSpecificApostrophe = Chain(
	// Split contractions left
	Replace("([^[:alpha:]])[']([^[:alpha:]])", "$1 ' $2"),
  Replace("([^[:alpha:]])[']([[:alpha:]])", "$1 ' $2"),
  Replace("([[:alpha:]])[']([^[:alpha:]])", "$1 ' $2"),
  Replace("([[:alpha:]])[']([[:alpha:]])", "$1' $2")
);

auto SoSpecificApostrophe = Chain(
	// Don't split glottals
	Replace("([^[:alpha:]])[']([^[:alpha:]])", "$1 ' $2"),
  Replace("([^[:alpha:]])[']([[:alpha:]])", "$1 ' $2"),
  Replace("([[:alpha:]])[']([^[:alpha:]])", "$1 ' $2")
);

auto NonSpecificApostrophe = Replace("'", " ' ");

auto TrailingDotApostrophe = Replace("\\.' ?$", " . ' ");

auto EscapeSpecialChars = Chain(
	Replace("&", "&amp;"),   // escape escape
	Replace("\\|", "&#124;"),  // factor separator
	Replace("<", "&lt;"),    // xml
	Replace(">", "&gt;"),    // xml
	Replace("'", "&apos;"),  // xml
	Replace("\"", "&quot;"),  // xml
	Replace("\\[", "&#91;"),   // syntax non-terminal
	Replace("\\]", "&#93;")    // syntax non-terminal
);

auto ReplaceMultidot = Loop(
	Replace("\\.([\\.]+)", " DOTMULTI$1"),
	Search("DOTMULTI\\."),
	Chain(
		Replace("DOTMULTI\\.([^\\.])", "DOTDOTMULTI $1"),
		Replace("DOTMULTI\\.", "DOTDOTMULTI")
	),
	Noop()
);

auto RestoreMultidot = Loop(
	Noop(),
	Search("DOTDOTMULTI"),
	Replace("DOTDOTMULTI", "DOTMULTI."),
	Replace("DOTMULTI", ".")
);

auto ContainsAlpha = Search("[[:alpha:]]");

auto StartsLowerCase = Search("^[[:lower:]]");

bool StartsNumeric(std::string const &str) {
	return str.size() > 0 && str[0] >= '0' && str[0] <= '9';
}

bool ContainsDot(std::string const &str) {
	return str.find('.') != std::string::npos;
}

bool IsWhitespace(char chr) {
	return chr == '\t' || chr == ' ';
}

bool TokenEndsWithPeriod(std::string const &str, std::string &prefix) {
	if (str.size() < 2)
		return false;

	if (str[str.size() - 1] != '.' || IsWhitespace(str[str.size() - 2]))
		return false;

	prefix = str.substr(0, str.size() - 1);
	return true;
}

class SplitIterator {
public:
	SplitIterator(std::string const &text, std::size_t offset = 0)
	: text_(text),
	  offset_(offset),
	  end_pos_(text_.find(' ', offset_)) {
		//
	}

	SplitIterator(SplitIterator const &other) = default;
	
	SplitIterator()
	: text_(kEmpty),
	  offset_(std::string::npos),
	  end_pos_(std::string::npos) {
		//
	}

	std::string const operator*() const {
		return text_.substr(offset_, HasNext() ? end_pos_ - offset_ : std::string::npos);
	}

	bool operator!=(SplitIterator const &other) const {
		return offset_ != other.offset_;
	}

	SplitIterator& operator++() {
		offset_ = end_pos_ == std::string::npos ? std::string::npos : end_pos_ + 1;
		end_pos_ = text_.find(' ', offset_);
		return *this;
	}

	SplitIterator operator+(int amount) {
		auto it = *this;
		while (amount-- > 0)
			++it;
		return it;
	}

	bool HasNext() const {
		return end_pos_ != std::string::npos;
	}
	
// private:
	static const std::string kEmpty;
	std::string const &text_;
	std::size_t offset_;
	std::size_t end_pos_;
};

std::string const SplitIterator::kEmpty = "";

} // anonymous namespace

namespace moses { namespace tokenizer {

Tokenizer::Tokenizer(const std::string &language, Options options)
: options_(options),
	language_(language),
  prefix_set_(NonbreakingPrefixSet::get(language)) {
  if (language_ == "fi" || language_ == "sv")
  	pad_nonalpha_op_ = ::FiSvPadNonAlphanumeric;
  else if (language_ == "ca")
  	pad_nonalpha_op_ = ::CaPadNonAlphanumeric;
  else
  	pad_nonalpha_op_ = ::PadNonAlphanumeric;

	if (language_ == "en")
		apostrophe_op_ = ::EnSpecificApostrophe;
	else if (language_ == "fr" || language_ == "it" || language_ == "ga" || language_ == "ca")
		apostrophe_op_ = ::FrItGaCaSpecificApostrophe;
	else if (language_ == "so")
		apostrophe_op_ = ::SoSpecificApostrophe;
	else
		apostrophe_op_ = ::NonSpecificApostrophe;
}

std::string &Tokenizer::operator()(const std::string &text, std::string &out) const {
	std::string tmp(text);

	// De-duplicate spaces and clean ASCII junk
	::DeduplicateSpace(tmp, out);
	std::swap(tmp, out);
	
	::RemoveASCIIJunk(tmp, out);
	std::swap(tmp, out);

	// If protected patterns
	// TODO: implement

	// Strips heading and trailing spaces.
	boost::algorithm::trim(tmp);

	// Separate out all "other" special characters
	pad_nonalpha_op_(tmp, out);
	std::swap(tmp, out);

	// Aggressively splits dashes
	if (options_ & aggressive) {
		::AggressiveHyphenSplit(tmp, out);
		std::swap(tmp, out);
	}

	// Multi-dots stay together
	::ReplaceMultidot(tmp, out);
	std::swap(tmp, out);

	// Separate out "," except if within numbers e.g. 5,300
	::SeparateCommaInNumbers(tmp, out);
	std::swap(tmp, out);

	// (Language-specific) apostrophe tokenization.
	apostrophe_op_(tmp, out);
	std::swap(tmp, out);

  HandleNonbreakingPrefixes(tmp, out);
  std::swap(tmp, out);

  // Cleans up extraneous spaces.
  ::DeduplicateSpace(tmp, out);
  std::swap(tmp, out);
  boost::algorithm::trim(tmp);

  // .' at end of sentence is missed
  ::TrailingDotApostrophe(tmp, out);
  std::swap(tmp, out);

  // Restore protected
  // TODO: implement

  // Restore mutli-dot
  ::RestoreMultidot(tmp, out);
  std::swap(tmp, out);

  // Escape special chars
  if (!(options_ & no_escape)) {
  	::EscapeSpecialChars(tmp, out);
  	std::swap(tmp, out);
  }

  std::swap(tmp, out);
	return out;
}

void Tokenizer::HandleNonbreakingPrefixes(std::string &text, std::string &out) const {
	out.clear();
	std::string prefix;
	for (auto it = SplitIterator(text); it != SplitIterator(); ++it) {
		bool split = false;

		if (::TokenEndsWithPeriod(*it, prefix)) {
			// Split last words independently as they are unlikely to be non-breaking prefixes
			if (!it.HasNext()) {
				split = true;
			} else if (::ContainsDot(prefix) && ::ContainsAlpha(prefix)) {
				// no change
			} else if (prefix_set_.IsNonbreakingPrefix(prefix)) {
				// no change
			} else if (::StartsLowerCase(*(it+1))) {
				// no change
			} else if (::StartsNumeric(*(it+1)) && prefix_set_.IsNumericNonbreakingPrefix(prefix)) {
				// no change
			} else {
				split = true;
			}
		}

		if (split) {
			out.append(prefix);
			out.append(" .");
		} else {
			out.append(*it);
		}
		out.append(" ");
	}
}

} } // end namespace
