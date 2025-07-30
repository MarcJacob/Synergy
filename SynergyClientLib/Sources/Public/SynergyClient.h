
#ifndef SYNERGY_CLIENT_INCLUDED
#define SYNERGY_CLIENT_INCLUDED

#define DLL_EXPORT extern "C" __declspec(dllexport)

// Test function outputting Hello World sentence and version info on std::out for testing.
DLL_EXPORT void Hello();

#endif