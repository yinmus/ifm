
#include <stdio.h>
#ifndef ICONS_EXTENSION_H
#define ICONS_EXTENSION_H

typedef struct
{
  const char* extension;
  const char* icon;
  const char* color;
} ExtensionMapping;

extern const ExtensionMapping extension_mappings[];
extern const size_t extlen;

#endif
