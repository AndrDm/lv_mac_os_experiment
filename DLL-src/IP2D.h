#ifdef IP2D_EXPORTS
#define IP2D_API extern "C" __declspec(dllexport) 
#else
#define IP2D_API __declspec(dllimport)
#endif

// lv_prolog.h and lv_epilog.h set up the correct alignment for LabVIEW data.
#include "lv_prolog.h"

// Typedefs
typedef struct {
    int32_t dimSizes[1];
    double elt[1];
} fTD;
typedef fTD** fTDHdl;

typedef struct {
    int32_t dimSizes[2];
    uint32_t elt[1];
} iTD;
typedef iTD** iTDHdl;

#include "lv_epilog.h"

IP2D_API unsigned int fnIP2D(iTDHdl arr, double fCol, double fRow, double gamma);
IP2D_API void fnIP2D2(iTDHdl arrSrc, iTDHdl arrDst, fTDHdl fCol, fTDHdl fRow, double gamma);
IP2D_API void fnLUT(double gamma);