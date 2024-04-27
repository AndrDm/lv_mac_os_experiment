#include "framework.h"
#include "IP2D.h"

#define RGB_STEPS 256
#define WEIGHT_STEPS 1024

INT64 LUT[RGB_STEPS]; // Not thread-safe
INT64 WLUT[RGB_STEPS * WEIGHT_STEPS];
static double Gamma = NAN;

IP2D_API void fnLUT(double gamma)
{
    for (int i = 0; i < RGB_STEPS; i++) {
        LUT[i] = (uint64_t)rint((pow((double)i, gamma)));
        for (int j = 0; j < WEIGHT_STEPS; j++) {
            WLUT[j + i * WEIGHT_STEPS] = j * LUT[i];
        }
        LUT[i] *= WEIGHT_STEPS - 1;
    }
}

inline BYTE* getpXY(UINT* image, int cols_width, double row_y, double col_x) {
    return (BYTE*) & (image[((int)row_y * cols_width) + (int)col_x]);
}

inline UINT binarySearch(INT64 LUT[], INT64 Test)
{
    UINT left = 0, right = 255;
    while (left <= right) {
        UINT mid = left + (right - left) / 2;
        if (LUT[mid] <= Test) left = mid + 1;
        else right = mid - 1;
    }
    return (256 == left) ? left-- : left; // if too much, step back
}

// This is an example of an 2D Interpolation with gamma.
IP2D_API UINT fnIP2D(iTDHdl arr, double fCol, double fRow, double gamma)
{
    uint32_t* pix = (*arr)->elt; //address to the pixels
    int cols = (*arr)->dimSizes[1]; //amount of columns (image width)
    // Recompute Gamma if needed:
    if (gamma != Gamma) fnLUT(gamma); Gamma = gamma;  //Like ShiftReg in LV
    // Relative Weights
    double ffRow = floor(fRow);
    double ffCol = floor(fCol);
    double ffRowDiff = fRow - ffRow;
    double ffColDiff = fCol - ffCol;
    UINT relW0 = (UINT)rint((1 - ffRowDiff) * (1 - ffColDiff) * (WEIGHT_STEPS-1));
    UINT relW1 = (UINT)rint((ffRowDiff) * (1 - ffColDiff) * (WEIGHT_STEPS - 1));
    UINT relW2 = (UINT)rint((1 - ffRowDiff) * (ffColDiff) * (WEIGHT_STEPS - 1));
    UINT relW3 = (UINT)rint((ffRowDiff) * (ffColDiff) * (WEIGHT_STEPS - 1));    
    // Four Pixels:
    BYTE* p0 = getpXY(pix, cols, rint(ffRow), rint(ffCol));
    BYTE* p1 = getpXY(pix, cols, rint(ffRow + 1), rint(ffCol));
    BYTE* p2 = getpXY(pix, cols, rint(ffRow), rint(ffCol + 1));
    BYTE* p3 = getpXY(pix, cols, rint(ffRow + 1), rint(ffCol + 1));
    // Target color: 
    BYTE bgr[3] = { 0,0,0 };
    //#pragma omp parallel for num_threads(3)   //<-slow down around twice
    for (int i = 0; i < 3; i++) {
        double res = 0.0;
        INT64 sum =  WLUT[p0[i] * WEIGHT_STEPS + relW0] +
                     WLUT[p1[i] * WEIGHT_STEPS + relW1] +
                     WLUT[p2[i] * WEIGHT_STEPS + relW2] +
                     WLUT[p3[i] * WEIGHT_STEPS + relW3];
        UINT pos = binarySearch(LUT, sum); //Threshold 1D Array analog
        if (pos--) res = pos + (double)(sum - LUT[pos]) / (LUT[pos + 1] - LUT[pos]);
        bgr[i] = (int)rint(res); // fractional index
    } //RGB loop
    return (bgr[0] | (bgr[1] << 8) | bgr[2] << 16);
}

// same as above with Rows/Cols cycles
IP2D_API void fnIP2D2(iTDHdl arrSrc, iTDHdl arrDst, fTDHdl fCol, fTDHdl fRow, double gamma)
{
    int Ncols = (*fCol)->dimSizes[0];
    int Nrows = (*fRow)->dimSizes[0];
    double* fColPtr = (*fCol)->elt;
    double* fRowPtr = (*fRow)->elt;
    unsigned int* DstPtr = (*arrDst)->elt;

#pragma omp parallel for // num_threads(4)
    for (int y = 0; y < Nrows; y++) {
        for (int x = 0; x < Ncols; x++) {
            DstPtr[x + y * Ncols] = fnIP2D(arrSrc, fColPtr[x], fRowPtr[y], gamma);
        }
    }
}
