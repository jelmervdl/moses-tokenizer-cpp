#ifndef PERL_REGEX
#define PERL_REGEX

#include <string>
#include <boost/regex/icu.hpp>
#include <boost/container_hash/hash.hpp>

namespace moses { namespace tokenizer {

typedef UChar32 char_type;

typedef std::vector<char_type> string_type;

void StrToUChar(std::string const &str, string_type &vec);

void UCharToStr(string_type const &vec, std::string &str);

string_type StrToUChar(std::string const &str); // handy

class ReplaceOp {
public:
	ReplaceOp(std::string const &pattern, std::string const &replacement, std::string const &original_pattern);
	ReplaceOp(std::string const &pattern, std::string const &replacement);
	void operator()(string_type &text, string_type &out) const;
private:
	std::string pattern_;
	boost::u32regex regex_;
	std::string replacement_;
};

class SearchOp {
public:
	SearchOp(std::string const &pattern);
	bool operator()(string_type const &text) const;
private:
	boost::u32regex regex_;
};

template <typename... T>
struct ChainOp {
	inline void operator()(string_type &text, string_type &out) const {
		std::swap(text, out);
	}
};

template <typename T, typename... R>
struct ChainOp<T, R...> {
	ChainOp(T&& op, R&& ...rest)
	: op(op), rest(std::forward<R>(rest)...) {
		//
	};
	
	inline void operator()(string_type &text, string_type &out) const {
		op(text, out);
		std::swap(text, out);
		rest(text, out);
	}

	T op;
	ChainOp<R...> rest;
};

template <typename Init, typename Cond, typename Op, typename Fin>
struct LoopOp {
	Init initial;
	Cond condition;
	Op operation;
	Fin finalize;

	LoopOp(Init initial, Cond condition, Op operation, Fin finalize)
	: initial(initial)
	, condition(condition)
	, operation(operation)
	, finalize(finalize) {
		//
	}

	void operator()(string_type &text, string_type &out) const
	{
		initial(text, out);
		std::swap(out, text);
		while (condition(text)) {
			operation(text, out);
			std::swap(out, text);
		}
		finalize(text, out);
	}
};

struct Noop {
	void operator()(string_type &text, string_type &out) const {
		std::swap(text, out);
	}
};

/**
 * Shortcuts
 */

SearchOp Search(const std::string &pattern);

ReplaceOp Replace(const std::string &pattern, const std::string &replacement);

template <typename... T> ChainOp<T...>
Chain(T&&... args) {
	return ChainOp<T...>(std::forward<T>(args)...);
}

template <typename Init, typename Cond, typename Op, typename Fin>
LoopOp<Init, Cond, Op, Fin> Loop(Init initial, Cond condition, Op operation, Fin finalize) {
	return LoopOp<Init, Cond, Op, Fin>(initial, condition, operation, finalize);
}

} } // enc namespace

namespace std {

template<> struct hash<moses::tokenizer::string_type> {
	std::size_t operator()(moses::tokenizer::string_type const &str) const {
		std::size_t seed = 0;
		for (auto chr : str)
			boost::hash_combine(seed, chr);
		return seed;
	}
};

}

#endif
