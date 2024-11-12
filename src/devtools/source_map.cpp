#include "source_map.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include <unordered_map>
#include <algorithm>
#include <filesystem>

SourceMap* SourceMap::singleton = nullptr;

// Helper function to parse a line into type name and key-value pairs
bool parseLine(const std::string& line, std::string& typeName, std::unordered_map<std::string, std::string>& keyValuePairs) {
    std::istringstream lineStream(line);

    // Get the type name before the tab character
    if (!std::getline(lineStream, typeName, '\t')) {
        return false;
    }

    std::string keyValuePart;
    // Get the key-value pairs after the tab character
    if (std::getline(lineStream, keyValuePart)) {
        std::istringstream kvStream(keyValuePart);
        std::string kvPair;

        // Parse each key-value pair separated by commas
        while (std::getline(kvStream, kvPair, ',')) {
            std::string key, value;
            std::istringstream kvPairStream(kvPair);
            
            // Split each pair into key and value based on '='
            if (std::getline(kvPairStream, key, '=') && std::getline(kvPairStream, value)) {
                keyValuePairs[key] = value;
            } else {
                return false;
            }
        }
    } else {
        return false;
    }

    return true;
}

bool compareSMLines(const SourceMapLine &a, const SourceMapLine &b) {
    return a.id < b.id;
}

bool compareSMFiles(const SourceMapFile &a, const SourceMapFile &b) {
    return a.id < b.id;
}

bool compareSMSegments(const SourceMapSegment &a, const SourceMapSegment &b) {
    return a.id < b.id;
}

bool compareSMSpans(const SourceMapSpan &a, const SourceMapSpan &b) {
    return a.id < b.id;
}

SourceMap::SourceMap(std::string& dbg_file_path) {
    std::filesystem::path fpath(dbg_file_path);
    project_root = fpath.parent_path().parent_path().string();
    std::ifstream dbgfile(dbg_file_path);
    if(!dbgfile.is_open()) {
        throw std::runtime_error("Unable to open " + dbg_file_path);
    }

    std::string line;

    std::unordered_map<std::string, std::vector<std::unordered_map<std::string, std::string>>> dbg_table;

    while (std::getline(dbgfile, line)) {
        std::string typeName;
        std::unordered_map<std::string, std::string> keyValuePairs;

        // Parse type name and key-value pairs
        if (parseLine(line, typeName, keyValuePairs)) {
            dbg_table[typeName].emplace_back(keyValuePairs);
        }
    }

    for(auto& line_map : dbg_table["line"]) {
        SourceMapLine entry;
        entry.id = std::stoi(line_map["id"]);
        entry.file = std::stoi(line_map["file"]);
        entry.line = std::stoi(line_map["line"]);
        entry.has_span = line_map.contains("span");
        if(entry.has_span) {
            entry.span = std::stoi(line_map["span"]);
        }
        entry.has_type = line_map.contains("type");
        if(entry.has_type) {
            entry.type = std::stoi(line_map["type"]);
        }
        lines.emplace_back(entry);
    }

    for(auto& line_map : dbg_table["file"]) {
        SourceMapFile entry;
        entry.id = std::stoi(line_map["id"]);
        entry.contents_cached = false;
        entry.name = line_map["name"];
        entry.name.erase(0, 1);            // Remove the first character
        entry.name.erase(entry.name.size() - 1);  // Remove the last character
        files.emplace_back(entry);
    }

    for(auto& line_map : dbg_table["span"]) {
        SourceMapSpan entry;
        entry.id = std::stoi(line_map["id"]);
        entry.segment = std::stoi(line_map["seg"]);
        entry.size = std::stoi(line_map["size"]);
        entry.start = std::stoi(line_map["start"]);
        entry.has_type = line_map.contains("type");
        if(entry.has_type) {
            entry.type = std::stoi(line_map["type"]);
        }
        spans.emplace_back(entry);
    }

    for(auto& line_map : dbg_table["seg"]) {
        SourceMapSegment entry;
        entry.id = std::stoi(line_map["id"]);
        entry.name = line_map["name"];
        entry.size = std::stoi(line_map["size"], nullptr, 16);
        entry.start = std::stoi(line_map["start"], nullptr, 16);
        entry.has_bank = line_map.contains("bank");
        if(entry.has_bank) {
            entry.bank = std::stoi(line_map["bank"]);
        }
        segments.emplace_back(entry);
    }

    std::sort(lines.begin(), lines.end(), compareSMLines);
    std::sort(files.begin(), files.end(), compareSMFiles);
    std::sort(segments.begin(), segments.end(), compareSMSegments);
    std::sort(spans.begin(), spans.end(), compareSMSpans);
}

SourceMapSearchResult SourceMap::Search(uint16_t addr, uint8_t bank) {

    SourceMapSearchResult result = {
        false, nullptr, nullptr, 0
    };

    bank &= 127;
    if(addr < 0x8000) {
        std::cout << "WARNING: mapping ramfuncs isn't supported yet but I'll try my best...\n";
    }

    if(addr >= 0xC000) {
        bank = 127;
    }

    int seg_idx = -1;
    uint16_t rel_addr = 0;

    for(auto& seg: segments) {
        if(seg.has_bank && ((seg.bank & 127) == bank)) {
            if((addr >= seg.start) && ((addr - seg.start) < seg.size)) {
                seg_idx = seg.id;
                rel_addr = addr - seg.start;
                break;
            }
        }
    }
    if(seg_idx == -1) {
        result.debug = 1;
        return result;
    }

    int spans_found = 0;
#define MAX_MATCHING_SPANS 3
    int matching_spans[3];
    for(auto& span: spans) {
        if(span.segment == seg_idx) {
            if((rel_addr >= span.start) && ((rel_addr - span.start) < span.size)) {
                    matching_spans[spans_found++] = span.id;
                    if(spans_found == MAX_MATCHING_SPANS)
                        break;
            }
        }
    }

    if(spans_found == 0) {
        result.debug = 2;
        return result;
    }


    int line_idx = -1;
    int first_found_line = -1;
    result.debug = seg_idx;
    for(int i = 0; i < spans_found; ++i) {
            for(auto& line: lines) {
            if(line.has_span && (line.span == matching_spans[i])) {
                if(first_found_line == -1)
                    first_found_line = line.id;

                /* Find the C version, presumably meaning type 1 */
                if(line.has_type && (line.type == 1)) {
                    line_idx = line.id;
                }
            }
        }
    }

    if(first_found_line == -1) {
        return result;
    }

    if(line_idx == -1) {
        line_idx = first_found_line;
    }

    result.line = &lines[line_idx];
    if((result.line == nullptr) || (result.line->id != line_idx)) {
        throw std::runtime_error("line at index doesn't match ID!");
    }
    result.file = &files[result.line->file];
    if((result.file == nullptr) || (result.file->id != result.line->file)) {
        throw std::runtime_error("file at index doesn't match ID!");
    }
    result.found = true;

    return result;
}

void SourceMap::GetFileContent(SourceMapFile &sourceMapFile) {
    // Check if contents are already cached
    if (sourceMapFile.contents_cached) {
        return; // Contents are already loaded, no further action required
    }

    // Open the file with the name specified in sourceMapFile.name
    std::ifstream file(project_root + "/" + sourceMapFile.name);
    if (!file) {
        return;
    }

    // Clear any existing contents and read the file line-by-line
    sourceMapFile.contents.clear();
    std::string line;
    while (std::getline(file, line)) {
        sourceMapFile.contents.push_back(line);
    }

    // Update the struct to indicate the contents are cached
    sourceMapFile.contents_cached = true;
}