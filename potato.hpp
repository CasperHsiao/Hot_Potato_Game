#ifndef __POTATO_H__
#define __POTATO_H__

#define MAX_HOPS 512

struct potato {
  int size;
  int hops_left;
  int trace[MAX_HOPS];
};

#endif
