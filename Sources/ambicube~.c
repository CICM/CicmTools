/**********************************************************************/
//                                                                    //
// /****************************************************************/ //
// /*                                                              */ //
// /*                         AMBICUBE                             */ //
// /*                                                              */ //
// /* Auteur: Rémi MIGNOT                                          */ //
// /*         Elève ingénieur Télécom2 à l'ISPG                    */ //
// /*         (Institut Supérieur Polytechnique de Galilée),       */ //
// /*         Université Paris13.                                  */ //
// /*                                                              */ //
// /* Date de creation:   11/07/03                                 */ //
// /* Version: 1.5        17/07/04                                 */ //
// /* Version: 2.0        07/04/12                                 */ //
// /*                                                              */ //
// /* Réalisé à la MSH Paris Nord (Maison des Sciences de l'Homme) */ //
// /*         en collaboration avec A.Sedes, B.Courribet           */ //
// /*         et J.B.Thiebaut,                                     */ //
// /*         CICM Université Paris8, MSH Paris Nord,              */ //
// /*         ACI Jeunes Chercheurs "Espaces Sonores".             */ //
// /*         Version 2.0 par Eliott PARIS                         */ //
// /*                                                              */ //
// /****************************************************************/ //
//                                                                    //
/**********************************************************************/


/**
 * Copyright (C) 2003-2004 RÈmi Mignot, MSH Paris Nord,
 * 
 * This library is free software; you can redistribute it and/or modify it 
 * under the terms of the GNU Library General Public License as published 
 * by the Free Software Foundation; either version 2 of the License.
 * 
 * This library is distributed in the hope that it will be useful, but WITHOUT 
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or 
 * FITNESS FOR A PARTICULAR PURPOSE. See the GNU Library General Public 
 * License for more details.
 * 
 * You should have received a copy of the GNU Library General Public License 
 * along with this library; if not, write to the Free Software Foundation, 
 * Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 * 
 * cicm.mshparisnord.org
 * cicm.mshparisnord@gmail.com
 */


/*Description:  Cet objet permet de spatialiser un son monoral
                à l'aide de 8 haut-parleurs situés en cube autour
                de l'auditeur. La spatialisation se fait grâce à
                l'ambisonie de Michael Gerzon.                        */


/**** Version 2.0.0:
 *  methode float.
 *  methode assist.
 *  methode Main actualisée.
 *  methode DSP revue.
 *  une seule methode perform pour le contrôle et le signal (plus besoin d'écrire s ou c, les 2 sont acceptés)
 *  optimisé si objet muté;
 *  ne bug plus quand on réinstancie l'objet avec l'audio allumé.
 *  2ème arg (c/s => controle/signal) est sans effet : l'offset est donc soit le 2ème soit le 3ème arg (pour comptabilité).
 *  gestion des erreurs mise à jour.
 ****/



/**********************************************************************/
/*                            EN TETE                                 */
/**********************************************************************/
 
#include "CicmTools.h"

#define   Nhp       8      //Nombre de hp, 8 pour un cube
#define   Xdefaut   0      //Valeur par defaut de l'abscisse,
#define   Ydefaut   1      //Valeur par défaut de l'ordonnée,
#define   Zdefaut   0      //Valeur par défaut de la cote de la source,
#define   Rdefaut   1      //Distance par défaut de la source,
#define   Phidefaut 1.5708F//Valeur de l'angle polaire de la source,
#define   Ksidefaut 1.5708F//Angle d'élévation par défaut de la source,
#define   OFFSET    .3     //Offset par defaut de l'ambisonie,
#define   T_COS     4096   //Taille du tableau cosinus (puissance de 2),
#define   MASQUE    4095   //Masque pour le modulo = T_COS - 1,
#define   DTIME     10     //Temps d'interpolation par défaut en ms,
#define   Pi        3.1415926535897932384F  // Pi
#define   I360      0.0027777777777777778F  // 1/360
#define   ELEV      0.61547970867039F       //Angle  d'élévation pour 
                           //les hp du cube, par rapport à l'horizontale.
#define   EPSILON   0.000001 //CritËre pour tester si les flottants sont nuls
                          
                          

/**********************************************************/
/*         La classe                                      */
/**********************************************************/

void *ambicube_tilde_class;

typedef struct _ambicube_tilde
{  
  t_pxobject x_obj;     //Membre indispensable à MAX,
	
  int       base;	      //Type de base, 0->cylindrique, 1->cartésienne, 2->sphérique,
  int 	    controle;	  //1->coordonnées en contrôle, 0->coordonnées en signal,
  int       mute;       //1->entrées signal mutées, 0->non mutées,
	
  float     x, y, z;	  //Coordonnées de la source en cartésien,
  float     phi, r, ksi;//Coordonnées sphériques de la source,
	float   c1, c2, c3;       // coordonnées entrées dans l'inlet 1 et 2 (en contrôle ou signal);

  float     offset;     //Offset de l'ambisonie,
  int       dtime;      //Temps d'interpolation en échantillons,
	
  float	    P[8];	      //Tableau contenant les N coefficients ambisoniques Pn,
  float     teta[8];    //Angle de chaque haut-parleur en polaire,
  float     elev[8];    //Angle d'élévation de chaque haut-parleur, 

  float     dP[8];      //pas pour l'interpolation,
  float     Pstop[8];   //cible pour l'interpolation,

  float     *cosin;     //Adresse des tableaux pour les cosinus,
  float     *cos_teta;  //sinus et cosinus des angles téta et
  float     *sin_teta;  //d'élévation des haut-parleurs.
  float     *cos_elev;
  float     *sin_elev;
	int connected[4];
	
} t_ambicube_tilde;




/**********************************************************************/
/*      Prototypage des fonctions et méthodes.     	                  */
/**********************************************************************/

void  ambicube_tilde_dsp(t_ambicube_tilde *x, t_signal **sp, short *count);
t_int *ambicube_tilde_perform_signal(t_int *w);
void  ambicube_tilde_dsp64(t_ambicube_tilde *x, t_object *dsp64, short *count, double samplerate, long maxvectorsize, long flags);
void  ambicube_tilde_perform_signal64(t_ambicube_tilde *x, t_object *dsp64, double **ins, long numins, double **outs, long numouts, long sampleframes, long flags, void *userparam);
void  ambicube_tilde_recoit_x(t_ambicube_tilde *x, double xp);
void  ambicube_tilde_recoit_y(t_ambicube_tilde *x, double yp);
void  ambicube_tilde_recoit_z(t_ambicube_tilde *x, double zp);
void  ambicube_tilde_recoit_liste( t_ambicube_tilde *x, t_symbol *s, int argc, t_atom *argv);
void  ambicube_tilde_dest(t_ambicube_tilde *x);
void  ambicube_tilde_muter_entrees_signal(t_ambicube_tilde *x, int mute);
void  ambicube_tilde_changer_type_repere( t_ambicube_tilde *x, t_symbol *sym);
void  ambicube_tilde_changer_offset(t_ambicube_tilde *x, double val);
void  *ambicube_tilde_new(t_symbol *s, int argc, Atom *argv );
void  ambicube_tilde_assist(t_ambicube_tilde *x, void *b, long m, long a, char *s);
void  ambipan_tilde_recoit_float(t_ambicube_tilde *x, double data);


/**********************************************************************/
/*      TRAITEMENT DU SIGNAL avec coordonnées recues en signal.       */
/**********************************************************************/

t_int *ambicube_tilde_perform_signal(t_int *w)
{
	/*Préparation des pointeurs****************************************/
	t_ambicube_tilde *x = (t_ambicube_tilde *)(w[1]);//adresse de la dataspace
	float     *son_in   = (t_float *)(w[2]);        //son en entrée
	float     *xp       = (t_float *)(w[3]);        //réception de x
	float     *yp       = (t_float *)(w[4]);        //réception de y
	float     *zp       = (t_float *)(w[5]);        //réception de z
	int       n         = (int)(w[6+Nhp]);          //taille des blocs
	
	/*Déclarations des variables locales*/
	float son;            //variable temporaire,
	int   hp;             //indices relatif au haut-parleur,
	int   i;              //indices du n° de l'échantillon,
	int   phii, ksii;     //angles en entier de 0 à T_COS,
	int   retour = Nhp + 7;  //retour pour pure data,
	float offset = x->offset;
	
	float K = (float)(sqrt(1/(float)Nhp)/1.66);  //Facteur correctif.
	
	//Paramètres ambisoniques:
	float xtemp, xl, yl, zl, ds, dist, X, Y, Z, W, P;
	
	/*Préparation des pointeurs des sorties ***************************/
	float *out[8];
	for( hp = 0; hp < Nhp; hp++)
		out[hp] = (t_float *)(w[hp+6]);
	
	/******************************************************************/
	/*  zero  **************************************************/
	
	if (x->x_obj.z_disabled) goto noProcess;
	if (!x->connected[0]) {
		for( i=0; i<n; i++) for( hp = 0; hp < Nhp; hp++) out[hp][i] = 0;/**/
		goto noProcess;
	}
	
	/******************************************************************/
	/*  Traitements  **************************************************/
	
	if (x->connected[1]) x->c1 = xp[n-1];
	if (x->connected[2]) x->c2 = yp[n-1];
	if (x->connected[3]) x->c3 = zp[n-1];
	
	/******************************************************************/
	/*Si entrees signal mutées, on utilise le tableau P de contrôle.***/
	if(x->mute || (!x->connected[1] && !x->connected[2] && !x->connected[3]) ) 
		for( i=0; i<n; i++)
		 {
			//on stocke l'échantillon d'entrée dans une variable temporaire,
			son = son_in[i];
			
			//Modulation des sorties avec les coefficients ambisoniques Pn.
			for( hp = Nhp-1; hp >= 0; hp--)
			 {
				//Incrémentation des P pour l'interpolation,
				if( x->P[hp] == x->Pstop[hp] ) /*rien*/;
				else if ( fabs(x->Pstop[hp] - x->P[hp]) > fabs(x->dP[hp]) )
					x->P[hp] += x->dP[hp];
				else
					x->P[hp] = x->Pstop[hp];
				
				/******************************/
				out[hp][i] = son * x->P[hp];/**/
				/******************************/
			 }
		 }
	
	/******************************************************************/
	/*Si entrées signal non mutées, on utilise les vecteurs de signal**/
	else 
		for( i=n-1; i>=0; i--)
		 {
			son = son_in[i];    //On stocke les échantillons d'entrée, dans des
			xl = x->connected[1] ? xp[i] : x->c1;
			yl = x->connected[2] ? yp[i] : x->c2;
			zl = x->connected[3] ? zp[i] : x->c3;
			
			//Conversion cylindriques -> cartésiennes,
			if( !x->base )
			 {
				phii = (int)( yl*T_COS*I360 )&(int)MASQUE; 
				xtemp    = xl*x->cosin[ phii ];
				phii = (int)(.25*T_COS - phii)&(int)MASQUE;
				yl       = xl*x->cosin[ phii ];
				xl = xtemp;
			 }
			//Conversion sphériques -> cartésiennes,
			if( x->base == 2 )
			 { 
				 phii = (int)( yl*T_COS*I360 )&(int)MASQUE; 
				 ksii = (int)( .25*T_COS - zl*T_COS*I360 )&(int)MASQUE; 
				 xtemp    = xl*x->cosin[ phii ]*x->cosin[ ksii ];
				 phii = (int)(.25*T_COS - phii)&(int)MASQUE;
				 yl       = xl*x->cosin[ phii ]*x->cosin[ ksii ];
				 ksii = (int)(.25*T_COS - ksii)&(int)MASQUE;
				 zl       = xl*x->cosin[ ksii ];
				 xl = xtemp;
			 }
			
			//Calcul des distances,
			ds   = xl*xl + yl*yl + zl*zl;
			dist = (float)sqrt(ds);
			
			//Calcul des paramètres ambisoniques,
			X = (float)( 2*xl/(ds + offset) ); 
			Y = (float)( 2*yl/(ds + offset) );  
			Z = (float)( 2*zl/(ds + offset) ); 
			W = (float)( .707/(dist + offset) );
			
			//Décodage de l'ambisonie:
			for( hp=Nhp-1; hp >= 0 ; hp--)
			 {
				P = K  * ( W + X*x->cos_teta[hp]*x->cos_elev[hp]  
						  + Y*x->sin_teta[hp]*x->cos_elev[hp]
						  + Z*x->sin_elev[hp] );
				
				//Si Pn<0 on les force à 0
				if(P < 0)            P = 0;
				
				/***********************/
				out[hp][i] = son * P;/**/
				/***********************/
			 }
		 }

noProcess :
	
	return ( w+retour );
}

//----------------------------------------------------------------------------------
//                perform64
//----------------------------------------------------------------------------------

void  ambicube_tilde_perform_signal64(t_ambicube_tilde *x, t_object *dsp64, double **ins, long numins, double **outs, long numouts, long sampleframes, long flags, void *userparam)

{
	/*Préparation des pointeurs****************************************/
	t_double     *son_in   = ins[0];        //son en entrée
	t_double     *xp       = ins[1];        //réception de x
	t_double     *yp       = ins[2];        //réception de y
	t_double     *zp       = ins[3];        //réception de z
	int       n         = sampleframes;          //taille des blocs
	
	/*Déclarations des variables locales*/
	t_double son;            //variable temporaire,
	int   hp;             //indices relatif au haut-parleur,
	int   i;              //indices du n° de l'échantillon,
	int   phii, ksii;     //angles en entier de 0 à T_COS,
	t_double offset = x->offset;
	
	t_double K = (t_double)(sqrt(1/(t_double)Nhp)/1.66);  //Facteur correctif.
	
	//Paramètres ambisoniques:
	t_double xtemp, xl, yl, zl, ds, dist, X, Y, Z, W, P;
	
	/*Préparation des pointeurs des sorties ***************************/
	t_double *out[8];
	for( hp = 0; hp < Nhp; hp++)
		out[hp] = outs[hp];
	
	/******************************************************************/
	/*  zero  **************************************************/
	
	if (x->x_obj.z_disabled) goto noProcess;
	if (!x->connected[0]) {
		for( i=0; i<n; i++) for( hp = 0; hp < Nhp; hp++) out[hp][i] = 0;/**/
		goto noProcess;
	}
	
	/******************************************************************/
	/*  Traitements  **************************************************/
	
	if (x->connected[1]) x->c1 = xp[n-1];
	if (x->connected[2]) x->c2 = yp[n-1];
	if (x->connected[3]) x->c3 = zp[n-1];
	
	/******************************************************************/
	/*Si entrees signal mutées, on utilise le tableau P de contrôle.***/
	if(x->mute || (!x->connected[1] && !x->connected[2] && !x->connected[3]) ) 
		for( i=0; i<n; i++)
		 {
			//on stocke l'échantillon d'entrée dans une variable temporaire,
			son = son_in[i];
			
			//Modulation des sorties avec les coefficients ambisoniques Pn.
			for( hp = Nhp-1; hp >= 0; hp--)
			 {
				//Incrémentation des P pour l'interpolation,
				if( x->P[hp] == x->Pstop[hp] ) /*rien*/;
				else if ( fabs(x->Pstop[hp] - x->P[hp]) > fabs(x->dP[hp]) )
					x->P[hp] += x->dP[hp];
				else
					x->P[hp] = x->Pstop[hp];
				
				/******************************/
				out[hp][i] = son * x->P[hp];/**/
				/******************************/
			 }
		 }
	
	/******************************************************************/
	/*Si entrées signal non mutées, on utilise les vecteurs de signal**/
	else 
		for( i=n-1; i>=0; i--)
		 {
			son = son_in[i];    //On stocke les échantillons d'entrée, dans des
			xl = x->connected[1] ? xp[i] : x->c1;
			yl = x->connected[2] ? yp[i] : x->c2;
			zl = x->connected[3] ? zp[i] : x->c3;
			
			//Conversion cylindriques -> cartésiennes,
			if( !x->base )
			 {
				phii = (int)( yl*T_COS*I360 )&(int)MASQUE; 
				xtemp    = xl*x->cosin[ phii ];
				phii = (int)(.25*T_COS - phii)&(int)MASQUE;
				yl       = xl*x->cosin[ phii ];
				xl = xtemp;
			 }
			//Conversion sphériques -> cartésiennes,
			if( x->base == 2 )
			 { 
				 phii = (int)( yl*T_COS*I360 )&(int)MASQUE; 
				 ksii = (int)( .25*T_COS - zl*T_COS*I360 )&(int)MASQUE; 
				 xtemp    = xl*x->cosin[ phii ]*x->cosin[ ksii ];
				 phii = (int)(.25*T_COS - phii)&(int)MASQUE;
				 yl       = xl*x->cosin[ phii ]*x->cosin[ ksii ];
				 ksii = (int)(.25*T_COS - ksii)&(int)MASQUE;
				 zl       = xl*x->cosin[ ksii ];
				 xl = xtemp;
			 }
			
			//Calcul des distances,
			ds   = xl*xl + yl*yl + zl*zl;
			dist = (float)sqrt(ds);
			
			//Calcul des paramètres ambisoniques,
			X = (t_double)( 2*xl/(ds + offset) ); 
			Y = (t_double)( 2*yl/(ds + offset) );  
			Z = (t_double)( 2*zl/(ds + offset) ); 
			W = (t_double)( .707/(dist + offset) );
			
			//Décodage de l'ambisonie:
			for( hp=Nhp-1; hp >= 0 ; hp--)
			 {
				P = K  * ( W + X*x->cos_teta[hp]*x->cos_elev[hp]  
						  + Y*x->sin_teta[hp]*x->cos_elev[hp]
						  + Z*x->sin_elev[hp] );
				
				//Si Pn<0 on les force à 0
				if(P < 0)            P = 0;
				
				/***********************/
				out[hp][i] = son * P;/**/
				/***********************/
			 }
		 }
	
	noProcess :
	return;
}


/**********************************************************************/
/*        METHODE RECOIT_float                                        */
/**********************************************************************/

void  ambicube_tilde_recoit_float(t_ambicube_tilde *x, double data)
{
	int inlet = proxy_getinlet((t_object *)x);
	switch (inlet) {
		case 1:
			if (!x->connected[1]) x->c1 = data;
			ambicube_tilde_recoit_x(x, data);
			break;
		case 2:
			if (!x->connected[2]) x->c2 = data;
			ambicube_tilde_recoit_y(x, data);
			break;
		case 3:
			if (!x->connected[3]) x->c3 = data;
			ambicube_tilde_recoit_z(x, data);
			break;
		default:
			break;
	}
}

/**********************************************************************/
/*        METHODE RECOIT_x                                            */
/**********************************************************************/
//Cette méthode reçoit x et change tous les paramètres ambisoniques: 
//  X, Y, Z, W et les Pn 
void ambicube_tilde_recoit_x(t_ambicube_tilde *x, double xp)
{
  float K = (float)(sqrt(1/(float)Nhp)/1.66); 
  int hp;
  float ds, dist;
  float X, Y, Z, W;
  float yp = x->y;
  float zp = x->z;

  //Conversion cylindrique -> cartésienne,
  if( x->base == 0 )
  {
    x->r = xp;
    xp   = (float)( x->r * cos( x->phi ) );
    yp   = (float)( x->r * sin( x->phi ) );
  }
  //Conversion sphérique -> cartésienne,
  if( x->base == 2 )
  {
    x->r = xp;
    xp   = (float)( x->r * cos( x->phi )*sin( x->ksi ) );
    yp   = (float)( x->r * sin( x->phi )*sin( x->ksi ) );
    zp   = (float)( x->r * cos( x->ksi ) );
  }
  x->x = xp;
  x->y = yp;
  x->z = zp;

  //Calcul des distances,
  ds = xp*xp + yp*yp + zp*zp;
  dist = (float)sqrt(ds);

  //Calcul des paramètres ambisoniques,
  X = (float)( 2*xp/(ds + x->offset) ); 
  Y = (float)( 2*yp/(ds + x->offset) ); 
  Z = (float)( 2*zp/(ds + x->offset) ); 
  W = (float)( .707/(dist + x->offset) );

  //Calcul des coefficients ambisoniques cibles et des pas,
  for( hp = Nhp-1; hp >= 0 ; hp--)
  {
    x->Pstop[hp] = (float)( ( W + X*cos(x->teta[hp])*cos(x->elev[hp]) 
                                + Y*sin(x->teta[hp])*cos(x->elev[hp])
                                + Z*sin(x->elev[hp])
                            ) * K);
    //Si Pstop_n < 0 on les force à 0.
    if(x->Pstop[hp] < 0)
      x->Pstop[hp] = 0;

    x->dP[hp] = (x->Pstop[hp] - x->P[hp])/(float)x->dtime;
  }

  return;
}



/**********************************************************************/
/*        METHODE RECOIT_y                                            */
/**********************************************************************/
//Cette méthode reçoit y et change tous les paramètres ambisoniques: 
//  X, Y, Z, W et les Pn 
void ambicube_tilde_recoit_y(t_ambicube_tilde *x, double yp)
{
  float K = (float)(sqrt(1/(float)Nhp)/1.66); 
  int hp;
  float ds, dist;
  float X, Y, Z, W;
  float xp = x->x;
  float zp = x->z;

  //Conversion cylindrique -> cartésienne,
  if( x->base == 0 )
  {
    x->phi = yp*Pi/180;
    xp   = (float)( x->r * cos( x->phi ) );
    yp   = (float)( x->r * sin( x->phi ) );
  }
  //Conversion sphèrique -> cartésienne,
  if( x->base == 2 )
  {
    x->phi = yp*Pi/180;
    xp   = (float)( x->r * cos( x->phi )*sin( x->ksi ) );
    yp   = (float)( x->r * sin( x->phi )*sin( x->ksi ) );
    zp   = (float)( x->r * cos( x->ksi ));
  }
  x->x = xp;
  x->y = yp;
  x->z = zp;

  //Calcul des distances,
  ds = xp*xp + yp*yp + zp*zp;
  dist = (float)sqrt(ds);

  //Calcul des paramètres ambisoniques,
  X = (float)( 2*xp/(ds + x->offset) ); 
  Y = (float)( 2*yp/(ds + x->offset) );
  Z = (float)( 2*zp/(ds + x->offset) );
  W = (float)( .707/(dist + x->offset) );

  //Calcul des coefficients ambisoniques cibles et des pas,
  for( hp=Nhp-1; hp >= 0 ; hp--)
  {
    x->Pstop[hp] = (float)( ( W + X*cos(x->teta[hp])*cos(x->elev[hp]) 
                                + Y*sin(x->teta[hp])*cos(x->elev[hp])
                                + Z*sin(x->elev[hp])
                            ) * K );
    //Si Pn<0 on les force à 0
    if(x->Pstop[hp] < 0)
      x->Pstop[hp] = 0;

    x->dP[hp] = (x->Pstop[hp] - x->P[hp])/(float)x->dtime;    
  }

  return;
}




/**********************************************************************/
/*        METHODE RECOIT_z                                            */
/**********************************************************************/
//Cette méthode reçoit z et change tous les paramètres ambisoniques: 
//  X, Y, Z, W et les Pn 
void ambicube_tilde_recoit_z(t_ambicube_tilde *x, double zp)
{
  float K = (float)(sqrt(1/(float)Nhp)/1.66); 
  int hp;
  float ds, dist;
  float X, Y, Z, W;
  float xp = x->x;
  float yp = x->y;
   
  //Conversion sphérique -> cartésienne,
  if( x->base == 2 )
  {
    x->ksi = zp*Pi/180;
    xp   = (float)( x->r * cos( x->phi )*sin( x->ksi ) );
    yp   = (float)( x->r * sin( x->phi )*sin( x->ksi ) );
    zp   = (float)( x->r * cos( x->ksi ));
  }
  x->x = xp;
  x->y = yp;
  x->z = zp;

  //Calcul des distances,
  ds = xp*xp + yp*yp + zp*zp;
  dist = (float)sqrt(ds);

  //Calcul des paramètres ambisoniques,
  X = (float)( 2*xp/(ds + x->offset) ); 
  Y = (float)( 2*yp/(ds + x->offset) );
  Z = (float)( 2*zp/(ds + x->offset) );
  W = (float)( .707/(dist + x->offset) );

  //Calcul des coefficients ambisoniques cibles et des pas,
  for( hp=Nhp-1; hp >= 0 ; hp--)
  {
    x->Pstop[hp] = (float)( ( W + X*cos(x->teta[hp])*cos(x->elev[hp]) 
                                + Y*sin(x->teta[hp])*cos(x->elev[hp])
                                + Z*sin(x->elev[hp])
                            ) * K);
    //Si Pn<0 on les force à 0
    if(x->Pstop[hp] < 0)
      x->Pstop[hp] = 0;

    x->dP[hp] = (x->Pstop[hp] - x->P[hp])/(float)x->dtime;    
  }

  return;
}




/**********************************************************************/
/*        METHODE RECOIT_LISTE                                        */
/**********************************************************************/

void ambicube_tilde_recoit_liste(t_ambicube_tilde *x, t_symbol *s, int argc, t_atom *argv)
{
  float xp, yp = 0, zp = 0;
  
  //Récupération du premier paramètre (abscisse ou rayon),
  if( argv[0].a_type == A_FLOAT )
    xp = argv[0].a_w.w_float;
  else if( argv[0].a_type == A_LONG )
    xp = (float)(argv[0].a_w.w_long);
    
  //Récupération du second paramètre (ordonnée ou angle),
  if( argc >= 2 )
  {
    if( argv[1].a_type == A_FLOAT )
      yp = argv[1].a_w.w_float;
    else if( argv[1].a_type == A_LONG )
      yp = (float)(argv[1].a_w.w_long);
  } 
  else if( x->base == 1 ) yp = x->y;
  else                    yp = 180*x->phi/Pi;

  //Récupération du troisième paramètre (cote ou ksi),
  if( argc >= 3 )
  {
    if( argv[2].a_type == A_FLOAT )
      zp = argv[2].a_w.w_float;
    else if( argv[2].a_type == A_LONG )
      zp = (float)(argv[2].a_w.w_long);
  }
  else if( x->base == 2 ) zp = 180*x->ksi/Pi;
  else                    zp = x->z; 
  
  //Initialisation des valeurs,
  if( x->base == 1 )  x->y = yp;
  else                x->phi = yp*Pi/180;

  if( x->base == 2 )  x->ksi = zp*Pi/180;
  else                x->z = zp;

  ambicube_tilde_recoit_x( x, xp);

  return;
}



/**********************************************************************/
/*                   MODIFICATION DU TYPE DE REPERE                   */
/**********************************************************************/

void ambicube_tilde_changer_type_repere( t_ambicube_tilde *x, t_symbol *sym)
{
  if( sym->s_name[0] == 'c')
    x->base = 1;
  else if( sym->s_name[0] == 'p')
    x->base = 0; 
  else if( sym->s_name[0] == 's')
    x->base = 2;
  else 
    object_error((t_object *)x, "type de repere inconnu.");
}




/**********************************************************************/
/*              METHODE POUR MUTER LES ENTREES SIGNAL                 */
/**********************************************************************/

void  ambicube_tilde_muter_entrees_signal(t_ambicube_tilde *x, int mute)
{
  if( mute == 0 )    x->mute = 0;
  else               x->mute = 1;
}




/**********************************************************************/
/*                     METHODE CHANGER OFFSET                         */
/**********************************************************************/

void  ambicube_tilde_changer_offset(t_ambicube_tilde *x, double val)
{
	if( val <= EPSILON ){
		object_error((t_object *)x, "Pas d'offset negatif ou nul s'il vous plait.");
		return;
	}
	
	x->offset = val; //Changement de l'offset
	
	/*Changement des paramètres ambisoniques*/
	if( x->base )      ambicube_tilde_recoit_x( x, x->x );
    else               ambicube_tilde_recoit_x( x, x->r );    
}



/**********************************************************************/
/*                       FONCTION DESTRUCTION                         */
/**********************************************************************/

void ambicube_tilde_dest(t_ambicube_tilde *x)
{
	dsp_free((t_pxobject *)x);
	//Libération de la mémoire allouée pour les cosinus,
	freebytes( (void*)(x->cos_teta), (short)((T_COS+4*Nhp)*sizeof(float)) );
	return;
}



/**********************************************************************/
/*                       FONCTION CREATION                            */
/**********************************************************************/

void *ambicube_tilde_new(t_symbol *s, int argc, t_atom *argv )
{
	int    i, hp, newVersion;
	char   car;
	
	/*Allocation de la dataspace ****************************/
	t_ambicube_tilde *x = object_alloc(ambicube_tilde_class);
	
	/*****************************************************/  
	/*Récupération du type repère ************************/
	if( argc >=1 ){
		if( argv[0].a_type == A_SYM )
			car = (char)(argv[0].a_w.w_sym->s_name[0]);
		
		if( car == 'c' )
			x->base=1;
		else if( car == 'p' )
			x->base=0;
		else if( car == 's')
			x->base=2;
		else {
			x->base=1;
			object_error((t_object *)x, "erreur dans le type des coordonnees, elles sont cartesiennes par defaut.");
		}
	}
	else 
		x->base = 1;
	
	/*Récupération du type des entrées (pour des questions de compatibilité avec l'ancienne version) *******************/
	if( argc >=2 && argv[1].a_type == A_SYM){
		object_error((t_object *)x, "l'argument signal/controle est sans effet dans cette version.");
		newVersion = 0;
	} else newVersion = 1;
	
	if (!newVersion) { // si ancienne version : arg 4 = offset, arg 5 = interp time :
		/*Récupération de l'offset ***************************/
		if( argc >= 3 && argv[2].a_type == A_FLOAT )
			x->offset = fabs(argv[2].a_w.w_float);
		else if( argc >= 3 && argv[2].a_type == A_LONG )
			x->offset = fabs(argv[2].a_w.w_long);
		else
			x->offset = (float)OFFSET;
		if( x->offset <= EPSILON ){
			object_error((t_object *)x, "Pas d'offset negatif ou nul s'il vous plait.");
			x->offset = (float)OFFSET;;
		}
		
		
		/*Récupération du temps d'interpolation **************/
		if( argc >= 4 && argv[3].a_type == A_LONG )
			x->dtime = (int)(argv[3].a_w.w_long*sys_getsr()/1000.);
		else
			x->dtime = (int)(DTIME*sys_getsr()/1000.);
		if( x->dtime <= 0 )
			x->dtime = 1;
	} 
	else {
		
		/*Récupération de l'offset ***************************/
		if( argc >= 2 && argv[1].a_type == A_FLOAT )
			x->offset = fabs(argv[1].a_w.w_float);
		else if( argc >= 3 && argv[1].a_type == A_LONG )
			x->offset = fabs(argv[1].a_w.w_long);
		else
			x->offset = (float)OFFSET;
		if( x->offset <= EPSILON ){
			object_error((t_object *)x, "Pas d'offset negatif ou nul s'il vous plait.");
			x->offset = (float)OFFSET;;
		}
		
		/*Récupération du temps d'interpolation **************/
		if( argc >= 3 && argv[2].a_type == A_LONG )
			x->dtime = (int)(argv[2].a_w.w_long*sys_getsr()/1000.);
		else
			x->dtime = (int)(DTIME*sys_getsr()/1000.);
		if( x->dtime <= 0 )
			x->dtime = 1;
	}
	
	
	/********************************************************/
	/*Initialisation des données de la classe. **************/
	for( hp = Nhp-1; hp >= 0; hp--)  //Initialisation des P
		x->P[hp] = 0;
	
	x->mute= 0;             //entrées signal actives
	x->z   = Zdefaut;       //initialisation de z
	x->y   = Ydefaut;       //initialisation de y
	x->phi = Phidefaut;     //initialisation de phi
	x->ksi = Ksidefaut;     //initialisation de ksi
	
	//Initialisation des angles des haut-parleurs et des rayons,
	for( hp=0; hp<4; hp++) {
		x->teta[hp] = x->teta[hp+4] 
		= (float)( Pi*( .5 + (1 - 2*hp )/(float)4) );
		x->elev[hp] = - ( x->elev[hp+4] = ELEV );
	 }
	
	/********************************************************/
	/* Création des tableaux cosinus, cos_teta et sin_teta. */
	/* pour l'audio.                                        */
    x->cos_teta = (float*)getbytes( (T_COS + 4*Nhp)*sizeof(float) );
    x->sin_teta = x->cos_teta+Nhp;
    x->cos_elev = x->cos_teta+2*Nhp;
    x->sin_elev = x->cos_teta+3*Nhp; 
	
    //Précalculs des cos et sin des haut-parleurs.
    for( hp=0; hp<Nhp; hp++) {
		x->cos_teta[hp] = (float)cos( x->teta[hp] );
		x->sin_teta[hp] = (float)sin( x->teta[hp] );
		x->cos_elev[hp] = (float)cos( x->elev[hp] );
		x->sin_elev[hp] = (float)sin( x->elev[hp] );
	 }
	
    //Remplissage des tableaux cosinus.
    x->cosin = x->cos_teta+4*Nhp;
    /*Pour avoir besoin de cos( phi ), on cherche:
	 cos(phi) = 
	 cosin[ (int)( phi*T_COS/360 ))&(((int)T_COS-1) ] */
    for( i=0; i<T_COS; i++)
		x->cosin[i] = (float)cos( i*2*Pi/T_COS );
	
	/********************************************************/
	//initialisation de x, X, Y, Z, W et Pn par la méthode recoit x.
	if( x->base )    ambicube_tilde_recoit_x( x, Xdefaut);
	else             ambicube_tilde_recoit_x( x, Rdefaut); 
	
	/********************************************************/
	/*Création des nouvelles entrées. ***********************/
	dsp_setup( (t_pxobject *)x, 4);
    
	/*Création des sorties***********************************/
	for( i=0; i < Nhp; i++) outlet_new((t_object *)x, "signal");
	
	return (void *)x;
}

/**********************************************************************/
/*                    ASSISTANCE METHODE    	                      */
/**********************************************************************/

void ambicube_tilde_assist(t_ambicube_tilde *x, void *b, long m, long a, char *s)
{
	if (m == ASSIST_INLET) {
		switch (a) {
			case 0:
				sprintf(s,"(Signal) Mono Source input");
				break;
			case 1:
				if (x->base == 1) sprintf(s,"(Signal/float) Abscissa of the Source");
				else sprintf(s,"(Signal/float) Distance of the Source");
				break;
			case 2:
				if (x->base == 1) sprintf(s,"(Signal/float) Ordinate of the Source");
				else sprintf(s,"(Signal/float) Angle of the Source");
				break;
			case 3:
				if (x->base == 1) sprintf(s,"(Signal/float) height of the source.");
				else sprintf(s,"(Signal/float) elevation angle of the source.");
				break;
		}
	}
	else
		sprintf(s,"(Signal) Out %ld", (a+1));
}


/**********************************************************************/
/*                    DEFINITION DE LA CLASSE	                      */
/**********************************************************************/

void ext_main(void *r)
{
	t_class *c = class_new("ambicube~", 
						   (method)ambicube_tilde_new, 
						   (method)ambicube_tilde_dest, 
						   (short)sizeof(t_ambicube_tilde), NULL, A_GIMME, 0);
	
	//-------------Définition des méthodes-------------------//
	class_addmethod(c, (method)ambicube_tilde_dsp, "dsp", A_CANT, 0);
	class_addmethod(c, (method)ambicube_tilde_dsp64, "dsp64", A_CANT, 0);
	class_addmethod(c, (method)ambicube_tilde_recoit_liste, "list", A_GIMME, 0);
	class_addmethod(c, (method)ambicube_tilde_recoit_float, "float", A_FLOAT, 0);
	class_addmethod(c, (method)ambicube_tilde_changer_offset, "set_offset", A_FLOAT, 0);
	class_addmethod(c, (method)ambicube_tilde_muter_entrees_signal, "mute_sig", A_LONG, 0);
	class_addmethod(c, (method)ambicube_tilde_changer_type_repere, "change_type", A_SYM, 0);
	class_addmethod(c, (method)ambicube_tilde_assist,"assist",A_CANT,0);	
	
	cicmtools_post_credits();
	
	class_dspinit(c);
	class_register(CLASS_BOX, c);
	ambicube_tilde_class = c;
}




/**********************************************************************/
/*                       METHODE DSP                                  */
/**********************************************************************/

void ambicube_tilde_dsp(t_ambicube_tilde *x, t_signal **sp, short *count)
{
	long i;
	for(i=0; i<4; i++) x->connected[i] = count[i];
	
	dsp_add(ambicube_tilde_perform_signal, 14, x,
			sp[0]->s_vec, sp[1]->s_vec, sp[2]->s_vec,
			sp[3]->s_vec, sp[4]->s_vec, sp[5]->s_vec, 
			sp[6]->s_vec, sp[7]->s_vec, sp[8]->s_vec, 
			sp[9]->s_vec, sp[10]->s_vec, sp[11]->s_vec,
			sp[0]->s_n);
}


void ambicube_tilde_dsp64(t_ambicube_tilde *x, t_object *dsp64, short *count, double samplerate, long maxvectorsize, long flags)
{
	int i;
	for(i=0; i<4; i++) x->connected[i] = count[i];
	object_method(dsp64, gensym("dsp_add64"), x, ambicube_tilde_perform_signal64, 0, NULL);
}
