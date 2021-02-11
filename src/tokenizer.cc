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

auto DeduplicateSpace = Replace("\\s+", " ");

auto RemoveASCIIJunk = Replace("[\\x00-\\x1F]", "");

auto PadNonAlphanumeric = Replace("([^\\p{IsAlnum}\\s\\.\\'\\`\\,\\-])", " $1 ");

auto FiSvPadNonAlphanumeric = Chain(
	// in Finnish and Swedish, the colon can be used inside words as an apostrophe-like character:
	// USA:n, 20:een, EU:ssa, USA:s, S:t
	Replace("([^\\p{IsAlnum}\\s\\.\\:\\'\\`\\,\\-])", " $1 "),
	// if a colon is not immediately followed by lower-case characters, separate it out anyway
	Replace("(:)(?=$|[^\\p{Ll}])", " $1 ")
);

auto CaPadNonAlphanumeric = Chain(
	// in Catalan, the middle dot can be used inside words: il�lusio
	Replace("([^\\p{IsAlnum}\\s\\.\\·\\'\\`\\,\\-])", " $1 "),
	// if a middot is not immediately followed by lower-case characters, separate it out anyway
	Replace("(·)(?=$|[^\\p{Ll}])", " $1 ")
);

auto AggressiveHyphenSplit = Replace("([\\p{IsAlnum}])\\-(?=[\\p{IsAlnum}])", "$1 @-@ ");

auto SeparateCommaInNumbers = Chain(
	// separate out "," except if within numbers (5,300)
   // previous "global" application skips some:  A,B,C,D,E > A , B,C , D,E
   // first application uses up B so rule can't see B,C
   // two-step version here may create extra spaces but these are removed later
   // will also space digit,letter or letter,digit forms (redundant with next section)
	Replace("([^\\p{IsN}])[,]", "$1 , "),
	Replace("[,]([^\\p{IsN}])", ", $1"),
	// Separate "," after a number if it's the end of a sentence
	Replace("([\\p{IsN}])[,]$", "$1 , ")
);

auto EnSpecificApostrophe = Chain(
	// Split contractions right
	Replace("([^\\p{IsAlpha}])[']([^\\p{IsAlpha}])", "$1 ' $2"),
	Replace("([^\\p{IsAlpha}\\p{IsN}])[']([\\p{IsAlpha}])", "$1 ' $2"),
	Replace("([\\p{IsAlpha}])[']([^\\p{IsAlpha}])", "$1 ' $2"),
	Replace("([\\p{IsAlpha}])[']([\\p{IsAlpha}])", "$1 '$2"),
	// Special case for "1990's"
	Replace("([\\p{IsN}])[']([s])", "$1 '$2")
);

auto FrItGaCaSpecificApostrophe = Chain(
	// Split contractions left
	Replace("([^\\p{IsAlpha}])[']([^\\p{IsAlpha}])", "$1 ' $2"),
  Replace("([^\\p{IsAlpha}])[']([\\p{IsAlpha}])", "$1 ' $2"),
  Replace("([\\p{IsAlpha}])[']([^\\p{IsAlpha}])", "$1 ' $2"),
  Replace("([\\p{IsAlpha}])[']([\\p{IsAlpha}])", "$1' $2")
);

auto SoSpecificApostrophe = Chain(
	// Don't split glottals
	Replace("([^\\p{IsAlpha}])[']([^\\p{IsAlpha}])", "$1 ' $2"),
  Replace("([^\\p{IsAlpha}])[']([\\p{IsAlpha}])", "$1 ' $2"),
  Replace("([\\p{IsAlpha}])[']([^\\p{IsAlpha}])", "$1 ' $2")
);

auto NonSpecificApostrophe = Replace("\\'", " ' ");

auto TrailingDotApostrophe = Replace("\\.' ?$", " . ' ");

auto EscapeSpecialChars = Chain(
	Replace("\\&", "&amp;"),   // escape escape
	Replace("\\|", "&#124;"),  // factor separator
	Replace("\\<", "&lt;"),    // xml
	Replace("\\>", "&gt;"),    // xml
	Replace("\\'", "&apos;"),  // xml
	Replace("\\\"", "&quot;"),  // xml
	Replace("\\[", "&#91;"),   // syntax non-terminal
	Replace("\\]", "&#93;")    // syntax non-terminal
);

auto TOKEN_ENDS_WITH_PERIOD_REGEX = std::regex("^(\\S+)\\.$");

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

auto ContainsDot = Search("\\.");

auto ContainsAlpha = Search("[\\p{IsAlpha}]");

auto StartsLowerCase = Search("^[\\p{IsLower}]");

auto StartsNumeric = Search("^[0-9]");

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
	out.clear();
	out.assign(text);

	// De-duplicate spaces and clean ASCII junk
	::DeduplicateSpace(out);
	::RemoveASCIIJunk(out);

	// If protected patterns
	// TODO: implement

	// Strips heading and trailing spaces.
	boost::algorithm::trim(out);

	// Separate out all "other" special characters
	pad_nonalpha_op_(out);

	// Aggressively splits dashes
	if (options_ & aggressive)
		::AggressiveHyphenSplit(out);

	// Multi-dots stay together
	::ReplaceMultidot(out);

	// Separate out "," except if within numbers e.g. 5,300
	::SeparateCommaInNumbers(out);

	// (Language-specific) apostrophe tokenization.
	apostrophe_op_(out);

  HandleNonbreakingPrefixes(out);

  // Cleans up extraneous spaces.
  ::DeduplicateSpace(out);
  boost::algorithm::trim(out);

  // .' at end of sentence is missed
  ::TrailingDotApostrophe(out);

  // Restore protected
  // TODO: implement

  // Restore mutli-dot
  ::RestoreMultidot(out);

  // Escape special chars
  if (!(options_ & no_escape))
  	::EscapeSpecialChars(out);

	return out;
}

void Tokenizer::HandleNonbreakingPrefixes(std::string &out) const {
	// TODO: Go for a faster more efficient implementation
	std::vector<std::string> tokens;
	boost::algorithm::split(tokens, out, boost::is_space());

	out.clear();

	std::smatch match;
	for (std::size_t i = 0; i < tokens.size(); ++i) {
		if (std::regex_match(tokens[i], match, ::TOKEN_ENDS_WITH_PERIOD_REGEX)) {
			// Split last words independently as they are unlikely to be non-breaking prefixes
			if (i == tokens.size() - 1) {
				tokens[i] = match.str(1) + " .";
			} else if (::ContainsDot(match.str(1)) && ::ContainsAlpha(match.str(1))) {
				// no change
			} else if (prefix_set_.IsNonbreakingPrefix(match.str(1))) {
				// no change
			} else if (::StartsLowerCase(tokens[i + 1])) {
				// no change
			} else if (::StartsNumeric(tokens[i + 1]) && prefix_set_.IsNumericNonbreakingPrefix(match.str(1))) {
				// no change
			} else {
				tokens[i] = match.str(1) + " .";
			}
		}
		out.append(tokens[i]);
		out.append(" ");
	}
}

} } // end namespace
