// Copyright (c) 2017, Lawrence Livermore National Security, LLC. Produced at
// the Lawrence Livermore National Laboratory. LLNL-CODE-734707. All Rights
// reserved. See files LICENSE and NOTICE for details.
//
// This file is part of CEED, a collection of benchmarks, miniapps, software
// libraries and APIs for efficient high-order finite element and spectral
// element discretizations for exascale applications. For more information and
// source code availability see http://github.com/ceed.
//
// The CEED research is supported by the Exascale Computing Project 17-SC-20-SC,
// a collaborative effort of two U.S. Department of Energy organizations (Office
// of Science and the National Nuclear Security Administration) responsible for
// the planning and preparation of a capable exascale ecosystem, including
// software, applications, hardware, advanced system engineering and early
// testbed platforms, in support of the nation's exascale computing imperative.
#ifndef MFEM_GENERAL_KERNELS_ARRAY
#define MFEM_GENERAL_KERNELS_ARRAY

MFEM_NAMESPACE


// *****************************************************************************
void kArrayInitVal(const size_t, const size_t, void*, size_t, void*);

// *****************************************************************************
template<typename T>
void kArraySetK(T *data, const size_t k, T value){
   GET_CUDA;
   GET_ADRS_T(data,T);
   if (cuda){ assert(false); }
   d_data[k] = value;
}

// *****************************************************************************
template<typename T>
T kArrayGetK(const T *data, const size_t k){
   GET_CUDA;
   GET_CONST_ADRS_T(data,T);
   if (cuda){ assert(false); }
   return d_data[k];
}

// *****************************************************************************
double* kArrayInitGet(const int, double**);
void* kArrayVoidGet(const int, void**);

void kArrayInitSet(double**, double*);

MFEM_NAMESPACE_END

#endif // MFEM_GENERAL_KERNELS_ARRAY