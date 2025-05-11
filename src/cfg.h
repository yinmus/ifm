/*
   cfg.h
   https://github.com/yinmus/ifm.git

*/

#ifndef CFG_H
#define CFG_H

#include "ifm.h"

typedef struct {
  char nxtaftlab[MAX_NAME];

} Config;

void create_default_config();
void load_config(Config *config);

#endif
