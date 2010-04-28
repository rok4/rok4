#ifndef CONVERT_H
#define CONVERT_H

template<typename F, typename T>
int convert(T* to, F* from, size_t length) {
  for(size_t i = 0; i < length; i++) to[i] = (T) from[i];
  return length;
}

#endif
