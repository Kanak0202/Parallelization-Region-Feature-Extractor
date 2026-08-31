// Smoothed Particle Hydrodynamics (SPH)
// Serial benchmark version
// No input files and no output files
// N controls the number of particles

#include <stdio.h>
#include <math.h>
#include <stdlib.h>

/* ============================================================
   Problem Size / Simulation Parameters
   ============================================================ */

#define N       100
#define NB      (N/2)

#define DY      1.0
#define RHO0    1.1547
#define MASSP   1.00
#define MASSB   1.00
#define VRING   0.10
#define SIGMA   0.80
#define EPS     50.0
#define RADIUS  3
#define MAX     200.0
#define DT      0.01
#define H       3.0
#define M       2.0

#define EMOD    18.470
#define G       6.928
#define B       13.856
#define YIELD   1.360
#define ULTI    4.270

#define alpha   0.50
#define beta    0.50

#define ART_VISCOSITY 1
#define MONA_CORR     1
#define MONA_CORR2    2
#define JAUMANN       1
#define TENSILE       1
#define GRADCORR      1

#define ri      15.00
#define ro      20.00


/* ============================================================
   Global Arrays
   ============================================================ */

double X[N], Y[N], VX[N], VY[N], RHO[N];

double E[N], SXX[N], SXY[N], SYY[N];

double XO[N], YO[N], VXO[N], VYO[N], RHOO[N];
double EO[N], SXXO[N], SXYO[N], SYYO[N];

double XN[N], YN[N], VXN[N], VYN[N], RHON[N];
double EN[N], SXXN[N], SXYN[N], SYYN[N];

double P[N], KE[N], PE[N], MASS[N];

double XDOT[N], YDOT[N];
double VXDOT[N], VYDOT[N];
double RHODOT[N], EDOT[N];
double SXXDOT[N], SXYDOT[N], SYYDOT[N];

double DVXDX[N], DVXDY[N];
double DVYDX[N], DVYDY[N];

double FX[N], FY[N];
double VXBAR[N], VYBAR[N];
double DeltaWP[N];

double EINT, WP;

double DAMAGE[N];

double wfgrad[N][5];


/* ============================================================
   Function Prototypes
   ============================================================ */

void initialize();
void derivatives();

void kernel(double wf[], double dist);

void Update(double dt);

void plasticity();

void viscosity(
    double visc[],
    int i,
    int j,
    double xij,
    double yij,
    double vxij,
    double vyij,
    double dist,
    double wf[],
    double wf_grad,
    double wfdx,
    double wfdy
);

void artificial_pressure(
    double Pi,
    double Pj,
    double rho2i,
    double rho2j,
    double wf[],
    double xij,
    double yij,
    double dist,
    int i,
    int j,
    double vxij,
    double vyij,
    double wf_grad,
    double wfdx,
    double wfdy
);

void monacorr(
    int i,
    int j,
    double wf[],
    double vxij,
    double vyij,
    double wf_grad,
    double wfdx,
    double wfdy
);

void basicsph(
    int i,
    int j,
    double wf[],
    double xij,
    double yij,
    double vxij,
    double vyij,
    double rho2i,
    double rho2j,
    double dist,
    double sigxxi,
    double sigyyi,
    double sigxyi,
    double sigxxj,
    double sigyyj,
    double sigxyj,
    double drhobar,
    double wf_grad,
    double wfdx,
    double wfdy
);


/* ============================================================
   Main
   ============================================================ */

int main()
{
    double t = 0.0;
    int i;
    int loopcounter = 0;

    /* Initialization */
    initialize();

    /* Main simulation loop */
    while(t < MAX)
    {
        /* ----------------------------------------------------
           Save old state
           ---------------------------------------------------- */

        #pragma capc profitability_region begin
        #pragma acc parallel loop copyin(X[0:N],Y[0:N],VX[0:N],VY[0:N],RHO[0:N],E[0:N],SXX[0:N],SXY[0:N],SYY[0:N]) copyout(XO[0:N],YO[0:N],VXO[0:N],VYO[0:N],RHOO[0:N],EO[0:N],SXXO[0:N],SXYO[0:N],SYYO[0:N])
        for(i = 0; i < N; i++)
        {
            XO[i]   = X[i];
            YO[i]   = Y[i];

            VXO[i]  = VX[i];
            VYO[i]  = VY[i];

            RHOO[i] = RHO[i];
            EO[i]   = E[i];

            SXXO[i] = SXX[i];
            SXYO[i] = SXY[i];
            SYYO[i] = SYY[i];
        }
        #pragma capc profitability_region end


        /* ----------------------------------------------------
           First derivative computation
           ---------------------------------------------------- */

        derivatives();


        /* ----------------------------------------------------
           First update
           ---------------------------------------------------- */

        Update(DT);


        /* ----------------------------------------------------
           Copy updated state
           ---------------------------------------------------- */

        #pragma capc profitability_region begin
        #pragma acc parallel loop copyin(XN[0:N],YN[0:N],VXN[0:N],VYN[0:N],RHON[0:N],EN[0:N],SXXN[0:N],SXYN[0:N],SYYN[0:N]) copyout(X[0:N],Y[0:N],VX[0:N],VY[0:N],RHO[0:N],E[0:N],SXX[0:N],SXY[0:N],SYY[0:N])
        for(i = 0; i < N; i++)
        {
            X[i]   = XN[i];
            Y[i]   = YN[i];

            VX[i]  = VXN[i];
            VY[i]  = VYN[i];

            RHO[i] = RHON[i];
            E[i]   = EN[i];

            SXX[i] = SXXN[i];
            SXY[i] = SXYN[i];
            SYY[i] = SYYN[i];
        }
        #pragma capc profitability_region end


        /* ----------------------------------------------------
           Second derivative computation
           ---------------------------------------------------- */

        derivatives();


        /* ----------------------------------------------------
           Restore old state
           ---------------------------------------------------- */

        #pragma capc profitability_region begin
        #pragma acc parallel loop copyin(XO[0:N],YO[0:N],VXO[0:N],VYO[0:N],RHOO[0:N],EO[0:N],SXXO[0:N],SXYO[0:N],SYYO[0:N]) copyout(X[0:N],Y[0:N],VX[0:N],VY[0:N],RHO[0:N],E[0:N],SXX[0:N],SXY[0:N],SYY[0:N])
        for(i = 0; i < N; i++)
        {
            X[i]   = XO[i];
            Y[i]   = YO[i];

            VX[i]  = VXO[i];
            VY[i]  = VYO[i];

            RHO[i] = RHOO[i];
            E[i]   = EO[i];

            SXX[i] = SXXO[i];
            SXY[i] = SXYO[i];
            SYY[i] = SYYO[i];
        }
        #pragma capc profitability_region end


        /* ----------------------------------------------------
           Second update
           ---------------------------------------------------- */

        Update(DT);


        /* ----------------------------------------------------
           Predictor-Corrector update
           ---------------------------------------------------- */

        #pragma capc profitability_region begin
        #pragma acc parallel loop copyin(XN[0:N],YN[0:N],VXN[0:N],VYN[0:N],SXXN[0:N],SXYN[0:N],SYYN[0:N],RHON[0:N],EN[0:N],XO[0:N],YO[0:N],VXO[0:N],VYO[0:N],SXXO[0:N],SXYO[0:N],SYYO[0:N],RHOO[0:N],EO[0:N]) copyout(X[0:N],Y[0:N],VX[0:N],VY[0:N],SXX[0:N],SXY[0:N],SYY[0:N],RHO[0:N],E[0:N])
        for(i = 0; i < N; i++)
        {
            X[i]   = 2.0 * XN[i] - XO[i];
            Y[i]   = 2.0 * YN[i] - YO[i];

            VX[i]  = 2.0 * VXN[i] - VXO[i];
            VY[i]  = 2.0 * VYN[i] - VYO[i];

            SXX[i] = 2.0 * SXXN[i] - SXXO[i];
            SXY[i] = 2.0 * SXYN[i] - SXYO[i];
            SYY[i] = 2.0 * SYYN[i] - SYYO[i];

            RHO[i] = 2.0 * RHON[i] - RHOO[i];
            E[i]   = 2.0 * EN[i] - EO[i];
        }
        #pragma capc profitability_region end


        t += DT;
        loopcounter++;
    }


    /* --------------------------------------------------------
       Print only a few values.
       This prevents complete elimination of the computation.
       -------------------------------------------------------- */

    printf("X[0] = %lf\n", X[0]);
    printf("Y[0] = %lf\n", Y[0]);
    printf("VX[0] = %lf\n", VX[0]);
    printf("VY[0] = %lf\n", VY[0]);

    printf("X[%d] = %lf\n", N-1, X[N-1]);
    printf("Y[%d] = %lf\n", N-1, Y[N-1]);

    printf("Simulation steps = %d\n", loopcounter);

    return 0;
}


/* ============================================================
   DERIVATIVES
   ============================================================ */

void derivatives()
{
    double xij, yij, vxij, vyij;
    double dist;
    double hsq = H * H;

    double rhobar, drhobar;

    double rho0rho;

    double wf[2];

    double visc[1];

    double rxydot[N];

    double lambda = B - G;
    double eta = G;
    double trace;

    double sigxxi, sigxyi, sigyyi;
    double sigxxj, sigxyj, sigyyj;

    double rho2i, rho2j;

    double term1, term12, term2;

    double tensterm = 0.0;
    double tensgamma = 0.3;

    double distinit, xinitij, yinitij;
    double wfinit[2];

    double mata, matb, matc, matd;
    double wfdx, wfdy, wf_grad;

    int i, j;


    /* --------------------------------------------------------
       Initialization of derivative arrays
       -------------------------------------------------------- */

    for(i = 0; i < N; i++)
    {
        RHODOT[i] = 0.0;

        VXDOT[i] = 0.0;
        VYDOT[i] = 0.0;

        DVXDX[i] = 0.0;
        DVXDY[i] = 0.0;
        DVYDX[i] = 0.0;
        DVYDY[i] = 0.0;

        VXBAR[i] = 0.0;
        VYBAR[i] = 0.0;

        FX[i] = 0.0;
        FY[i] = 0.0;

        EDOT[i] = 0.0;

        rho0rho = RHO0 / RHO[i];

        P[i] = -6.0 * M * RHO0 *
               (pow(2.0 - rho0rho, 2*M - 1)
               - pow(2.0 - rho0rho, M - 1));

        rxydot[i] = 0.0;

        EINT = 0.0;
    }


    /* --------------------------------------------------------
       Kernel computation for gradient correction
       -------------------------------------------------------- */

    #pragma capc profitability_region begin
    #pragma acc parallel loop collapse(2) copyout(wfgrad[0:N][0:5])
    for(i = 0; i < N; i++)
    {
        for(j = 0; j < 5; j++)
        {
            wfgrad[i][j] = 0.0;
        }
    }
    #pragma capc profitability_region end


    if(GRADCORR == 0)
    {
        #pragma capc profitability_region begin
        #pragma acc parallel loop copyout(wfgrad[0:N][0:5])
        for(i = 0; i < N; i++)
        {
            wfgrad[i][0] = 1.0;
            wfgrad[i][1] = 1.0;
            wfgrad[i][2] = 0.0;
            wfgrad[i][3] = 0.0;
            wfgrad[i][4] = 1.0;
        }
        #pragma capc profitability_region end
    }


    if(GRADCORR == 1)
    {
        #pragma capc profitability_region begin
        for(i = 0; i < N-NB; i++)
        {
            for(j = 0; j < N-NB; j++)
            {
                if(i != j)
                {
                    xij = X[i] - X[j];
                    yij = Y[i] - Y[j];

                    dist = xij*xij + yij*yij;

                    if(dist <= hsq)
                    {
                        dist = sqrt(dist);

                        kernel(wf, dist);

                        wfgrad[i][0] +=
                            wf[0] * MASS[j] / RHO[j];

                        wfgrad[i][1] +=
                            -1.0 * xij * wf[1] *
                            xij / dist * MASS[j] / RHO[j];

                        wfgrad[i][2] +=
                            -1.0 * yij * wf[1] *
                            xij / dist * MASS[j] / RHO[j];

                        wfgrad[i][3] +=
                            -1.0 * xij * wf[1] *
                            yij / dist * MASS[j] / RHO[j];

                        wfgrad[i][4] +=
                            -1.0 * yij * wf[1] *
                            yij / dist * MASS[j] / RHO[j];
                    }
                }
            }
        }
        #pragma capc profitability_region end
    }


    if(GRADCORR == 1)
    {
        #pragma capc profitability_region begin
        for(i = N-NB; i < N; i++)
        {
            for(j = N-NB; j < N; j++)
            {
                if(i != j)
                {
                    xij = X[i] - X[j];
                    yij = Y[i] - Y[j];

                    dist = xij*xij + yij*yij;

                    if(dist <= hsq)
                    {
                        dist = sqrt(dist);

                        kernel(wf, dist);

                        wfgrad[i][0] +=
                            wf[0] * MASS[j] / RHO[j];

                        wfgrad[i][1] +=
                            -1.0 * xij * wf[1] *
                            xij / dist * MASS[j] / RHO[j];

                        wfgrad[i][2] +=
                            -1.0 * yij * wf[1] *
                            xij / dist * MASS[j] / RHO[j];

                        wfgrad[i][3] +=
                            -1.0 * xij * wf[1] *
                            yij / dist * MASS[j] / RHO[j];

                        wfgrad[i][4] +=
                            -1.0 * yij * wf[1] *
                            yij / dist * MASS[j] / RHO[j];
                    }
                }
            }
        }
        #pragma capc profitability_region end
    }


    /* --------------------------------------------------------
       Actual SPH computation
       -------------------------------------------------------- */

    for(i = 0; i < N; i++)
    {
        sigxxi = SXX[i] + P[i];
        sigyyi = SYY[i] + P[i];
        sigxyi = SXY[i];

        rho2i = 1.0 / (RHO[i] * RHO[i]);


        for(j = 0; j < N-NB; j++)
        {
            if(i != j)
            {
                /* --------------------------------------------
                   Ball-Plate Interaction
                   -------------------------------------------- */

                if(i >= N-NB)
                {
                    xij = X[i] - X[j];
                    yij = Y[i] - Y[j];

                    dist = xij*xij + yij*yij;
                    dist = sqrt(dist);

                    term1 = (dist - RADIUS) / SIGMA;

                    if(term1 < 1.0)
                    {
                        term12 = term1 * term1;

                        term2 =
                            8.0 * EPS / SIGMA *
                            term1 *
                            (1-term12) *
                            (1-term12) *
                            (1-term12);

                        VXDOT[i] +=
                            term2 * xij / dist / MASS[i];

                        VYDOT[i] +=
                            term2 * yij / dist / MASS[i];

                        VXDOT[j] -=
                            term2 * xij / dist / MASS[j];

                        VYDOT[j] -=
                            term2 * yij / dist / MASS[j];

                        EINT +=
                            EPS *
                            (1.0-term12) *
                            (1.0-term12) *
                            (1.0-term12) *
                            (1.0-term12);
                    }
                }


                /* --------------------------------------------
                   Plate-Plate Interaction
                   -------------------------------------------- */

                if(i < N-NB)
                {
                    xij = X[i] - X[j];
                    yij = Y[i] - Y[j];

                    vxij = VX[i] - VX[j];
                    vyij = VY[i] - VY[j];

                    dist = xij*xij + yij*yij;

                    if(dist <= hsq)
                    {
                        dist = sqrt(dist);

                        mata = wfgrad[i][1];
                        matb = wfgrad[i][2];
                        matc = wfgrad[i][3];
                        matd = wfgrad[i][4];

                        kernel(wf, dist);

                        wf_grad =
                            wf[0] * MASS[j] /
                            RHO[j] / wfgrad[i][0];

                        wfdx =
                            1.0 /
                            (mata*matd - matb*matc) *
                            (wfgrad[i][4] * wf[1] * xij / dist
                            - wfgrad[i][2] * wf[1] * yij / dist);

                        wfdy =
                            1.0 /
                            (mata*matd - matb*matc) *
                            (wfgrad[i][1] * wf[1] * yij / dist
                            - wfgrad[i][3] * wf[1] * xij / dist);


                        rhobar = RHO[j];

                        drhobar = dist * rhobar;

                        RHODOT[i] +=
                            MASS[j] *
                            (vxij*wfdx + vyij*wfdy);


                        sigxxj = SXX[j] + P[j];
                        sigyyj = SYY[j] + P[j];
                        sigxyj = SXY[j];

                        rho2j =
                            1.0 / (RHO[j] * RHO[j]);


                        if(MONA_CORR == 1)
                            monacorr(
                                i, j, wf,
                                vxij, vyij,
                                wf_grad,
                                wfdx, wfdy
                            );


                        if(JAUMANN == 1)
                        {
                            rxydot[i] +=
                                -0.5 *
                                MASS[j] / RHO[j] *
                                (vxij*wfdy - vyij*wfdx);
                        }


                        basicsph(
                            i, j,
                            wf,
                            xij, yij,
                            vxij, vyij,
                            rho2i, rho2j,
                            dist,
                            sigxxi, sigyyi, sigxyi,
                            sigxxj, sigyyj, sigxyj,
                            drhobar,
                            wf_grad,
                            wfdx, wfdy
                        );


                        if(ART_VISCOSITY == 1)
                        {
                            viscosity(
                                visc,
                                i, j,
                                xij, yij,
                                vxij, vyij,
                                dist,
                                wf,
                                wf_grad,
                                wfdx, wfdy
                            );
                        }


                        if(TENSILE == 1)
                        {
                            artificial_pressure(
                                P[i], P[j],
                                rho2i, rho2j,
                                wf,
                                xij, yij,
                                dist,
                                i, j,
                                vxij, vyij,
                                wf_grad,
                                wfdx, wfdy
                            );
                        }
                    }
                }
            }
        }


        trace =
            -1.0/3.0 *
            (DVXDX[i] + DVYDY[i]);

        SXXDOT[i] =
            2.0*G *
            (DVXDX[i] + trace) +
            2.0*SXY[i]*rxydot[i];

        SYYDOT[i] =
            2.0*G *
            (DVYDY[i] + trace) -
            2.0*SXY[i]*rxydot[i];

        SXYDOT[i] =
            G *
            (DVXDY[i] + DVYDX[i]) -
            rxydot[i] *
            (SXX[i] - SYY[i]);

        XDOT[i] = VX[i] + VXBAR[i];
        YDOT[i] = VY[i] + VYBAR[i];

        EDOT[i] = -0.5 * EDOT[i];
    }


    /* --------------------------------------------------------
       Ball-Ball Interaction
       -------------------------------------------------------- */

    #pragma capc profitability_region begin
    for(i = N-NB; i < N; i++)
    {
        sigxxi = SXX[i] + P[i];
        sigyyi = SYY[i] + P[i];
        sigxyi = SXY[i];

        rho2i =
            1.0 / (RHO[i] * RHO[i]);


        for(j = N-NB; j < N; j++)
        {
            if(i != j)
            {
                xij = X[i] - X[j];
                yij = Y[i] - Y[j];

                vxij = VX[i] - VX[j];
                vyij = VY[i] - VY[j];

                dist = xij*xij + yij*yij;

                if(dist <= hsq)
                {
                    dist = sqrt(dist);

                    mata = wfgrad[i][1];
                    matb = wfgrad[i][2];
                    matc = wfgrad[i][3];
                    matd = wfgrad[i][4];

                    kernel(wf, dist);

                    wf_grad =
                        wf[0] * MASS[j] /
                        RHO[j] / wfgrad[i][0];

                    wfdx =
                        1.0 /
                        (mata*matd - matb*matc) *
                        (wfgrad[i][4] * wf[1] * xij / dist
                        - wfgrad[i][2] * wf[1] * yij / dist);

                    wfdy =
                        1.0 /
                        (mata*matd - matb*matc) *
                        (wfgrad[i][1] * wf[1] * yij / dist
                        - wfgrad[i][3] * wf[1] * xij / dist);


                    rhobar = RHO[j];

                    drhobar = dist * rhobar;

                    RHODOT[i] +=
                        MASS[j] *
                        (vxij*wfdx + vyij*wfdy);


                    sigxxj = SXX[j] + P[j];
                    sigyyj = SYY[j] + P[j];
                    sigxyj = SXY[j];

                    rho2j =
                        1.0 / (RHO[j] * RHO[j]);


                    if(MONA_CORR == 1)
                        monacorr(
                            i, j, wf,
                            vxij, vyij,
                            wf_grad,
                            wfdx, wfdy
                        );


                    if(JAUMANN == 1)
                    {
                        rxydot[i] +=
                            -0.5 *
                            MASS[j] / RHO[j] *
                            (vxij*wfdy - vyij*wfdx);
                    }


                    basicsph(
                        i, j,
                        wf,
                        xij, yij,
                        vxij, vyij,
                        rho2i, rho2j,
                        dist,
                        sigxxi, sigyyi, sigxyi,
                        sigxxj, sigyyj, sigxyj,
                        drhobar,
                        wf_grad,
                        wfdx, wfdy
                    );


                    if(ART_VISCOSITY == 1)
                    {
                        viscosity(
                            visc,
                            i, j,
                            xij, yij,
                            vxij, vyij,
                            dist,
                            wf,
                            wf_grad,
                            wfdx, wfdy
                        );
                    }


                    if(TENSILE == 1)
                    {
                        artificial_pressure(
                            P[i], P[j],
                            rho2i, rho2j,
                            wf,
                            xij, yij,
                            dist,
                            i, j,
                            vxij, vyij,
                            wf_grad,
                            wfdx, wfdy
                        );
                    }
                }
            }
        }


        trace =
            -1.0/3.0 *
            (DVXDX[i] + DVYDY[i]);

        SXXDOT[i] =
            2.0*G *
            (DVXDX[i] + trace) +
            2.0*SXY[i]*rxydot[i];

        SYYDOT[i] =
            2.0*G *
            (DVYDY[i] + trace) -
            2.0*SXY[i]*rxydot[i];

        SXYDOT[i] =
            G *
            (DVXDY[i] + DVYDX[i]) -
            rxydot[i] *
            (SXX[i] - SYY[i]);

        XDOT[i] = VX[i] + VXBAR[i];
        YDOT[i] = VY[i] + VYBAR[i];

        EDOT[i] = -0.5 * EDOT[i];
    }
    #pragma capc profitability_region end


    /* --------------------------------------------------------
       Monaghan correction
       -------------------------------------------------------- */

    #pragma capc profitability_region begin
    #pragma acc parallel loop copyin(VXBAR[0:N],VYBAR[0:N]) copy(VX[0:N],VY[0:N])
    for(i = 0; i < N; i++)
    {
        if(MONA_CORR2 == 1)
        {
            VX[i] = VX[i] + VXBAR[i];
            VY[i] = VY[i] + VYBAR[i];
        }
    }
    #pragma capc profitability_region end
}


/* ============================================================
   Basic SPH
   ============================================================ */

void basicsph(
    int i,
    int j,
    double wf[],
    double xij,
    double yij,
    double vxij,
    double vyij,
    double rho2i,
    double rho2j,
    double dist,
    double sigxxi,
    double sigyyi,
    double sigxyi,
    double sigxxj,
    double sigyyj,
    double sigxyj,
    double drhobar,
    double wf_grad,
    double wfdx,
    double wfdy
)
{
    double smxx, smyx, smxy, smyy;

    double rhobar = drhobar / dist;


    smxx = -wfdx * vxij / rhobar;
    smxy = -wfdy * vxij / rhobar;
    smyx = -wfdx * vyij / rhobar;
    smyy = -wfdy * vyij / rhobar;


    DVXDX[i] += MASS[j] * smxx;
    DVXDY[i] += MASS[j] * smxy;
    DVYDX[i] += MASS[j] * smyx;
    DVYDY[i] += MASS[j] * smyy;


    VXDOT[i] +=
        MASS[j] *
        (sigxxi*rho2i + sigxxj*rho2j) *
        wfdx
        +
        MASS[j] *
        (sigxyi*rho2i + sigxyj*rho2j) *
        wfdy;


    VYDOT[i] +=
        MASS[j] *
        (sigyyi*rho2i + sigyyj*rho2j) *
        wfdy
        +
        MASS[j] *
        (sigxyi*rho2i + sigxyj*rho2j) *
        wfdx;


    EDOT[i] +=
        MASS[j] *
        vxij *
        (sigxxi*rho2i + sigxxj*rho2j) *
        wfdx;


    EDOT[i] +=
        MASS[j] *
        vxij *
        (sigxyi*rho2i + sigxyj*rho2j) *
        wfdy;


    EDOT[i] +=
        MASS[j] *
        vyij *
        (sigyyi*rho2i + sigyyj*rho2j) *
        wfdy;


    EDOT[i] +=
        MASS[j] *
        vyij *
        (sigxyi*rho2i + sigxyj*rho2j) *
        wfdx;
}


/* ============================================================
   Monaghan Correction
   ============================================================ */

void monacorr(
    int i,
    int j,
    double wf[],
    double vxij,
    double vyij,
    double wf_grad,
    double wfdx,
    double wfdy
)
{
    VXBAR[i] -=
        0.50 *
        MASS[j] *
        wf[0] *
        vxij /
        (RHO[i] + RHO[j]);

    VYBAR[i] -=
        0.50 *
        MASS[j] *
        wf[0] *
        vyij /
        (RHO[i] + RHO[j]);
}


/* ============================================================
   Artificial Pressure
   ============================================================ */

void artificial_pressure(
    double Pi,
    double Pj,
    double rho2i,
    double rho2j,
    double wf[],
    double xij,
    double yij,
    double dist,
    int i,
    int j,
    double vxij,
    double vyij,
    double wf_grad,
    double wfdx,
    double wfdy
)
{
    double tensterm1 = 0.0;
    double tensterm2 = 0.0;
    double tensterm;

    double wfinit[2];

    double tensgamma = 0.50;
    double temp;


    kernel(wfinit, DY);

    temp = wf[0] / wfinit[0];

    temp = temp * temp;
    temp = temp * temp;


    if(Pi > 0.0)
        tensterm1 = Pi * rho2i;

    if(Pj > 0.0)
        tensterm2 = Pj * rho2j;


    tensterm =
        tensgamma *
        (tensterm1 + tensterm2) *
        temp;


    VXDOT[i] +=
        -1.0 *
        MASS[j] *
        tensterm *
        wfdx;


    VYDOT[i] +=
        -1.0 *
        MASS[j] *
        tensterm *
        wfdy;


    EDOT[i] +=
        -1.0 *
        MASS[j] *
        vxij *
        tensterm *
        wfdx;


    EDOT[i] +=
        -1.0 *
        MASS[j] *
        vyij *
        tensterm *
        wfdy;
}


/* ============================================================
   Lucy Kernel
   ============================================================ */

void kernel(double wf[], double dist)
{
    double h = H;
    double q = dist / h;

    double par =
        5.0 /
        (3.14159265359 * h * h);


    wf[0] =
        par *
        (1.0 + 3.0*q) *
        (1.0-q) *
        (1.0-q) *
        (1.0-q);


    wf[1] =
        -par *
        12.0 / h *
        q *
        (1.0-q) *
        (1.0-q);
}


/* ============================================================
   Update
   ============================================================ */

void Update(double dt)
{
    int i;

    #pragma capc profitability_region begin
    #pragma acc parallel loop copyin(RHO[0:N],RHODOT[0:N],E[0:N],EDOT[0:N],X[0:N],XDOT[0:N],Y[0:N],YDOT[0:N],VX[0:N],VXDOT[0:N],VY[0:N],VYDOT[0:N],SXX[0:N],SXXDOT[0:N],SXY[0:N],SXYDOT[0:N],SYY[0:N],SYYDOT[0:N]) copyout(RHON[0:N],EN[0:N],XN[0:N],YN[0:N],VXN[0:N],VYN[0:N],SXXN[0:N],SXYN[0:N],SYYN[0:N])
    for(i = 0; i < N; i++)
    {
        RHON[i] =
            RHO[i] +
            0.5 * dt * RHODOT[i];

        EN[i] =
            E[i] +
            0.5 * dt * EDOT[i];

        XN[i] =
            X[i] +
            0.5 * dt * XDOT[i];

        YN[i] =
            Y[i] +
            0.5 * dt * YDOT[i];

        VXN[i] =
            VX[i] +
            0.5 * dt * VXDOT[i];

        VYN[i] =
            VY[i] +
            0.5 * dt * VYDOT[i];

        SXXN[i] =
            SXX[i] +
            0.5 * dt * SXXDOT[i];

        SXYN[i] =
            SXY[i] +
            0.5 * dt * SXYDOT[i];

        SYYN[i] =
            SYY[i] +
            0.5 * dt * SYYDOT[i];
    }
    #pragma capc profitability_region end
}


/* ============================================================
   Artificial Viscosity
   ============================================================ */

void viscosity(
    double visc[],
    int i,
    int j,
    double xij,
    double yij,
    double vxij,
    double vyij,
    double dist,
    double wf[],
    double wf_grad,
    double wfdx,
    double wfdy
)
{
    double delVdelR;
    double muij;

    double ci = sqrt(EMOD / RHO[i]);
    double cj = sqrt(EMOD / RHO[j]);


    delVdelR =
        xij * vxij +
        yij * vyij;


    muij =
        (H * delVdelR) /
        (dist*dist + 0.01*H*H);


    visc[0] = 0.0;


    if(delVdelR < 0.0)
    {
        visc[0] =
            (
                -0.5 *
                (ci+cj) *
                muij *
                alpha
                +
                beta *
                muij *
                muij
            )
            /
            (0.5 * (RHO[i] + RHO[j]));
    }


    VXDOT[i] +=
        -1.0 *
        MASS[j] *
        visc[0] *
        wfdx;


    VYDOT[i] +=
        -1.0 *
        MASS[j] *
        visc[0] *
        wfdy;


    EDOT[i] +=
        -1.0 *
        MASS[j] *
        vxij *
        visc[0] *
        wfdx;


    EDOT[i] +=
        -1.0 *
        MASS[j] *
        vyij *
        visc[0] *
        wfdy;
}


/* ============================================================
   Initialization
   ============================================================ */

void initialize()
{
    int i;

    /*
       The original program creates two particle groups:
         1. Plate particles
         2. Ball particles

       Here we generate the same two groups deterministically
       without reading any input file.

       The particles are distributed on circular rings.
    */

    int plate_particles = N - NB;
    int ball_particles  = NB;

    double angle;
    double radius;

    double pi = 3.14159265358979323846;


    /* --------------------------------------------------------
       Initialize plate particles
       -------------------------------------------------------- */

    #pragma capc profitability_region begin
    #pragma acc parallel loop private(angle,radius) copyout(X[0:plate_particles],Y[0:plate_particles],VX[0:plate_particles],VY[0:plate_particles],E[0:plate_particles],MASS[0:plate_particles],RHO[0:plate_particles],SXX[0:plate_particles],SXY[0:plate_particles],SYY[0:plate_particles],DAMAGE[0:plate_particles],DeltaWP[0:plate_particles])
    for(i = 0; i < plate_particles; i++)
    {
        angle =
            2.0 * pi *
            (double)i /
            (double)plate_particles;

        radius =
            ri +
            (ro-ri) *
            ((double)(i % 100) / 100.0);


        X[i] =
            radius * cos(angle);

        Y[i] =
            radius * sin(angle);


        VX[i] = VRING;
        VY[i] = 0.0;

        E[i] = 0.0;

        MASS[i] = MASSP;

        RHO[i] = RHO0;

        SXX[i] = 0.0;
        SXY[i] = 0.0;
        SYY[i] = 0.0;

        DAMAGE[i] = 0.0;

        DeltaWP[i] = 0.0;
    }
    #pragma capc profitability_region end


    /* --------------------------------------------------------
       Initialize ball particles
       -------------------------------------------------------- */

    #pragma capc profitability_region begin
    #pragma acc parallel loop private(angle,radius) copyout(X[plate_particles:ball_particles],Y[plate_particles:ball_particles],VX[plate_particles:ball_particles],VY[plate_particles:ball_particles],E[plate_particles:ball_particles],MASS[plate_particles:ball_particles],RHO[plate_particles:ball_particles],SXX[plate_particles:ball_particles],SXY[plate_particles:ball_particles],SYY[plate_particles:ball_particles],DAMAGE[plate_particles:ball_particles],DeltaWP[plate_particles:ball_particles])
    for(i = 0; i < ball_particles; i++)
    {
        int idx = plate_particles + i;

        angle =
            2.0 * pi *
            (double)i /
            (double)ball_particles;

        radius =
            ri +
            (ro-ri) *
            ((double)(i % 100) / 100.0);


        X[idx] =
            87.0 +
            radius * cos(angle);

        Y[idx] =
            radius * sin(angle);


        VX[idx] = -VRING;
        VY[idx] = 0.0;

        E[idx] = 0.0;

        MASS[idx] = MASSB;

        RHO[idx] = RHO0;

        SXX[idx] = 0.0;
        SXY[idx] = 0.0;
        SYY[idx] = 0.0;

        DAMAGE[idx] = 0.0;

        DeltaWP[idx] = 0.0;
    }
    #pragma capc profitability_region end


    /* --------------------------------------------------------
       Initialize remaining arrays
       -------------------------------------------------------- */

    #pragma capc profitability_region begin
    #pragma acc parallel loop copyin(X[0:N],Y[0:N],VX[0:N],VY[0:N],RHO[0:N],E[0:N],SXX[0:N],SXY[0:N],SYY[0:N]) copyout(XO[0:N],YO[0:N],VXO[0:N],VYO[0:N],RHOO[0:N],EO[0:N],SXXO[0:N],SXYO[0:N],SYYO[0:N],XN[0:N],YN[0:N],VXN[0:N],VYN[0:N],RHON[0:N],EN[0:N],SXXN[0:N],SXYN[0:N],SYYN[0:N])
    for(i = 0; i < N; i++)
    {
        XO[i] = X[i];
        YO[i] = Y[i];

        VXO[i] = VX[i];
        VYO[i] = VY[i];

        RHOO[i] = RHO[i];

        EO[i] = E[i];

        SXXO[i] = SXX[i];
        SXYO[i] = SXY[i];
        SYYO[i] = SYY[i];

        XN[i] = X[i];
        YN[i] = Y[i];

        VXN[i] = VX[i];
        VYN[i] = VY[i];

        RHON[i] = RHO[i];

        EN[i] = E[i];

        SXXN[i] = SXX[i];
        SXYN[i] = SXY[i];
        SYYN[i] = SYY[i];
    }
    #pragma capc profitability_region end


    printf("Number of particles = %d\n", N);
    printf("Plate particles = %d\n", plate_particles);
    printf("Ball particles = %d\n", ball_particles);
}
