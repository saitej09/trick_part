/*******************************************************************************
*                                                                              *
* Trick Simulation Environment Software                                        *
*                                                                              *
* Copyright (c) 1996,1997 LinCom Corporation, Houston, TX                      *
* All rights reserved.                                                         *
*                                                                              *
* Copyrighted by LinCom Corporation and proprietary to it. Any unauthorized    *
* use of Trick Software including source code, object code or executables is   *
* strictly prohibited and LinCom assumes no liability for such actions or      *
* results thereof.                                                             *
*                                                                              *
* Trick Software has been developed under NASA Government Contracts and        *
* access to it may be granted for Government work by the following contact:    *
*                                                                              *
* Contact: Charles Gott, Branch Chief                                          *
*          Simulation and Graphics Branch                                      *
*          Automation, Robotics, & Simulation Division                         *
*          NASA, Johnson Space Center, Houston, TX                             *
*                                                                              *
*******************************************************************************/
/* 
   PURPOSE: (Computes all eigenvalues and eigenvectors)

   REFERENCE: ((Les Quiocho))

   ASSUMPTIONS AND LIMITATIONS: ((Generalized Eigenvalue problem) (Mass and stiffness matrices are symmetric and
   positive definite) (IMPORTANT: mass and stiffness matrices are destroyed upon entry))

   PROGRAMMERS: (((Les Quiocho) (NASA/JSC) (September 93) (Trick-CR-xxxxx) (--))) */

/* 
 * $Id: eigen_jacobi_4.c 49 2009-02-02 22:37:59Z lin $
 */

#include <stdio.h>
#include <stdlib.h>
#include "../include/trick_math.h"

void eigen_jacobi_4(double k[4][4],     /* In: input stiffness matrix */
                    double mass[4][4],  /* In: input mass matrix */
                    double v[4][4],     /* Out: output a set of eigen vectors */
                    double alpha[4],    /* Out: output a set of eigen values */
                    int m,      /* In: input dimension of matrix */
                    int mmax,   /* In: input maximun dimension of matrix */
                    int sort)
{                                      /* In: input sorting flag */
    int i, ip1, j, n, jmin, loop;

    double c11, c22, c, dis, eps;
    double x, sign, cgamm, calph;
    double p, q, r, s;
    double glamb, cd, cn, eig;
    double temp1, temp2;

    (void) mmax;                       /* unused */

    eps = 1.0e-12;

    /* Initialize alpha and eigenvectors */
    for (i = 0; i < m; i++)
        alpha[i] = 0.0;

    for (i = 0; i < m; i++)
        for (j = 0; j < m; j++)
            v[i][j] = 0.0;
    for (i = 0; i < m; i++)
        v[i][i] = 1.0;

    for (loop = 1; loop <= 100; loop++) {
        for (i = 0; i < m; i++) {
            for (j = 0; j < m; j++) {
                if (i != j) {
                    /* Zero out elements k[i][j] and mass[i][j] */
                    c11 = k[i][i] * mass[i][j] - k[i][j] * mass[i][i];
                    c22 = k[j][j] * mass[i][j] - k[i][j] * mass[j][j];
                    c = k[i][i] * mass[j][j] - k[j][j] * mass[i][i];
                    dis = (c * c / 4.0) + (c11 * c22);
                    if (dis < 0.0) {
                        fprintf(stdout, "\nNegative " "discriminant ... " "exiting\n");
                        exit(0);

                    }
                    sign = 1.0;
                    if (c < 0.0)
                        sign = -1.0;
                    x = (c / 2.0) + (sign * sqrt(dis));
                    cgamm = -c11 / x;
                    calph = c22 / x;

                    for (n = 0; n < m; n++) {
                        p = k[n][i];
                        q = k[n][j];
                        r = mass[n][i];
                        s = mass[n][j];
                        k[n][i] = p + q * cgamm;
                        k[n][j] = p * calph + q;
                        mass[n][i] = r + s * cgamm;
                        mass[n][j] = r * calph + s;
                    }
                    for (n = 0; n < m; n++) {
                        p = k[i][n];
                        q = k[j][n];
                        r = mass[i][n];
                        s = mass[j][n];
                        k[i][n] = p + q * cgamm;
                        k[j][n] = p * calph + q;
                        mass[i][n] = r + s * cgamm;
                        mass[j][n] = r * calph + s;
                    }
                    for (n = 0; n < m; n++) {
                        p = v[n][i];
                        q = v[n][j];
                        v[n][i] = p + q * cgamm;
                        v[n][j] = p * calph + q;

                    }
                }
            }
        }
        for (i = 0; i < m; i++) {
            glamb = k[i][i] / mass[i][i];
            if (fabs(glamb - alpha[i]) > fabs(glamb) * eps)
                goto label_190;
        }

        for (i = 0; i < m; i++) {
            for (j = 0; j < i; j++) {
                cn = fabs(k[i][j]);
                cd = sqrt(fabs(k[i][i] * k[j][j]));
                if (cn > cd * eps)
                    goto label_190;
                cn = fabs(mass[i][j]);
                cd = sqrt(fabs(mass[i][i] * mass[j][j]));
                if (cn > cd * eps)
                    goto label_190;
            }
        }
        goto label_210;
      label_190:
        for (j = 0; j < m; j++) {
            if (!((k[j][j] > 0.0) && (mass[j][j] > 0.0))) {
                fprintf(stdout, "\nTransformed matrix not positive " "definite, exiting ...\n");
                exit(0);
            }
            alpha[j] = k[j][j] / mass[j][j];
        }
    }


    fprintf(stdout, "Jacobi iteration failed , exiting ...\n");
    exit(0);


  label_210:

    /* Sort eigenvalues into ascending order */
    if (sort) {
        for (i = 0; i < m; i++) {
            ip1 = i + 1;
            eig = alpha[i];
            jmin = i;
            for (j = ip1; j < m; j++) {
                if (alpha[j] < eig) {
                    jmin = j;
                    eig = alpha[j];
                }
            }
            if (jmin != i) {
                alpha[jmin] = alpha[i];
                alpha[i] = eig;
                for (j = 0; j < m; j++) {
                    temp1 = v[j][jmin];
                    temp2 = v[j][i];
                    v[j][i] = temp1;
                    v[j][jmin] = temp2;
                }
            }
        }
    }
}
