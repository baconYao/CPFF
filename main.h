#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

#include "./configs/structure.h"
#include "./modules/prize_caching_manager/prize_caching.h"

/* 選擇SSD Cache管理測略 */
#ifdef STATIC_CACHING_SPACE
  #include "./modules/caching_space_manager/static_caching_space.h"
#elif defined DYNAMIC_CACHING_SPACE
  #include "./modules/caching_space_manager/dynamic_caching_space.h"
#elif defined COMPETITION_CACHING_SPACE
  #include "./modules/caching_space_manager/compition_caching_space.h"
#endif

#include "./modules/debug/debug.h"