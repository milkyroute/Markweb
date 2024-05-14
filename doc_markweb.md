# Markweb

Markweb is a literate programing tool based on markdown. Its goal is to rejuvenate literate programing by using a more recent and easy-to-learn markup language than LaTeX, but with indexing support to allow create references and crosslinking inside the documentation. Its objective are to allow writing documentation and software at the same time. Markweb generates a proper markdown document that contains the document of the program as well as the program files ready to be compliled. The first supported language will be C++.

Contrarily to CWEB, Markweb does not differenciate between weave and tangle: it calls both at the same time. This is because weave (the action of creating the documentation file) needs tangle (the action of creating the code files) to have the crosslinks and references.

## Introduction

Before beginning any work, the program will check the given console arguments for the source file to load. In C++, the arguments @main>argc and @main>argv contain the information given in the console. First, we put them in C++ objects.

###### Converting console arguments
```cpp
for(int i = 1; i < argc; i++) {
	arguments.push_back(std::string(argv[i]));
}
```

If there is no arguments, this operation is not needed and the program must return an error. For this, we'll need to include `<iostream>`.

###### Includes
```cpp
#include <iostream>
```

###### Not enough arguments
```cpp
std::cerr << "Not enough arguments given. You have to use \"markweb\"." << std::endl;
return 1;
```

When we're sure we have enough arguments, we open the file and reads its content. It is stored in a vector so the line can be indicated if there is an error.

###### Includes
```cpp
#include <fstream>
#include <vector>
```

###### File opening and reading
```cpp
std::ifstream input_file(arguments[0]);
@{Checking if the file has correctly been opened}
std::string buffer;
std::vector<std::string> lines;
while(!input_file.eof()) {
	std::getline(input_file, buffer);
	lines.push_back(buffer);
}
```

We need to check if the second argument is really a file.

###### Checking if the file has correctly been opened
```cpp
if(!input_file) {
	std::cerr << "Can't open file \"" << arguments[1] << "\"." << std::endl;
	return 2;
}
```

The introduction of the program can be summarized like this:

###### Loading
```cpp
std::vector<std::string> arguments;
if(argc >= 2) {
	@{Converting console arguments}
} else {
	@{Not enough arguments}
}

@{File opening and reading}
```

As we'll see later, the main function is organized as follows:

###### Main
```cpp
int main(int argc, char* argv[]) {
	@{Loading}
	@{Tangle}
	@{Weave}
	return 0;
}
```

## Tangle

This part of the program creates the C++ files from the markweb code. It parses code chunks and finally puts them together in the right order, one file at a time. Its steps are the following:

###### Tangle
```cpp
@{Parsing code chunks}
@{Reconstructing code and output}
```

### Parsing code chunks

To parse the code chunks, we must first define how they are going to be stored. We'll first declare a structure defining a chunk.

###### Chunk structure
```cpp
struct Chunk {
std::vector<std::vector<std::string>> content = std::vector<std::vector<std::string>>(1);
std::vector<int> lines = std::vector<int>();
std::string name = "";
bool complete = false; // If true, all referenced chunks have been replaced by their content.
};
```

Then we can parse all chunks. 

###### Includes
```cpp
#include <map>
```

###### Parsing code chunks
```cpp
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
			@{Append to chunk}
		} else if(line.ends_with("=")) {
			@{Create new chunk}
		} else {
			std::cerr << "[ERROR] At line " << i << ": unknown code chunk suffix." << std::endl;
			return 3;
		}
	}
	i++;
}
```

When a new chunk is found, we need to parse its name and create the object.

###### Create new chunk
```cpp
Chunk c;
c.name = std::regex_replace(line.substr(3), std::regex("="), "");
if(c.name.starts_with("*")) {// If file chunk
	c.name = c.name.substr(1);
	files.push_back(c.name);
}
c.lines.push_back(i);
chunks[c.name] = c;
current_chunk = c.name;
```

If the code chunk ends with `+=`, it appends code to an existing chunk instead of creating a new one.

###### Append to chunk
```cpp
std::string cname = std::regex_replace(line.substr(line.starts_with("@@@*") ? 4 : 3), std::regex("\\+="), "");
if(!chunks.contains(cname)) {
	std::cerr << "[ERROR] At line " << i << ": unknown code chunk to append to." << std::endl;
	return 3;
}
chunks[cname].lines.push_back(i);
chunks[cname].content.push_back(std::vector<std::string>());
current_chunk = cname;
```

At the end of this part, all chunks are stored in the @chunks map. The file chunks, that are going to be the roots from where we'll be reconstructing the code, are stored in the @files vector.

### Reconstructing code

Now, we need to reassemble the code from all chunks. In order to do this, we'll need recursion, so a separate function. It searches for chunk inclusions in the current chunk's content and replaces it with the included chunk's content. It also calls itself for the included chunk if it is not complete (see @Chunk.complete).

###### Includes
```cpp
#include <regex>
```

###### Constants
```cpp
const std::regex CODE_REFERENCE_REGEX("(@)\\{([^:]*)\\}"); 
```

###### Tangle function
```cpp
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
					@{Replace line with chunk content}
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
```

To replace the chunk inclusion line by the included chunk's content, the program must first compile all the chunk's lines into one string.

###### Replace line with chunk content
```cpp
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
```

Now, we need to call this function from @{Tangle} in @{Reconstructing code} on each file.

###### Reconstructing code and output
```cpp
for(std::string const& file : files) {
	if(!tangle(file, chunks, arguments[0])) {
		std::cerr << "[ERROR] Tangle aborted for file \"" << file << "\". See error above." << std::endl;
		return 4;
	}
	@{Tangle output}
}
```

Creating the file is a more straightforward process.

###### Tangle output
```cpp
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
```

The source files are now created and ready to be compiled.

## Weave function

The weave function has three goals: change markweb code chunks to markdown ones, link references to their definition (code chunks and variables) and list code objects definitions below code chunks. For this, we read again the file, line by line, and format the different elements. To avoid reading the file multiple times, each line is written just after it's proccessed.

###### Weave
```cpp
@{Open weave output file}
bool in_code_chunk = false;
for(std::string& line : lines) {
	@{Format code chunks}
	@{Link text references}
	@{Weave output}
}
```

### Format code chunks

This step is quite simple, we only need to replace the markweb format with the markdown one. The code chunks are named with a h6 header.

###### Format code chunks
```cpp
std::smatch match;
bool code_chunk_delimiter = std::regex_search(line, match, std::regex("^\\s*@@@"));
if(code_chunk_delimiter && !in_code_chunk) {
	line = std::regex_replace(line, std::regex("^\\s*@@@\\*?"), "###### ");
	line = std::regex_replace(line, std::regex("\\+?="), "\n```cpp");
	in_code_chunk = true;
} else if(code_chunk_delimiter && in_code_chunk) {
	line = "```";
	in_code_chunk = false;
	@{List definitions}
	@{List code references}
} else if(in_code_chunk) {
	line = std::regex_replace(line, std::regex("^\t"), "");
}
```

### Listing

For convinience, a lot of links are put below the code chunk. In order: the defined code objects, the code chunks included then the code objects used.

TODO

###### List definitions
```cpp
/* TODO */
```

###### List code references
```cpp
/* TODO */
```

### Link references

This step is done in two sub-steps. First, we need to reference the code chunks, and then the code objects. 

TODO

###### Link text references
```cpp
/* TODO */
```

### Writing output file

This step only consists in writing each line of the file. First we open the file and check if there is any problems.

###### Open weave output file
```cpp
std::ofstream weave_output(std::regex_replace("doc_" + arguments[0], std::regex(".mw"), ".md"));
if(!weave_output) {
	std::cerr << "[ERROR] Can't open documentation output file. Documentation will not be created." << std::endl;
	return 5;
}
```

Then we simply write each line at the end of processing.

###### Weave output
```cpp
weave_output << line << std::endl;
```

## Final structure

At the end, the main file of the program is organized as follows.

###### markweb.cpp
```cpp
@{Includes}
@{Constants}

@{Chunk structure}
@{Tangle function}

@{Main}
```

