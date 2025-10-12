
#ifndef ICONS_FILENAME_H
#define ICONS_FILENAME_H

#include <stdio.h>
typedef struct
{
  const char* filename;
  const char* icon;
  const char* color;
} FilenameMapping;

extern const FilenameMapping filename_mappings[];
extern const size_t fnlen;

#endif
