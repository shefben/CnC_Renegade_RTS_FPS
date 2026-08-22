#ifndef _DEBUG_H_
#define _DEBUG_H_


#ifdef _DEBUG
IStream* GenerateMemStatsPage();
#else //_DEBUG
#define GenerateMemStatsPage() (NULL)
#endif //!_DEBUG

#endif //!_DEBUG_H_
