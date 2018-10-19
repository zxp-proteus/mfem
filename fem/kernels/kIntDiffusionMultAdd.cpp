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

#include "../../general/okina.hpp"

// *****************************************************************************
#define QUAD_2D_ID(X, Y) (X + ((Y) * NUM_QUAD_1D))
#define QUAD_3D_ID(X, Y, Z) (X + ((Y) * NUM_QUAD_1D) + ((Z) * NUM_QUAD_1D*NUM_QUAD_1D))

// *****************************************************************************
template<const int NUM_DOFS_1D,
         const int NUM_QUAD_1D> static
void kIntDiffusionMultAdd2D(const int numElements,
                            const double* __restrict dofToQuad,
                            const double* __restrict dofToQuadD,
                            const double* __restrict quadToDof,
                            const double* __restrict quadToDofD,
                            const double* __restrict oper,
                            const double* __restrict solIn,
                            double* __restrict solOut)
{
   const int NUM_QUAD = NUM_QUAD_1D*NUM_QUAD_1D;
   forall(e, numElements,
   {
      double grad[NUM_QUAD_1D][NUM_QUAD_1D][2];
      for (int qy = 0; qy < NUM_QUAD_1D; ++qy)
      {
         for (int qx = 0; qx < NUM_QUAD_1D; ++qx)
         {
            grad[qy][qx][0] = 0.0;
            grad[qy][qx][1] = 0.0;
         }
      }

      for (int dy = 0; dy < NUM_DOFS_1D; ++dy)
      {
         double gradX[NUM_QUAD_1D][2];
         for (int qx = 0; qx < NUM_QUAD_1D; ++qx)
         {
            gradX[qx][0] = 0.0;
            gradX[qx][1] = 0.0;
         }

         for (int dx = 0; dx < NUM_DOFS_1D; ++dx)
         {
            const double s = solIn[ijkN(dx,dy,e,NUM_DOFS_1D)];
            for (int qx = 0; qx < NUM_QUAD_1D; ++qx)
            {
               gradX[qx][0] += s * dofToQuad[ijN(qx,dx,NUM_QUAD_1D)];
               gradX[qx][1] += s * dofToQuadD[ijN(qx,dx,NUM_QUAD_1D)];
            }
         }

         for (int qy = 0; qy < NUM_QUAD_1D; ++qy)
         {
            const double wy  = dofToQuad[ijN(qy,dy,NUM_QUAD_1D)];
            const double wDy = dofToQuadD[ijN(qy,dy,NUM_QUAD_1D)];
            for (int qx = 0; qx < NUM_QUAD_1D; ++qx)
            {
               grad[qy][qx][0] += gradX[qx][1] * wy;
               grad[qy][qx][1] += gradX[qx][0] * wDy;
            }
         }
      }

      // Calculate Dxy, xDy in plane
      for (int qy = 0; qy < NUM_QUAD_1D; ++qy)
      {
         for (int qx = 0; qx < NUM_QUAD_1D; ++qx)
         {
            const int q = QUAD_2D_ID(qx, qy);
            
            const double O11 = oper[ijkNM(0,q,e,3,NUM_QUAD)];
            const double O12 = oper[ijkNM(1,q,e,3,NUM_QUAD)];
            const double O22 = oper[ijkNM(2,q,e,3,NUM_QUAD)];

            const double gradX = grad[qy][qx][0];
            const double gradY = grad[qy][qx][1];

            grad[qy][qx][0] = (O11 * gradX) + (O12 * gradY);
            grad[qy][qx][1] = (O12 * gradX) + (O22 * gradY);
         }
      }

      for (int qy = 0; qy < NUM_QUAD_1D; ++qy)
      {
         double gradX[NUM_DOFS_1D][2];
         for (int dx = 0; dx < NUM_DOFS_1D; ++dx)
         {
            gradX[dx][0] = 0;
            gradX[dx][1] = 0;
         }

         for (int qx = 0; qx < NUM_QUAD_1D; ++qx)
         {
            const double gX = grad[qy][qx][0];
            const double gY = grad[qy][qx][1];
            for (int dx = 0; dx < NUM_DOFS_1D; ++dx)
            {
               const double wx  = quadToDof[ijN(dx,qx,NUM_DOFS_1D)];
               const double wDx = quadToDofD[ijN(dx,qx,NUM_DOFS_1D)];
               gradX[dx][0] += gX * wDx;
               gradX[dx][1] += gY * wx;
            }
         }

         for (int dy = 0; dy < NUM_DOFS_1D; ++dy)
         {
            const double wy  = quadToDof[ijN(dy,qy,NUM_DOFS_1D)];
            const double wDy = quadToDofD[ijN(dy,qy,NUM_DOFS_1D)];
            for (int dx = 0; dx < NUM_DOFS_1D; ++dx)
            {
               solOut[ijkN(dx,dy,e,NUM_DOFS_1D)] += ((gradX[dx][0] * wy) +
                                                     (gradX[dx][1] * wDy));
            }
         }
      }
   });
}

// *****************************************************************************
template<const int NUM_DOFS_1D,
         const int NUM_QUAD_1D> static
void kIntDiffusionMultAdd3D(const int numElements,
                            const double* __restrict dofToQuad,
                            const double* __restrict dofToQuadD,
                            const double* __restrict quadToDof,
                            const double* __restrict quadToDofD,
                            const double* __restrict oper,
                            const double* __restrict solIn,
                            double* __restrict solOut)
{
   const int NUM_QUAD = NUM_QUAD_1D*NUM_QUAD_1D*NUM_QUAD_1D;
   forall(e, numElements,
   {
      double grad[NUM_QUAD_1D][NUM_QUAD_1D][NUM_QUAD_1D][4];
      for (int qz = 0; qz < NUM_QUAD_1D; ++qz) {
         for (int qy = 0; qy < NUM_QUAD_1D; ++qy) {
            for (int qx = 0; qx < NUM_QUAD_1D; ++qx) {
               grad[qz][qy][qx][0] = 0.0;
               grad[qz][qy][qx][1] = 0.0;
               grad[qz][qy][qx][2] = 0.0;
            }
         }
      }
      for (int dz = 0; dz < NUM_DOFS_1D; ++dz) {
         double gradXY[NUM_QUAD_1D][NUM_QUAD_1D][4];
         for (int qy = 0; qy < NUM_QUAD_1D; ++qy) {
            for (int qx = 0; qx < NUM_QUAD_1D; ++qx) {
               gradXY[qy][qx][0] = 0.0;
               gradXY[qy][qx][1] = 0.0;
               gradXY[qy][qx][2] = 0.0;
            }
         }
         for (int dy = 0; dy < NUM_DOFS_1D; ++dy) {
            double gradX[NUM_QUAD_1D][2];
            for (int qx = 0; qx < NUM_QUAD_1D; ++qx) {
               gradX[qx][0] = 0.0;
               gradX[qx][1] = 0.0;
            }
            for (int dx = 0; dx < NUM_DOFS_1D; ++dx) {
               const double s = solIn[ijklN(dx,dy,dz,e,NUM_DOFS_1D)];
               for (int qx = 0; qx < NUM_QUAD_1D; ++qx) {
                  gradX[qx][0] += s * dofToQuad[ijN(qx,dx,NUM_QUAD_1D)];
                  gradX[qx][1] += s * dofToQuadD[ijN(qx,dx,NUM_QUAD_1D)];
               }
            }
            for (int qy = 0; qy < NUM_QUAD_1D; ++qy) {
               const double wy  = dofToQuad[ijN(qy,dy,NUM_QUAD_1D)];
               const double wDy = dofToQuadD[ijN(qy,dy,NUM_QUAD_1D)];
               for (int qx = 0; qx < NUM_QUAD_1D; ++qx) {
                  const double wx  = gradX[qx][0];
                  const double wDx = gradX[qx][1];
                  gradXY[qy][qx][0] += wDx * wy;
                  gradXY[qy][qx][1] += wx  * wDy;
                  gradXY[qy][qx][2] += wx  * wy;
               }
            }
         }
         for (int qz = 0; qz < NUM_QUAD_1D; ++qz) {
            const double wz  = dofToQuad[ijN(qz,dz,NUM_QUAD_1D)];
            const double wDz = dofToQuadD[ijN(qz,dz,NUM_QUAD_1D)];
            for (int qy = 0; qy < NUM_QUAD_1D; ++qy) {
               for (int qx = 0; qx < NUM_QUAD_1D; ++qx) {
                  grad[qz][qy][qx][0] += gradXY[qy][qx][0] * wz;
                  grad[qz][qy][qx][1] += gradXY[qy][qx][1] * wz;
                  grad[qz][qy][qx][2] += gradXY[qy][qx][2] * wDz;
               }
            }
         }
      }

      // Calculate Dxyz, xDyz, xyDz in plane
      for (int qz = 0; qz < NUM_QUAD_1D; ++qz) {
         for (int qy = 0; qy < NUM_QUAD_1D; ++qy) {
            for (int qx = 0; qx < NUM_QUAD_1D; ++qx) {
               const int q = QUAD_3D_ID(qx, qy, qz);
               const double O11 = oper[ijkNM(0,q,e,6,NUM_QUAD)];
               const double O12 = oper[ijkNM(1,q,e,6,NUM_QUAD)];
               const double O13 = oper[ijkNM(2,q,e,6,NUM_QUAD)];
               const double O22 = oper[ijkNM(3,q,e,6,NUM_QUAD)];
               const double O23 = oper[ijkNM(4,q,e,6,NUM_QUAD)];
               const double O33 = oper[ijkNM(5,q,e,6,NUM_QUAD)];

               const double gradX = grad[qz][qy][qx][0];
               const double gradY = grad[qz][qy][qx][1];
               const double gradZ = grad[qz][qy][qx][2];

               grad[qz][qy][qx][0] = (O11 * gradX) + (O12 * gradY) + (O13 * gradZ);
               grad[qz][qy][qx][1] = (O12 * gradX) + (O22 * gradY) + (O23 * gradZ);
               grad[qz][qy][qx][2] = (O13 * gradX) + (O23 * gradY) + (O33 * gradZ);
            }
         }
      }

      for (int qz = 0; qz < NUM_QUAD_1D; ++qz) {
         double gradXY[NUM_DOFS_1D][NUM_DOFS_1D][4];
         for (int dy = 0; dy < NUM_DOFS_1D; ++dy) {
            for (int dx = 0; dx < NUM_DOFS_1D; ++dx) {
               gradXY[dy][dx][0] = 0;
               gradXY[dy][dx][1] = 0;
               gradXY[dy][dx][2] = 0;
            }
         }

         for (int qy = 0; qy < NUM_QUAD_1D; ++qy) {
            double gradX[NUM_DOFS_1D][4];
            for (int dx = 0; dx < NUM_DOFS_1D; ++dx) {
               gradX[dx][0] = 0;
               gradX[dx][1] = 0;
               gradX[dx][2] = 0;
            }

            for (int qx = 0; qx < NUM_QUAD_1D; ++qx) {
               const double gX = grad[qz][qy][qx][0];
               const double gY = grad[qz][qy][qx][1];
               const double gZ = grad[qz][qy][qx][2];
               for (int dx = 0; dx < NUM_DOFS_1D; ++dx) {
                  const double wx  = quadToDof[ijN(dx,qx,NUM_QUAD_1D)];
                  const double wDx = quadToDofD[ijN(dx,qx,NUM_QUAD_1D)];
                  gradX[dx][0] += gX * wDx;
                  gradX[dx][1] += gY * wx;
                  gradX[dx][2] += gZ * wx;
               }
            }

            for (int dy = 0; dy < NUM_DOFS_1D; ++dy) {
               const double wy  = quadToDof[ijN(dy,qy,NUM_DOFS_1D)];
               const double wDy = quadToDofD[ijN(dy,qy,NUM_DOFS_1D)];
               for (int dx = 0; dx < NUM_DOFS_1D; ++dx) {
                  gradXY[dy][dx][0] += gradX[dx][0] * wy;
                  gradXY[dy][dx][1] += gradX[dx][1] * wDy;
                  gradXY[dy][dx][2] += gradX[dx][2] * wy;
               }
            }
         }

         for (int dz = 0; dz < NUM_DOFS_1D; ++dz) {
            const double wz  = quadToDof[ijN(dz,qz,NUM_DOFS_1D)];
            const double wDz = quadToDofD[ijN(dz,qz,NUM_DOFS_1D)];
            for (int dy = 0; dy < NUM_DOFS_1D; ++dy) {
               for (int dx = 0; dx < NUM_DOFS_1D; ++dx) {
                  solOut[ijklN(dx,dy,dz,e,NUM_DOFS_1D)] +=
                     ((gradXY[dy][dx][0] * wz) +
                      (gradXY[dy][dx][1] * wz) +
                      (gradXY[dy][dx][2] * wDz));
               }
            }
         }
      }
   });
}

// *****************************************************************************
typedef void (*fDiffusionMultAdd)(const int numElements,
                                  const double* __restrict dofToQuad,
                                  const double* __restrict dofToQuadD,
                                  const double* __restrict quadToDof,
                                  const double* __restrict quadToDofD,
                                  const double* __restrict oper,
                                  const double* __restrict solIn,
                                  double* __restrict solOut);

// *****************************************************************************
void kIntDiffusionMultAdd(const int DIM,
                          const int NUM_DOFS_1D,
                          const int NUM_QUAD_1D,
                          const int numElements,
                          const double* __restrict dofToQuad,
                          const double* __restrict dofToQuadD,
                          const double* __restrict quadToDof,
                          const double* __restrict quadToDofD,
                          const double* __restrict op,
                          const double* __restrict x,
                          double* __restrict y)
{
   const unsigned int id = (DIM<<16)|(NUM_DOFS_1D<<8)|(NUM_QUAD_1D);
   dbg("NUM_DOFS_1D=%d",NUM_DOFS_1D);
   dbg("NUM_QUAD_1D=%d",NUM_QUAD_1D);
   dbg("id=0x%x",id);
   static std::unordered_map<unsigned int, fDiffusionMultAdd> call =
   {      
      {0x20202,&kIntDiffusionMultAdd2D<2,2>},
      {0x20303,&kIntDiffusionMultAdd2D<3,3>},
      {0x20404,&kIntDiffusionMultAdd2D<4,4>},
      {0x20505,&kIntDiffusionMultAdd2D<5,5>},
      {0x20606,&kIntDiffusionMultAdd2D<6,6>},
      {0x20707,&kIntDiffusionMultAdd2D<7,7>},
      {0x20808,&kIntDiffusionMultAdd2D<8,8>},
      {0x20909,&kIntDiffusionMultAdd2D<9,9>},
      {0x20A0A,&kIntDiffusionMultAdd2D<10,10>},
      {0x20B0B,&kIntDiffusionMultAdd2D<11,11>},
      {0x20C0C,&kIntDiffusionMultAdd2D<12,12>},
      {0x20D0D,&kIntDiffusionMultAdd2D<13,13>},
      {0x20E0E,&kIntDiffusionMultAdd2D<14,14>},
      {0x20F0F,&kIntDiffusionMultAdd2D<15,15>},
      {0x21010,&kIntDiffusionMultAdd2D<16,16>},
      {0x21111,&kIntDiffusionMultAdd2D<17,17>},
      
      {0x30203,&kIntDiffusionMultAdd3D<2,3>},/*
      {0x30304,&kIntDiffusionMultAdd3D<3,4>},
      {0x30405,&kIntDiffusionMultAdd3D<4,5>},
      {0x30506,&kIntDiffusionMultAdd3D<5,6>},
      {0x30607,&kIntDiffusionMultAdd3D<6,7>},
      {0x30708,&kIntDiffusionMultAdd3D<7,8>},
      {0x30809,&kIntDiffusionMultAdd3D<8,9>},
      {0x3090A,&kIntDiffusionMultAdd3D<9,10>},
      {0x30A0B,&kIntDiffusionMultAdd3D<10,11>},
      {0x30B0C,&kIntDiffusionMultAdd3D<11,12>},
      {0x30C0D,&kIntDiffusionMultAdd3D<12,13>},
      {0x30D0E,&kIntDiffusionMultAdd3D<13,14>},
      {0x30E0F,&kIntDiffusionMultAdd3D<14,15>},
      {0x30F10,&kIntDiffusionMultAdd3D<15,16>},*/
   };
   if (!call[id])
   {
      printf("\n[kIntDiffusionMultAdd] id \033[33m0x%X\033[m ",id);
      fflush(stdout);
   }
   assert(call[id]);

   GET_CONST_ADRS(dofToQuad);
   GET_CONST_ADRS(dofToQuadD);
   GET_CONST_ADRS(quadToDof);
   GET_CONST_ADRS(quadToDofD);
   GET_CONST_ADRS(op);
   GET_CONST_ADRS(x);
   GET_ADRS(y);

   call[id](numElements, d_dofToQuad, d_dofToQuadD, d_quadToDof, d_quadToDofD,
            d_op, d_x, d_y);
}