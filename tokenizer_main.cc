#include <fstream>
#include <iostream>
#include "src/tokenizer.hh"

using moses::tokenizer::Tokenizer;

int usage(char *progname) {
	std::cerr << "Usage: " << progname << " -l language\n"
	             "\n"
	             "Any line in stdin will be tokenized to stdout.\n"
	          << std::endl;
	return 1;
}

void process(Tokenizer const &tokenizer, std::istream &in, std::ostream &out) {
	std::string text, tokenized;
	while (std::getline(in, text))
		out << tokenizer(text, tokenized) << std::endl;
}

int process_files(Tokenizer const &tokenizer, int argc, char *argv[], std::ostream &out) {
	int i = 0;
	do {
			if (i == argc || argv[i] == std::string("-"))
				process(tokenizer, std::cin, out);
			else {
				std::ifstream in(argv[i]);
				process(tokenizer, in, out);
			}
		} while (++i < argc);
}

int main(int argc, char *argv[]) {
	std::string language("en");
	std::string output("-");
	int filename_i = argc;
	Tokenizer::Options options;

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
			options |= Tokenizer::aggressive;

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
			options |= Tokenizer::no_escape;

		else if (arg == "-o")
			output = argv[++i];

		else if (arg.size() > 1 && arg.substr(0, 1) == "-")
			return usage(argv[0]);

		else {
			filename_i = i;
			break;
		}
	}

	Tokenizer tokenizer(language, options);

	if (output.empty() || output == "-") {
		return process_files(tokenizer, argc - filename_i, argv + filename_i, std::cout);
	} else {
		std::ofstream out(output);
		return process_files(tokenizer, argc - filename_i, argv + filename_i, out);
	}
}
