#ifndef PERL_REGEX
#define PERL_REGEX

#include <string>
#include <regex>

namespace moses { namespace tokenizer {

class ReplaceOp {
public:
	ReplaceOp(std::string const &pattern, std::string const &replacement, std::string const &original_pattern);
	ReplaceOp(std::string const &pattern, std::string const &replacement);
	std::string &operator()(std::string &text) const;
private:
	std::string pattern_;
	std::regex regex_;
	std::string replacement_;
};

class SearchOp {
public:
	SearchOp(std::string const &pattern);
	bool operator()(std::string const &text) const;
private:
	std::regex regex_;
};

template <typename... T>
struct ChainOp {
	inline void operator()(std::string &) const {
		// void
	}
};

template <typename T, typename... R>
struct ChainOp<T, R...> {
	ChainOp(T&& op, R&& ...rest)
	: op(op), rest(std::forward<R>(rest)...) {
		//
	};
	
	inline void operator()(std::string &text) const {
		op(text);
		rest(text);
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

	void operator()(std::string &out) const
	{
		initial(out);
		while (condition(out))
			operation(out);
		finalize(out);
	}
};

struct Noop {
	void operator()(std::string &) const {
		// do nothing!
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

std::string FormatPerlRegex(std::string const &pattern);

} } // enc namespace

#endif
