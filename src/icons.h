/*
======================================
    IFM by yinmus (c) 2025-2025
======================================

Relative path : ifm/src/icons.h
Github url : https://github.com/yinmus/ifm.git
License : GPLv3

*/

#ifndef ICONS_H
#define ICONS_H

#include <string.h>

typedef struct {
  const char* extension;
  const char* icon;
} IconMapping;

static const IconMapping icon_mappings[] = {
    {"txt", ""},      {"c", ""},        {"py", ""},
    {"7z", ""},       {"a", ""},        {"ai", ""},
    {"apk", ""},      {"asm", " "},     {"asp", " "},
    {"aup", ""},      {"avi", ""},      {"awk", ""},
    {"bash", ""},     {"bat", ""},      {"bmp", ""},
    {"bz2", ""},      {"c++", ""},      {"cab", ""},
    {"cbr", ""},      {"cbz", ""},      {"cc", ""},
    {"class", ""},    {"svg", ""},      {"clj", ""},
    {"cljc", ""},     {"cljs", ""},     {"cmake", ""},
    {"coffee", ""},   {"conf", ""},     {"cfg", ""},
    {"cp", ""},       {"cpio", ""},     {"cpp", ""},
    {"cs", "󰌛"},      {"csh", ""},      {"css", ""},
    {"cue", ""},      {"cvs", ""},      {"cxx", ""},
    {"d", ""},        {"dart", ""},     {"db", ""},
    {"deb", ""},      {"diff", ""},     {"dll", ""},
    {"wps", ""},      {"wpt", ""},      {"doc", ""},
    {"docx", ""},     {"docm", ""},     {"dotx", ""},
    {"dotm", ""},     {"dump", ""},     {"edn", ""},
    {"eex", ""},      {"efi", ""},      {"ejs", ""},
    {"elf", ""},      {"elm", ""},      {"epub", ""},
    {"erl", ""},      {"ex", ""},       {"exe", ""},
    {"exs", ""},      {"f#", ""},       {"fifo", "󰟥"},
    {"fish", ""},     {"flac", ""},     {"flv", ""},
    {"fs", ""},       {"fsi", ""},      {"fsscript", ""},
    {"fsx", ""},      {"gem", ""},      {"gemspec", ""},
    {"gif", ""},      {"go", ""},       {"gz", ""},
    {"gzip", ""},     {"h", ""},        {"haml", ""},
    {"hbs", ""},      {"hh", ""},       {"hpp", ""},
    {"hrl", ""},      {"hs", ""},       {"htaccess", ""},
    {"htm", ""},      {"html", ""},     {"htpasswd", ""},
    {"hxx", ""},      {"ico", ""},      {"img", ""},
    {"ini", ""},      {"ipynb", ""},    {"iso", ""},
    {"jar", ""},      {"java", ""},     {"jl", ""},
    {"jpeg", ""},     {"jpg", ""},      {"js", ""},
    {"json", ""},     {"jsonc", ""},    {"jsx", ""},
    {"key", ""},      {"ksh", ""},      {"leex", ""},
    {"less", ""},     {"lha", ""},      {"lhs", ""},
    {"log", ""},      {"lua", ""},      {"lz", ""},
    {"lzh", ""},      {"lzma", ""},     {"m4a", ""},
    {"m4v", ""},      {"markdown", ""}, {"md", ""},
    {"mdx", ""},      {"mjs", ""},      {"mka", ""},
    {"mkv", ""},      {"ml", "λ"},         {"mli", "λ"},
    {"mov", ""},      {"mp3", ""},      {"mp4", ""},
    {"mpeg", ""},     {"mpg", ""},      {"msi", ""},
    {"mustache", ""}, {"nix", ""},      {"o", ""},
    {"ogg", ""},      {"opus", ""},     {"part", ""},
    {"pdf", ""},      {"php", ""},      {"pl", ""},
    {"pm", ""},       {"png", ""},      {"pp", ""},
    {"dps", ""},      {"dpt", ""},      {"ppt", ""},
    {"pptx", ""},     {"pptm", ""},     {"pot", ""},
    {"potx", ""},     {"potm", ""},     {"pps", ""},
    {"ppsx", ""},     {"ppsm", ""},     {"ps1", ""},
    {"psb", ""},      {"psd", ""},      {"pub", ""},
    {"py", ""},       {"pyc", ""},      {"pyd", ""},
    {"pyo", ""},      {"r", "󰟔"},       {"rake", ""},
    {"rar", ""},      {"rb", ""},       {"rc", ""},
    {"rlib", ""},     {"rmd", ""},      {"rom", ""},
    {"rpm", ""},      {"rproj", "󰗆"},   {"rs", ""},
    {"rss", ""},      {"rtf", ""},      {"s", ""},
    {"sass", ""},     {"scala", ""},    {"scss", ""},
    {"sh", ""},       {"slim", ""},     {"sln", ""},
    {"so", ""},       {"sql", ""},      {"styl", ""},
    {"suo", ""},      {"svelte", ""},   {"swift", ""},
    {"t", ""},        {"tar", ""},      {"tex", "󰙩"},
    {"tgz", ""},      {"toml", ""},     {"torrent", ""},
    {"ts", ""},       {"tsx", ""},      {"twig", ""},
    {"vim", ""},      {"vimrc", ""},    {"vue", "󰡄"},
    {"wav", ""},      {"webm", ""},     {"webmanifest", ""},
    {"webp", ""},     {"xbps", ""},     {"xcplayground", ""},
    {"xhtml", ""},    {"et", "󰈛"},      {"ett", "󰈛"},
    {"xls", "󰈛"},     {"xlt", "󰈛"},     {"xlsx", "󰈛"},
    {"xlsm", "󰈛"},    {"xlsb", "󰈛"},    {"xltx", "󰈛"},
    {"xltm", "󰈛"},    {"xla", "󰈛"},     {"xlam", "󰈛"},
    {"xml", ""},      {"xul", ""},      {"xz", ""},
    {"yaml", ""},     {"yml", ""},      {"zip", ""},
    {"zsh", ""},
};

const char* icon_ext(const char* extension) {
  for (size_t i = 0; i < sizeof(icon_mappings) / sizeof(icon_mappings[0]);
       i++) {
    if (!strcasecmp(extension, icon_mappings[i].extension)) {
      return icon_mappings[i].icon;
    }
  }
  return "";
}

#endif
