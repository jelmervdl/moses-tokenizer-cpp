#include "tokenizer.hh"
#include "regex.hh"
#include "nonbreaking_prefix_set.hh"
#include <iostream>
#include <string>
#include <regex>
#include <boost/algorithm/string.hpp>
#include <boost/algorithm/string/split.hpp>
#include <boost/algorithm/string/classification.hpp>
#include <unicode/utypes.h>


namespace {

using moses::tokenizer::Search;
using moses::tokenizer::Replace;
using moses::tokenizer::Chain;
using moses::tokenizer::Loop;
using moses::tokenizer::Noop;
using moses::tokenizer::string_type;
using moses::tokenizer::char_type;

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
	Replace("[,]([^[:Number:]])", " , $1"),
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

bool StartsNumeric(string_type const &str) {
	return str.size() > 0 && str[0] >= '0' && str[0] <= '9';
}

bool ContainsDot(string_type const &str) {
	return std::find(str.begin(), str.end(), '.') != str.end();
}

bool IsWhitespace(char_type const chr) {
	return chr == '\t' || chr == ' ';
}

bool TokenEndsWithPeriod(string_type const &str, string_type &prefix) {
	if (str.size() < 2)
		return false;

	if (str[str.size() - 1] != '.' || IsWhitespace(str[str.size() - 2]))
		return false;

	prefix.clear();
	prefix.resize(str.size() - 1);
	std::copy(str.begin(), str.end() - 1, prefix.begin());
	return true;
}

void Trim(string_type &vec) {
	auto begin = vec.begin();
	auto end = vec.end();
	
	while (begin != end && IsWhitespace(*begin))
		++begin;

	while (end != begin && IsWhitespace(*(end-1)))
		--end;

	// Should be safe, we're never "trimming" something to something longer
	std::copy(begin, end, vec.begin());
	vec.resize(std::distance(begin, end));
}

class SplitIterator {
public:
	SplitIterator(string_type::const_iterator offset, string_type::const_iterator end)
	: offset_(offset),
	  end_(end),
	  end_pos_(std::find_if(offset_, end_, IsWhitespace)) {
		//
	}

	SplitIterator(SplitIterator const &other) = default;
	
	string_type const operator*() const {
		string_type word;
		word.resize(std::distance(offset_, end_pos_));
		std::copy(offset_, end_pos_, word.begin());
		return word;
	}

	bool operator!=(SplitIterator const &other) const {
		return offset_ != other.offset_;
	}

	SplitIterator& operator++() {
		offset_ = end_pos_ == end_ ? end_ : end_pos_ + 1;
		end_pos_ = std::find_if(offset_, end_, IsWhitespace);
		return *this;
	}

	SplitIterator operator+(int amount) {
		auto it = *this;
		while (amount-- > 0)
			++it;
		return it;
	}

	bool HasNext() const {
		return end_pos_ != end_;
	}
	
private:
	string_type::const_iterator offset_;
	string_type::const_iterator end_;
	string_type::const_iterator end_pos_;
};

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
	string_type tmp1, tmp2;
	StrToUChar(text, tmp1);

	// De-duplicate spaces and clean ASCII junk
	::DeduplicateSpace(tmp1, tmp2);
	std::swap(tmp1, tmp2);
	
	::RemoveASCIIJunk(tmp1, tmp2);
	std::swap(tmp1, tmp2);

	// If protected patterns
	// TODO: implement

	// Strips heading and trailing spaces.
	::Trim(tmp1);

	// Separate out all "other" special characters
	pad_nonalpha_op_(tmp1, tmp2);
	std::swap(tmp1, tmp2);

	// Aggressively splits dashes
	if (options_ & aggressive) {
		::AggressiveHyphenSplit(tmp1, tmp2);
		std::swap(tmp1, tmp2);
	}

	// Multi-dots stay together
	::ReplaceMultidot(tmp1, tmp2);
	std::swap(tmp1, tmp2);

	// Separate out "," except if within numbers e.g. 5,300
	::SeparateCommaInNumbers(tmp1, tmp2);
	std::swap(tmp1, tmp2);

	// (Language-specific) apostrophe tokenization.
	apostrophe_op_(tmp1, tmp2);
	std::swap(tmp1, tmp2);

  HandleNonbreakingPrefixes(tmp1, tmp2);
	std::swap(tmp1, tmp2);

  // Cleans up extraneous spaces.
  ::DeduplicateSpace(tmp1, tmp2);
	std::swap(tmp1, tmp2);
  ::Trim(tmp1);

  // .' at end of sentence is missed
  ::TrailingDotApostrophe(tmp1, tmp2);
	std::swap(tmp1, tmp2);

  // Restore protected
  // TODO: implement

  // Restore mutli-dot
  ::RestoreMultidot(tmp1, tmp2);
	std::swap(tmp1, tmp2);

  // Escape special chars
  if (!(options_ & no_escape)) {
  	::EscapeSpecialChars(tmp1, tmp2);
		std::swap(tmp1, tmp2);
  }

  UCharToStr(tmp1, out);
	return out;
}

void Tokenizer::HandleNonbreakingPrefixes(string_type &text, string_type &out) const {
	out.clear();
	string_type prefix;
	for (auto it = SplitIterator(text.begin(), text.end()); it != SplitIterator(text.end(), text.end()); ++it) {
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
			out.insert(out.end(), prefix.begin(), prefix.end());
			out.push_back(' ');
			out.push_back('.');
		} else {
			string_type word(*it);
			out.insert(out.end(), word.begin(), word.end());
		}
		out.push_back(' ');
	}
}

} } // end namespace
