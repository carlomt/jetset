//=================================================================
//
//  FUNZIONI SPETTRO SINCROTRONE
//=================================================================
#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <string.h>
#include <unistd.h>
#include "Blazar_SED.h"

/**
 * \file funzioni_sincrotrone.c
 * \author Andrea Tramacere
 * \date 27-04-2004
 * \brief spettro di sincrotrone
 *
 */





//=========================================================================================
// Sync F(X) log-log interpolation
//=========================================================================================
double F_K_53(struct spettro * pt, double x){
    return log_log_interp(log10(x), pt->log_F_Sync_x, pt->log_x_Bessel_min, pt->log_x_Bessel_max, pt->log_F_Sync_y,static_bess_table_size,0  );
}


double F_K_ave(struct spettro *pt, double x){
    return log_log_interp(log10(x), pt->log_F_ave_Sync_x, pt->log_x_ave_Bessel_min, pt->log_x_ave_Bessel_max, pt->log_F_ave_Sync_y,static_bess_table_size,0  );

}
//=========================================================================================


//=========================================================================================
// j_nu Sync integrands
//=========================================================================================
double F_int_fix(struct spettro * pt,unsigned long  ID){
    //PITCH ANGLE FIXED
    double a, y,g;
    g=pt->griglia_gamma_Ne_log[ID];
    y=(pt->nu/(g*g))*pt->C2_Sync_K53;
    a=F_K_53(pt, y);
    a*=pt->Ne[ID];
    return a;
}

double F_int_ave(struct spettro * pt,unsigned long  ID){
    //PITCH ANGLE AVE
    double a, y,g;
    g=pt->griglia_gamma_Ne_log[ID];
    y=pt->nu/(g*g)*pt->C2_Sync_K_AVE;
    a=F_K_ave(pt, y);
    a*=pt->Ne[ID];
    return a;
}
//=========================================================================================







//=========================================================================================
//    integrand for alfa_nu_Sync
//=========================================================================================
double Sync_self_abs_int(struct spettro *pt,unsigned long  ID){
	unsigned long i;
    double a,g, y, x1, x2, y1, y2, delta;
    
    

    //----------derivative-----------------
    //if i=0 forward
    //if i>0 back
    //-----------------------------------
    if(ID==0){
        x1=pt->griglia_gamma_Ne_log[ID];
        x2=pt->griglia_gamma_Ne_log[ID+1];
        y1=pt->Ne[ID]/(x1*x1);
        y2=pt->Ne[ID+1]/(x2*x2);
    }
    else{
        x1=pt->griglia_gamma_Ne_log[ID-1];
        x2=pt->griglia_gamma_Ne_log[ID];
        y1=pt->Ne[ID-1]/(x1*x1);
        y2=pt->Ne[ID]/(x2*x2);
    }
    
    delta=x2-x1;
    a=(y2-y1)/delta;

    g=pt->griglia_gamma_Ne_log[ID];
    if (pt->Sync_kernel==0){
    	y=(pt->nu/(g*g))*pt->C2_Sync_K53;
    	a*=(g*g)*F_K_53(pt, y);
    }
    else{
    	y=pt->nu/(g*g)*pt->C2_Sync_K_AVE;
    	a*=(g*g)*F_K_ave(pt, y);
    }

    return a;   
}
//=========================================================================================





//=========================================================================================
// Radiative transfer solution for sefl abs
// see Kataoka Thesis, page 299
// tau_nu in I_nu=alfa_nu*R, so we multiply by 0.5
double solve_S_nu_Sync(struct spettro * pt, unsigned long  NU_INT){
	double S_nu,tau_nu;
    pt->I_nu_Sync[NU_INT] = 0.0;


	if (pt->do_Sync == 2) {
		tau_nu = 2 * pt->R * pt->alfa_Sync[NU_INT];
		if (tau_nu > 1e-4) {
			pt->I_nu_Sync[NU_INT] =
					(pt->j_Sync[NU_INT] / pt->alfa_Sync[NU_INT])*
					(1 - exp(-tau_nu*0.5));

			S_nu =(pt->j_Sync[NU_INT] / pt->alfa_Sync[NU_INT])*
					(1.0 - (2 / (tau_nu * tau_nu))*(1 - exp(-tau_nu)*(tau_nu + 1)));
		} else {
			pt->I_nu_Sync[NU_INT] =
					(pt->j_Sync[NU_INT] / pt->alfa_Sync[NU_INT])*
					( tau_nu*0.5 - (1.0 / 4.0) * tau_nu * tau_nu*0.5*0.5);
			S_nu =(pt->j_Sync[NU_INT] / pt->alfa_Sync[NU_INT])*
					((2.0 / 3.0) * tau_nu - (1.0 / 4.0) * tau_nu * tau_nu);
		}
		//printf("ratio nu %e, %e\n",nu,(S_nu/pt->I_nu_Sync[NU_INT]));
	}

	//==========================
	//Radiative solution for no self abs
	//limit of S_nu,alfa->0=(4/3)*R
	if (pt->do_Sync == 1) {
		pt->I_nu_Sync[NU_INT]=pt->j_Sync[NU_INT] * pt->R;
		S_nu = pt->j_Sync[NU_INT] * pt->R*four_by_three;
	}
	if (pt->verbose>1) {
		printf("#-> nu=%e j=%e alfa=%e tau_nu=%e  I_nu=%e\n", pt->nu_Sync[NU_INT], pt->j_Sync[NU_INT],
				pt->alfa_Sync[NU_INT], tau_nu, pt->I_nu_Sync[NU_INT]);
	}


	return S_nu;
}
//=========================================================================================






//=========================================================================================
//  Synchrotron emissivity j_nu_Sync
//=========================================================================================
double j_nu_Sync(struct spettro * f){
    double a;
    double gamma_sp;
    double (*pf_fint) (struct spettro * ,unsigned long  ID);
    /*** segli in base al kernel ***/
    if (f->Sync_kernel==0){
		pf_fint=&F_int_fix;
		a=integrale_Sync(pf_fint, f);
		return a*f->C1_Sync_K53;
    }
    else {
    	pf_fint=&F_int_ave;
    	a=integrale_Sync(pf_fint, f);
    	return a*f->C1_Sync_K_AVE;
    }
}
//=========================================================================================






//=========================================================================================
// Synch self abs alfa_nu_Sync
//=========================================================================================
double alfa_nu_Sync(struct spettro * f){
    double a;
    double (*pf_fint1) (struct spettro * ,unsigned long  ID);
    pf_fint1=&Sync_self_abs_int;
    a=integrale_Sync(pf_fint1, f);
    return a*f->C3_Sync_K53*(f->B)/(f->nu*f->nu);
}
//=========================================================================================





//=========================================================================================
// Sync INTEGRATION WITH SIMPSON AND GRIGLIA EQUI-LOG
//=========================================================================================
double integrale_Sync(double (*pf) (struct spettro *, unsigned long  ID), struct spettro * pt ) {
    double integr, y1, y2, y3, x1, x2, x3;
    double delta;
    unsigned long  ID;
    integr=0;
    x1=pt->griglia_gamma_Ne_log[0];
    y1=pf(pt,0);


    for(ID=1;ID<pt->gamma_grid_size-1;ID++){

        y2=pf(pt,ID);
        ID++;
        x3=pt->griglia_gamma_Ne_log[ID];
        y3=pf(pt,ID);
               

        //QUESTO DELTA RIMANE QUI
        //PERCHE' LA GRIGLIA NON E' EQUISPACED
        //NON PUO ANDARE FUORI DAL LOOP
        delta=(x3-x1);
        integr+=(y1+4.0*y2+y3)*delta;
        y1=y3;
        x1=x3;
        
    }
    if(pt->verbose>2){
        printf("Synch Integr=%e\n", integr);
    }
    return integr*(0.5/3.0);
}
//=========================================================================================




//=========================================================================================
double Sync_tcool(struct spettro * pt, double g){
    	return g/Sync_cool(pt,g);
}
//=========================================================================================




//=========================================================================================
double Sync_cool(struct spettro * pt, double g){
	double beta_gamma,c;
	beta_gamma=eval_beta_gamma(g);
    if (pt->Sync_kernel==0){
    	c=2.0*beta_gamma*beta_gamma*pt->UB*g*g*pt->sin_psi*pt->sin_psi;

    }
    else{
    	c=four_by_three*beta_gamma*beta_gamma*pt->UB*g*g;

    }

    return pt->COST_Sync_COOLING*c;
}
//=========================================================================================
