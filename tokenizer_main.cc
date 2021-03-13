#include <fstream>
#include <iostream>
#include "src/tokenizer.hh"
#include <boost/regex/icu.hpp>

using moses::tokenizer::Tokenizer;

int usage(char *progname) {
	std::cerr << "Usage: " << progname << " -l language\n"
	             "\n"
	             "Any line in stdin will be tokenized to stdout.\n"
	          << std::endl;
	return 1;
}

void ProcessStream(Tokenizer const &tokenizer, std::istream &in, std::ostream &out) {
	std::string text, tokenized;
	while (std::getline(in, text))
		out << tokenizer(text, tokenized) << std::endl;
}

int ProcessFiles(Tokenizer const &tokenizer, int argc, char *argv[], std::ostream &out) {
	int i = 0;
	do {
			if (i == argc || argv[i] == std::string("-"))
				ProcessStream(tokenizer, std::cin, out);
			else {
				std::ifstream in(argv[i]);
				ProcessStream(tokenizer, in, out);
			}
		} while (++i < argc);

	return 0;
}

int TestExpression(std::string const &pattern, std::string const &replacement) {
	auto regex = boost::make_u32regex(pattern, boost::regex::perl);
	std::string text;
	while (std::getline(std::cin, text))
		std::cout << boost::u32regex_replace(text, regex, replacement) << std::endl;
	return 0;
}

int main(int argc, char *argv[]) {
	std::string language("en");
	std::string output("-");
	int filename_i = argc;
	Tokenizer::Options options(Tokenizer::Options::none);

	for (int i = 1; i < argc; ++i) {
		std::string arg(argv[i]);

		if (arg == "-b") // Disable buffering: not implemented
			continue;

		else if (arg == "-l") {
			if (i + 1 == argc)
				return usage(argv[0]);
			
			language = argv[++i];
			continue;
		}

		else if (arg == "-q") // Quiet: we're always quiet
			continue;

		else if (arg == "-h")
			return usage(argv[0]);

		else if (arg == "-x") // Skip XML: not implemented
			continue;

		else if (arg == "-a")
			options |= Tokenizer::Options::aggressive;

		else if (arg == "-time") // Timing: not implemented, just use `/usr/bin/time`
			continue;

		else if (arg == "-protected") {
			std::cerr << "-protected not implemented" << std::endl;
			return 1;
		}

		else if (arg == "-threads") // Threads not implemented
			++i;

		else if (arg == "-lines")
			++i;

		else if (arg == "-penn") {
			std::cerr << "-penn not implemented" << std::endl;
			return 1;
		}

		else if (arg == "-no-escape")
			options |= Tokenizer::Options::no_escape;
		
		else if (arg == "-o") {
			if (i + 1 == argc)
				return usage(argv[0]);

			output = argv[++i];
		}

		else if (arg == "--test"){
			if (i + 2 >= argc)
				return usage(argv[0]);
			
			return TestExpression(argv[i+1], argv[i+2]);
		}

		else if (arg.size() > 1 && arg.substr(0, 1) == "-")
			return usage(argv[0]);

		else {
			filename_i = i;
			break;
		}
	}

	Tokenizer tokenizer(language, options);

	if (output.empty() || output == "-") {
		return ProcessFiles(tokenizer, argc - filename_i, argv + filename_i, std::cout);
	} else {
		std::ofstream out(output);
		return ProcessFiles(tokenizer, argc - filename_i, argv + filename_i, out);
	}
}
