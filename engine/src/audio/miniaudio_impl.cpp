// The single-header miniaudio implementation, isolated in its own translation
// unit so the (very large) implementation compiles once instead of on every
// change to AudioInput.cpp.
#define MINIAUDIO_IMPLEMENTATION
#include "miniaudio.h"
