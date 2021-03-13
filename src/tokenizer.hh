#ifndef TOKENIZER_HH
#define TOKENIZER_HH

#include "regex.hh"
#include "nonbreaking_prefix_set.hh"
#include <string>
#include <functional>

namespace moses { namespace tokenizer {

class Tokenizer {
	public:
		enum class Options : uint32_t {
			none       = 0,
			aggressive = (1 << 0),
			no_escape  = (1 << 1)
		};

		Tokenizer(const std::string &language, Options options = static_cast<Options>(0));
		std::string &operator()(const std::string &text, std::string &out) const;
	private:
		void HandleNonbreakingPrefixes(string_type &text, string_type &out) const;

		Options options_;
		std::string language_;
		NonbreakingPrefixSet const &prefix_set_;
		std::function<void(string_type &, string_type &)> pad_nonalpha_op_;
		std::function<void(string_type &, string_type &)> apostrophe_op_;
};

constexpr Tokenizer::Options operator|(Tokenizer::Options x, Tokenizer::Options y) {
	return static_cast<Tokenizer::Options>(static_cast<uint32_t>(x) | static_cast<uint32_t>(y));
}

constexpr Tokenizer::Options operator&(Tokenizer::Options x, Tokenizer::Options y) {
	return static_cast<Tokenizer::Options>(static_cast<uint32_t>(x) & static_cast<uint32_t>(y));
}

inline Tokenizer::Options& operator|=(Tokenizer::Options& x, Tokenizer::Options y) {
	x = x | y; return x;
}

} } // end namespace

#endif
