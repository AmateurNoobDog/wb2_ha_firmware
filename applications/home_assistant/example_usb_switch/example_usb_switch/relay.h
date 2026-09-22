#ifndef __RELAY_H__
#define __RELAY_H__

#include <stdbool.h>

void relay_init(void);
void relay_set(int idx, bool on);
bool relay_get(int idx);

#endif /* __RELAY_H__ */