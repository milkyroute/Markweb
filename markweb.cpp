#line 20 "markweb.mw"
	#include <iostream>
#line 265 "markweb.mw"
#line 31 "markweb.mw"
	#include <fstream>
	#include <vector>
#line 265 "markweb.mw"
#line 104 "markweb.mw"
	#include <map>
#line 265 "markweb.mw"
#line 167 "markweb.mw"
	#include <regex>
#line 265 "markweb.mw"

#line 171 "markweb.mw"
	const std::regex CODE_REFERENCE_REGEX("(@)\\{([^:]*)\\}"); 
#line 266 "markweb.mw"

	
#line 93 "markweb.mw"
struct Chunk {
	std::vector<std::vector<std::string>> content = std::vector<std::vector<std::string>>(1);
	std::vector<int> lines = std::vector<int>();
	std::string name = "";
	bool complete = false; // If true, all referenced chunks have been replaced by their content.
};
#line 268 "markweb.mw"

#line 175 "markweb.mw"
	bool tangle(std::string chunk_name, std::map<std::string, Chunk>& chunks, std::string file_name) {
		int content_id = 0; /* Used to keep trace of which content declaration is being used */
		for(std::vector<std::string>& sub_content : chunks[chunk_name].content) {
			int line_id = 1;
			for(std::string& line : sub_content) {
				std::smatch match;
				if(std::regex_search(line, match, CODE_REFERENCE_REGEX)) {
					std::string cname = match[2].str();
					if(chunks.contains(cname)) {
						if(!chunks[cname].complete) {
							if(!tangle(cname, chunks, file_name)) { // Propagates false if failure
								std::cerr << "=> In chunk \"" << chunk_name << "\" at line " << chunks[chunk_name].lines[content_id] + line_id << "." << std::endl;
								return false;
							}
						}
#line 208 "markweb.mw"
	line = "";
	Chunk included_chunk = chunks[cname];
	int sub_content_id = 0;
	for(std::vector<std::string> const& sub_content : included_chunk.content) {
		line += "#line " + std::to_string(included_chunk.lines[sub_content_id] + 1) + " \"" + file_name + "\"\n";
		for(std::string const& ic_line : sub_content) {
			line += ic_line + "\n";
		}
		line += "#line " + std::to_string(chunks[chunk_name].lines[content_id] + line_id + 1) + " \"" + file_name + "\"\n";
		sub_content_id++;
	}
#line 191 "markweb.mw"

					} else {
						std::cerr << "[ERROR] In chunk \"" << chunk_name << "\" at line " << chunks[chunk_name].lines[content_id] + line_id << ": unknown reference to chunk named \"" << cname << "\"" << std::endl;
						return false;
					}
				}
				line_id++;
			}
			content_id++;
		}
		chunks[chunk_name].complete = true;
		return true;
	}
#line 269 "markweb.mw"

	
#line 71 "markweb.mw"
	int main(int argc, char* argv[]) {
#line 58 "markweb.mw"
	std::vector<std::string> arguments;
	if(argc >= 2) {
#line 12 "markweb.mw"
	for(int i = 1; i < argc; i++) {
		arguments.push_back(std::string(argv[i]));
	}
#line 61 "markweb.mw"

	} else {
#line 24 "markweb.mw"
	std::cerr << "Not enough arguments given. You have to use \"markweb\"." << std::endl;
	return 1;
#line 63 "markweb.mw"

	}
	
#line 36 "markweb.mw"
	std::ifstream input_file(arguments[0]);
#line 49 "markweb.mw"
	if(!input_file) {
		std::cerr << "Can't open file \"" << arguments[1] << "\"." << std::endl;
		return 2;
	}
#line 38 "markweb.mw"

	std::string buffer;
	std::vector<std::string> lines;
	while(!input_file.eof()) {
		std::getline(input_file, buffer);
		lines.push_back(buffer);
	}
#line 66 "markweb.mw"

#line 73 "markweb.mw"

#line 84 "markweb.mw"
#line 108 "markweb.mw"
	std::map<std::string, Chunk> chunks;
	std::vector<std::string> files; //Used to store the chunks that define files. They are the root chunks.
	std::string current_chunk = "";
	int i = 1;
	for(std::string const& line : lines) {
		if(current_chunk != "") {
			if(line == "@@@") { // End of the chunk
				current_chunk = "";
			} else { // Content of the chunk
				chunks[current_chunk].content[chunks[current_chunk].content.size() - 1].push_back(line);
			}
		} else if(line.starts_with("@@@")) { // Beginning of a chunk
			if(line.ends_with("+=")) {
#line 150 "markweb.mw"
	std::string cname = std::regex_replace(line.substr(line.starts_with("@@@*") ? 4 : 3), std::regex("\\+="), "");
	if(!chunks.contains(cname)) {
		std::cerr << "[ERROR] At line " << i << ": unknown code chunk to append to." << std::endl;
		return 3;
	}
	chunks[cname].lines.push_back(i);
	chunks[cname].content.push_back(std::vector<std::string>());
	current_chunk = cname;
#line 122 "markweb.mw"

			} else if(line.ends_with("=")) {
#line 136 "markweb.mw"
	Chunk c;
	c.name = std::regex_replace(line.substr(3), std::regex("="), "");
	if(c.name.starts_with("*")) {// If file chunk
		c.name = c.name.substr(1);
		files.push_back(c.name);
	}
	c.lines.push_back(i);
	chunks[c.name] = c;
	current_chunk = c.name;
#line 124 "markweb.mw"

			} else {
				std::cerr << "[ERROR] At line " << i << ": unknown code chunk suffix." << std::endl;
				return 3;
			}
		}
		i++;
	}
#line 85 "markweb.mw"

#line 224 "markweb.mw"
	for(std::string const& file : files) {
		if(!tangle(file, chunks, arguments[0])) {
			std::cerr << "[ERROR] Tangle aborted for file \"" << file << "\". See error above." << std::endl;
			return 4;
		}
#line 236 "markweb.mw"
	std::ofstream output_file(file);
	if(!output_file) {
		std::cerr << "[ERROR] At line " << chunks[file].lines[0] << ": can't open output file \"" << file << "\"." << std::endl;
		return 5;
	}
	for(std::vector<std::string> const& sub_content : chunks[file].content) {
		for(std::string const& line : sub_content) {
			output_file << line << std::endl;
		}
	}
	output_file.close();
#line 230 "markweb.mw"

	}
#line 86 "markweb.mw"

#line 74 "markweb.mw"

#line 256 "markweb.mw"
	// TODO
#line 75 "markweb.mw"

		return 0;
	}
#line 271 "markweb.mw"

