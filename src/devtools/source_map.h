#pragma once
#include <vector>
#include <string>

typedef struct SourceMapLine {
    unsigned int id;
    unsigned int file;
    unsigned int line;
    bool has_type;
    unsigned int type;
    bool has_span;
    unsigned int span;
} SourceMapLine;

typedef struct SourceMapFile{
    unsigned int id;
    std::string name;
    bool contents_cached;
    std::vector<std::string> contents;
} SourceMapFile;

typedef struct SourceSpan {
    unsigned int id;
    unsigned int segment;
    unsigned int start;
    unsigned int size;
    bool has_type;
    unsigned int type;
} SourceMapSpan;

typedef struct SourceMapSegment {
    unsigned int id;
    std::string name;
    unsigned int start;
    unsigned int size;
    bool has_bank;
    unsigned int bank;
} SourceMapSegment;

typedef struct SourceMapSearchResult {
    bool found;
    SourceMapFile *file;
    SourceMapLine *line;
    unsigned int debug;
} SourceMapSearchResult;

class SourceMap {
private:
    std::vector<SourceMapLine> lines;
    std::vector<SourceMapFile> files;
    std::vector<SourceMapSpan> spans;
    std::vector<SourceMapSegment> segments;
public:
    std::string project_root;
    static SourceMap* singleton;
    void GetFileContent(SourceMapFile &sourceMapFile);
    SourceMap(std::string& dbg_file_path);
    SourceMapSearchResult Search(uint16_t addr, uint8_t bank);
};