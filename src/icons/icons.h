
#ifndef ICONS_H
#define ICONS_H

#include "icons_extension.h"
#include "icons_filename.h"
#include <string.h>

typedef struct {
  const char *icon;
  const char *color;
} IconResult;

IconResult icon_ext(const char *extension) {
  for (size_t i = 0;
       i < sizeof(extension_mappings) / sizeof(extension_mappings[0]); i++) {
    if (!strcasecmp(extension, extension_mappings[i].extension)) {
      return (IconResult){extension_mappings[i].icon,
                          extension_mappings[i].color};
    }
  }
  return (IconResult){"", "#66767b"};
}

IconResult icon_filename(const char *filename) {
  for (size_t i = 0;
       i < sizeof(filename_mappings) / sizeof(filename_mappings[0]); i++) {
    if (!strcasecmp(filename, filename_mappings[i].filename)) {
      return (IconResult){filename_mappings[i].icon,
                          filename_mappings[i].color};
    }
  }

  return (IconResult){"", "#6D8086"};
}
/**/
#endif
