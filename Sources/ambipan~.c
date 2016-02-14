/**********************************************************************/
//                                                                    //
// /****************************************************************/ //
// /*                                                              */ //
// /*                         AMBIPAN                              */ //
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
                à l'aide de N haut-parleurs situés en cercle autour
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
 *  3ème arg (c/s => controle/signal) est sans effet : l'offset est donc soit le 3ème soit le 4ème arg (pour comptabilité).
 *  gestion des erreurs mise à jour.
 *  16->64 hp Max.
 ****/



/**********************************************************************/
/*                            EN TETE                                 */
/**********************************************************************/
 
#include "CicmTools.h"

#define   Nmax      64     //Nombre maximum de haut-parleurs,
#define   Ndefaut   4      //Nombre de haut-parleurs par défaut,
#define   Xdefaut   0      //Valeur par defaut de l'absisse,
#define   Ydefaut   1      //Valeur par défaut de l'ordonnée,
#define   Phidefaut 1.5708F//Valeur par défaut de l'angle (PI/2),
#define   Rdefaut   1      //Distance par défaut de la source,
#define   OFFSET    .3     //Offset par defaut de l'ambisonie,
#define   T_COS     4096   //Taille du tableau cosinus (puissance de 2),
#define   MASQUE    4095   //Masque pour le modulo = T_COS - 1,
#define   DTIME     10     //Temps d'interpolation par dÈfaut,
                           //en Èchantillons (10ms),
#define   Pi        3.1415926535897932384F  // Pi
#define   I360      0.0027777777777777778F  // 1/360
#define   EPSILON   0.000001 //CritËre pour tester si les flottants sont nuls



/**********************************************************/
/*         La classe                                      */
/**********************************************************/

t_class* ambipan_tilde_class;

typedef struct _ambipan_tilde
{
	t_pxobject x_obj;     //Membre indispensable à Pure Data,
	
	int     base;         //Type de base, 1->cartésienne, 0->polaire,
	int     mute;         //1->entrées signal mutées, 0->non mutées,
	int     Nout;         //Nombre de sorties,
	int     N;            //Nombre de haut-parleurs.
	
	float   x, y;         //Coordonnées de la source en cartésien,
	float   phi, r;       //Coordonnées polaires de la source,
	float   c1, c2;       // coordonnées entrées dans l'inlet 1 et 2 (en contrôle ou signal);
	
	float   offset;       //Offset de l'ambisonie,
	int     dtime;        //Temps d'interpolation en échantillons,
	
	float   P[Nmax];      //Tableau contenant les N coefficients ambisoniques Pn,
	float   teta[Nmax];   //Angle de chaque haut-parleur,
	float   dist[Nmax];   //Distance des haut-parleurs,
	
	float   dP[Nmax];     //pas pour l'interpolation,
	float   Pstop[Nmax];  //cible pour l'interpolation,
	
	float   *cosin;       //Adresse des tableaux pour les cosinus,
	float   *cos_teta;    //les sinus et cosinus des angles des 
	float   *sin_teta;    //haut-parleurs.
	int     connected[3];
} t_ambipan_tilde;



  
/**********************************************************************/
/*      Prototypage des fonctions et méthodes.     	                  */
/**********************************************************************/

void  ambipan_tilde_dsp64(t_ambipan_tilde *x, t_object *dsp64, short *count, double samplerate, long maxvectorsize, long flags);
void  ambipan_tilde_perform_signal64(t_ambipan_tilde *x, t_object *dsp64, double **ins, long numins, double **outs, long numouts, long sampleframes, long flags, void *userparam);
void  ambipan_tilde_recoit_x(t_ambipan_tilde *x, double xp);
void  ambipan_tilde_recoit_y(t_ambipan_tilde *x, double yp);
void  ambipan_tilde_recoit_liste( t_ambipan_tilde *x, t_symbol *s, int argc, t_atom *argv);
void  ambipan_tilde_initialiser_nb_hp( t_ambipan_tilde *x, long N);
void  ambipan_tilde_dist_teta_positionner_hp( t_ambipan_tilde *x, t_symbol *s, int argc, t_atom *argv );
void  ambipan_tilde_teta_positionner_hp( t_ambipan_tilde *x, t_symbol *s, int argc, t_atom *argv );
void  ambipan_tilde_xy_positionner_hp( t_ambipan_tilde *x, t_symbol *s, int argc, t_atom *argv );
void  ambipan_tilde_changer_offset( t_ambipan_tilde *x, double val);
void  ambipan_tilde_changer_type_repere(t_ambipan_tilde *x, t_symbol *sym);
void  ambipan_tilde_muter_entrees_signal(t_ambipan_tilde *x, int mute);
void  ambipan_tilde_informations( t_ambipan_tilde *x );
void  ambipan_tilde_dest(t_ambipan_tilde *x);
void  *ambipan_tilde_new(t_symbol *s, int argc, t_atom *argv );
void  ambipan_tilde_assist(t_ambipan_tilde *x, void *b, long m, long a, char *s);
void  ambipan_tilde_recoit_float(t_ambipan_tilde *x, double data);

/**********************************************************************/
/*                       METHODE DSP                                  */
/**********************************************************************/

void ambipan_tilde_dsp64(t_ambipan_tilde *x, t_object *dsp64, short *count, double samplerate, long maxvectorsize, long flags)
{
    int i;
    for(i=0; i<3; i++) x->connected[i] = count[i];
    object_method(dsp64, gensym("dsp_add64"), x, ambipan_tilde_perform_signal64, 0, NULL);
}

//---------------------------------------------------------------------//

void ambipan_tilde_perform_signal64(t_ambipan_tilde *x, t_object *dsp64, double **ins, long numins, double **outs, long numouts, long sampleframes, long flags, void *userparam)
{
	/*Préparation des pointeurs****************************************/
	t_double     *son_in   = ins[0];       //son en entrée
	t_double     *xp       = ins[1];       //réception de x
	t_double     *yp       = ins[2];       //réception de y
	int          n         = sampleframes;     //taille des blocs
	
	/*Déclarations des variables locales*/
	t_double son;            //variable temporaire,
	int   hp;             //indices relatif au haut-parleur,
	int   i;              //indices du n° de l'échantillon,
	int   N = x->N;       //Nombre de haut-parleur,_
	t_double offset = x->offset;
	
	t_double K = (t_double)(sqrt(1/(t_double)N)/1.66);   //Facteur correctif.
	
	//Paramètres ambisoniques:
	t_double xtemp, xl, yl, ds, dist, X, Y, W, P;
	int   phii;
	
	/*Préparation des pointeurs des sorties ***************************/
	t_double *out[Nmax];
	for( hp = 0; hp < x->Nout; hp++)
		out[hp] = outs[hp];
	
	/******************************************************************/
	/*  zero  **************************************************/
	
	if (x->x_obj.z_disabled) goto noProcess;
	if (!x->connected[0]) {
		for( i=0; i<n; i++) for( hp = 0; hp < x->Nout; hp++) out[hp][i] = 0;/**/
		goto noProcess;
	}
	
	/******************************************************************/
	/*  Traitements  **************************************************/
	
	if (x->connected[1]) x->c1 = xp[sampleframes-1];
	if (x->connected[2]) x->c2 = yp[sampleframes-1];
	
	/******************************************************************/
	/*Si entrees signal mutées, on utilise le tableau P de contrôle.***/
	if(x->mute || (!x->connected[1] && !x->connected[2])) 
		for( i=0; i<n; i++)
		 {
			//on stocke l'échantillon d'entrée dans une variable temporaire
			//car son_in = out[0].
			son = son_in[i];
			
			//Modulation des sorties avec les coefficients ambisoniques Pn.
			for( hp = N-1; hp >= 0; hp--)
			 {
				//Incrémentation des P pour l'interpolation,
				if( x->P[hp] == x->Pstop[hp] ) /*rien*/  ;
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
			
			//Conversion polaires -> cartésiennes,
			if(!x->base)
			 {
				//ici xl = rayon et yl = angle, 
				phii = (int)( yl*T_COS*I360 )&(int)MASQUE; 
				xtemp    = xl*x->cosin[ phii ];
				phii = (int)(.25*T_COS - phii)&(int)MASQUE;
				yl       = xl*x->cosin[ phii ];
				xl = xtemp;
				//maintenant xl = abscisse et yl = ordonnée.  
			 }
			
			//Calcul des distances,
			ds   = xl*xl + yl*yl;
			dist = (t_double)sqrt(ds);
			
			//Calcul des paramètres ambisoniques,
			X = (t_double)( 2*xl / (ds + offset) ); 
			Y = (t_double)( 2*yl / (ds + offset) ); 
			W = (t_double)( .707 / (dist + offset) );
			
			for( hp=N-1; hp >= 0 ; hp--)
			 {
				P = K  * ( W + X*x->cos_teta[hp]  
						  + Y*x->sin_teta[hp]  )
				* x->dist[hp];
				
				//Si Pn<0 on les force à 0
				if(P < 0)            P = 0;
				
				/***********************/
				out[hp][i] = son * P;/**/
				/***********************/
			 }
		 }
	
	
	/*Initialisation à zéro des sorties inutilisées*/
	if( x->Nout > x->N){
		for( hp = x->Nout-1 ; hp >= N ; hp--){
			for( i=n-1 ; i>=0; i--)
				out[hp][i] = 0;
		}
	}
	
	noProcess :
	return;
}


/**********************************************************************/
/*        METHODE RECOIT_float                                        */
/**********************************************************************/

void  ambipan_tilde_recoit_float(t_ambipan_tilde *x, double data)
{
	int inlet = proxy_getinlet((t_object *)x);
	switch (inlet) {
		case 1:
			if (!x->connected[1]) x->c1 = data;
			ambipan_tilde_recoit_x(x, data);
			break;
		case 2:
			if (!x->connected[2]) x->c2 = data;
			ambipan_tilde_recoit_y(x, data);
			break;
		default:
			break;
	}
}


/**********************************************************************/
/*        METHODE RECOIT_x                                            */
/**********************************************************************/
//Cette méthode reçoit x et change tous les paramètres ambisoniques: 
//  X, Y, W et les Pn 
void ambipan_tilde_recoit_x(t_ambipan_tilde *x, double xp)
{
  int hp;
  float ds, dist;
  float X, Y, W;
  float yp = x->y;
  float K = (float)(sqrt(1/(float)x->N)/1.66 ); 


  //Conversion polaire -> cartésienne,
  if( !x->base )
  {
    //ici xp = rayon,
    x->r = xp;
    xp   = (float)( x->r * cos( x->phi ) );
    yp   = (float)( x->r * sin( x->phi ) );
    //maintenant xp = abscisse,
  }
  x->x = xp;
  x->y = yp;

  //Calcul des distances,
  ds = xp*xp + yp*yp;
  dist = (float)sqrt(ds);

  //Calcul des paramètres ambisoniques,
  X = (float)( 2*xp/(ds + x->offset) ); 
  Y = (float)( 2*yp/(ds + x->offset) ); 
  W = (float)( .707/(dist + x->offset) );

  //Calcul des coefficients ambisoniques cibles et des pas,
  for( hp = x->N-1; hp >= 0 ; hp--)
  {
    x->Pstop[hp] = (float)( ( W + X*cos(x->teta[hp]) 
                                + Y*sin(x->teta[hp])
                            ) * K  * x->dist[hp]);
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
//  X, Y, W et les Pn 
void ambipan_tilde_recoit_y(t_ambipan_tilde *x, double yp)
{
  int hp;
  float ds, dist;
  float X, Y, W;
  float xp = x->x;
  float K = (float)(sqrt(1/(float)x->N)/1.66 ); 

  
  //Conversion polaires -> cartésienne,
  if( !x->base )
  {
    //ici yp = angle,
    x->phi = (float)( yp*Pi/180 );
    xp   = (float)( x->r * cos( x->phi ) );
    yp   = (float)( x->r * sin( x->phi ) );
    //maintenant xp = ordonnée,
  }
  x->y = yp;
  x->x = xp;

  //Calcul des distances,
  ds = xp*xp + yp*yp;
  dist = (float)sqrt(ds);

  //Calcul des paramètres ambisoniques,
  X = (float)( 2*xp/(ds + x->offset) ); 
  Y = (float)( 2*yp/(ds + x->offset) ); 
  W = (float)( .707/(dist + x->offset) );

  //Calcul des coefficients ambisoniques cibles et des pas,
  for( hp=x->N-1; hp >= 0 ; hp--)
  {
    x->Pstop[hp] = (float)( ( W + X*cos(x->teta[hp]) 
                                + Y*sin(x->teta[hp])
                            ) * K  * x->dist[hp]);
    //Si Pstop_n<0 on les force ‡ 0
    if(x->Pstop[hp] < 0)
      x->Pstop[hp] = 0;

    x->dP[hp] = (x->Pstop[hp] - x->P[hp])/(float)x->dtime;    
  }

  return;
}



/**********************************************************************/
/*        METHODE RECOIT_LISTE                                        */
/**********************************************************************/

void ambipan_tilde_recoit_liste( t_ambipan_tilde *x, t_symbol *s, int argc, t_atom *argv)
{
    float xp = 0.f;
    float yp = 0.f;
  
  //Récupération du premier paramètre (abscisse ou rayon),
  if( atom_gettype(argv) == A_FLOAT || atom_gettype(argv) == A_LONG)
      xp = atom_getfloat(argv);
    
  //Récupération du second paramètre (ordonnée ou angle),
  if( argc >= 2 )
  {
    if( atom_gettype(argv+1) == A_FLOAT || atom_gettype(argv+1) == A_LONG )
      yp = atom_getfloat(argv+1);
  } 
  //si qu'un paramËtre, on prend la valeur dÈja prÈsente.
  else if( x->base )     yp = x->y;
  else                   yp = x->phi*180/Pi;
  
  //Initialisation des valeurs,
  if( x->base )    x->y = yp;
  else             x->phi = yp*Pi/180;

  ambipan_tilde_recoit_x( x, xp);
}



/**********************************************************************/
/*      METHODE POSITIONNER HAUT-PARLEUR sur le cercle avec phi       */
/**********************************************************************/

void ambipan_tilde_teta_positionner_hp( t_ambipan_tilde *x, t_symbol *s, int argc, t_atom *argv )
{
  int hp;

  for( hp=0; hp<x->N && hp<argc; hp++)
  {
    //Récupération des angles.
    if( argv[hp].a_type == A_FLOAT )
      x->teta[hp] = ((argv[hp].a_w.w_float)*Pi/180.);
    else if( argv[hp].a_type == A_LONG )
      x->teta[hp] = (float)((argv[hp].a_w.w_long)*Pi/180.);
    else if( argv[hp].a_type == A_SYM ){
      object_error((t_object *)x, "les membres de la liste doivent etre des nombres.");
      return;
    }
    
    x->dist[hp] = 1;
   
    //Précalcul des cos et sin en signal. 
	 x->cos_teta[hp] = (float)cos( x->teta[hp] );
	 x->sin_teta[hp] = (float)sin( x->teta[hp] );
  }
  
  /*Affectation des gains de l'ambisonie*/
    if(x->base)     ambipan_tilde_recoit_x( x, x->x);
    else            ambipan_tilde_recoit_x( x, x->r);

  return;
}



/**********************************************************************/
/*      METHODE POSITIONNER HAUT-PARLEUR  avec phi et le rayon        */
/**********************************************************************/

void ambipan_tilde_dist_teta_positionner_hp( t_ambipan_tilde *x, t_symbol *s, int argc, t_atom *argv )
{
  int hp;

  for( hp=0; hp<x->N && 2*hp<argc; hp++)
  {
    //Récupération des distances des haut-praleurs au centre,
    if( argv[2*hp].a_type == A_FLOAT )
      x->dist[hp] = (argv[2*hp].a_w.w_float);
    else if( argv[2*hp].a_type == A_LONG )
      x->dist[hp] = (float)(argv[2*hp].a_w.w_long);
    else if( argv[2*hp].a_type == A_SYM ){
      object_error((t_object *)x, "members of the list must be float or int");
      return;
    }

    //Récupération des angles des hp,
    if( 2*hp+1<argc )
    {
      if( argv[2*hp+1].a_type == A_FLOAT )
        x->teta[hp] = ((argv[2*hp+1].a_w.w_float)*Pi/180);
      else if( argv[2*hp+1].a_type == A_LONG )
        x->teta[hp] = (float)((argv[2*hp+1].a_w.w_long)*Pi/180);
      else if( argv[2*hp+1].a_type == A_SYM ){
        object_error((t_object *)x, "members of the list must be float or int");
        return;
      }
    }
   
    //Précalcul des cos et sin en signal.
	 x->cos_teta[hp] = (float)cos( x->teta[hp] );
	 x->sin_teta[hp] = (float)sin( x->teta[hp] );
  }
    
  /*Affectation des gains de l'ambisonie*/
    if(x->base)     ambipan_tilde_recoit_x( x, x->x);
    else            ambipan_tilde_recoit_x( x, x->r);

  return;
}



/**********************************************************************/
/*      METHODE POSITIONNER HAUT-PARLEUR  avec coordonnées x et y     */
/**********************************************************************/

void ambipan_tilde_xy_positionner_hp( t_ambipan_tilde *x, t_symbol *s, int argc, t_atom *argv )
{
  int hp;
  float xp, yp = 0,
        rayon, angle;

  /*Récupération des coordonnées cartésiennes**********/
  //Abscisses:
  for( hp=0; hp<x->N && 2*hp<argc; hp++)
  {
    if( argv[2*hp].a_type == A_FLOAT )
      xp = (argv[2*hp].a_w.w_float);
    else if( argv[2*hp].a_type == A_LONG )
      xp = (float)(argv[2*hp].a_w.w_long);
    else if( argv[2*hp].a_type == A_SYM ){
      object_error((t_object *)x, "members of the list must be float or int");
      return;
    }

    //Ordonnées:
    if( 2*hp+1<argc )
    {
      if( argv[2*hp+1].a_type == A_FLOAT )
        yp = (argv[2*hp+1].a_w.w_float);
      else if( argv[2*hp+1].a_type == A_LONG )
        yp = (float)(argv[2*hp+1].a_w.w_long);
      else if( argv[2*hp+1].a_type == A_SYM ){
        object_error((t_object *)x, "members of the list must be float or int");
        return;
      }
    }


    /*Conversion en polaires***************************/  
    rayon = (float)sqrt( pow( xp, 2) + pow( yp, 2) );
    if( rayon!=0 )
    {
      angle = (float)acos( (float)xp/rayon );
      if(yp<0)
        angle = -angle;
    }
    else
      angle = 0;

    //Affectatoin des nouvelles valeurs
    x->teta[hp] = angle;
    x->dist[hp] = rayon;  
   
    /*Précalcul des cos et des sin ********************/
      x->cos_teta[hp] = (float)(cos( angle ));
      x->sin_teta[hp] = (float)(sin( angle ));
  }
    
  /*Affectation des gains de l'ambisonie*/
    if(x->base)     ambipan_tilde_recoit_x( x, x->x);
    else            ambipan_tilde_recoit_x( x, x->r);
    
  return;
}



/**********************************************************************/
/*              METHODE MODIFIER LE NOMBRE DE HP                      */
/**********************************************************************/

void ambipan_tilde_initialiser_nb_hp( t_ambipan_tilde *x, long N)
{
  int hp;


  if( x->Nout >= N && 2 <= N )
  {
    x->N = (int)N;
    
    //Modifications des angles des haut-parleurs et des rayons,
    for( hp=0; hp<x->N; hp++)
    {
      x->teta[hp] = (float)( Pi*( .5 + (1 - 2*hp )/(float)x->N) );
      x->dist[hp] = 1;
    }


    /*****************************************************/
    /*Modification des tableaux cos_teta et sin_teta     */
    /*pour l'audio.                                      */
	 //Précalculs des cos et sin des haut-parleurs.
	 for( hp=0; hp<x->N; hp++)
      {
		 x->cos_teta[hp] = (float)cos( x->teta[hp] );
		 x->sin_teta[hp] = (float)sin( x->teta[hp] );
      }
  }  
  else
      object_error((t_object *)x, "The number of loudspeakers must be between 2 and %d", x->Nout);
    
  /*Affectation des gains de l'ambisonie*/
    if(x->base)     ambipan_tilde_recoit_x( x, x->x);
    else            ambipan_tilde_recoit_x( x, x->r);
   
}

/**********************************************************************/
/*              METHODE POUR MUTER LES ENTREES SIGNAL                 */
/**********************************************************************/

void  ambipan_tilde_muter_entrees_signal(t_ambipan_tilde *x, int mute)
{
  if( mute == 0)   x->mute = 0;
  else             x->mute = 1;
}



/**********************************************************************/
/*                   MODIFICATION DE L'OFFSET                         */
/**********************************************************************/

void ambipan_tilde_changer_offset( t_ambipan_tilde *x, double val)
{    
  if( val <= EPSILON ){
    object_error((t_object *)x, "No negative offset please");
    return;
  }

  x->offset = val; //Changement de l'offset
  
  /*Affectation des gains de l'ambisonie*/
    if(x->base)     ambipan_tilde_recoit_x( x, x->x);
    else            ambipan_tilde_recoit_x( x, x->r);
}



/**********************************************************************/
/*                   MODIFICATION DU TYPE DE REPERE                   */
/**********************************************************************/

void ambipan_tilde_changer_type_repere( t_ambipan_tilde *x, t_symbol *sym)
{
  //Changement de x->base
  if( sym->s_name[0] == 'c')
    x->base = 1;
  else if( sym->s_name[0] == 'p')
    x->base = 0; 
  else 
    object_error((t_object *)x, "type de repere inconnu.");
    
  //Si coordonnées polaires il faut initialiser x->r et x->phi:
  if( x->base == 0 )
  {
    //Conversion de x et y en polaires:  
    x->r = (float)sqrt( pow( x->x, 2) + pow( x->y, 2) );
    if( x->r!=0 )
    {
      x->phi = (float)acos( (float)x->x/x->r );
      if(x->y<0)
        x->phi = -x->phi;
    }
    else
      x->phi = 0;
  }  
}



/**********************************************************************/
/*         AFFICHAGE DES INFORMATIONS DANS LA FENETRE DE MAX          */
/**********************************************************************/

void ambipan_tilde_informations( t_ambipan_tilde *x)
{
	int hp;
	object_post((t_object *)x, "Info Ambipan~ : ");
	
	if(x->base) post("   coordonnees cartesiennes,");
	else post("   coordonnees polaires,");
    
	if(!x->mute)	post("   avec entrees signal actives,");
	else post("   avec entrees signal inactives,");
	
	post("   offset                 = %f,", x->offset);
	post("   temps d'interpolation (pour le controle)  = %d ms,", (int)(x->dtime*1000/sys_getsr()));
	post("   nombre de haut-parleurs = %d," , x->N);
	post("   position des haut-parleurs:");
	for( hp=0; hp<x->N-1; hp++)
		post("      hp %d: %f.x + %f.y,", hp+1, x->dist[hp]*cos(x->teta[hp]),
			 x->dist[hp]*sin(x->teta[hp]));
    
	post("      hp n %d: %f.x + %f.y.", hp+1, x->dist[hp]*cos(x->teta[hp]),
		 x->dist[hp]*sin(x->teta[hp]));
	post("      ***");
}

//--------------- Assistance Methode ---------------------//

void ambipan_tilde_assist(t_ambipan_tilde *x, void *b, long m, long a, char *s)
{
	if (m == ASSIST_INLET) {
		switch (a) {
			case 0:
				sprintf(s,"(Signal) Mono Source input");
				break;
			case 1:
				if (x->base) sprintf(s,"(Signal/float) Abscissa of the Source");
				else sprintf(s,"(Signal/float) Distance of the Source");
				break;
			case 2:
				if (x->base) sprintf(s,"(Signal/float) Ordinate of the Source");
				else sprintf(s,"(Signal/float) Angle of the Source");
				break;
		}
	}
	else
		sprintf(s,"(Signal) Out %ld", (a+1));
}
    


/**********************************************************************/
/*                       FONCTION DESTRUCTION                         */
/**********************************************************************/

void ambipan_tilde_dest(t_ambipan_tilde *x)
{
	dsp_free((t_pxobject *)x);
    freebytes( (void*)(x->cos_teta), (short)(2*x->N*sizeof(float)) ); //Libération de la mémoire allouée pour les cosinus//
	return;
}

/**********************************************************************/
/*                       FONCTION CREATION                            */
/**********************************************************************/

void *ambipan_tilde_new(t_symbol *s, int argc, t_atom *argv )
{
	int    i, hp, newVersion;
	char   car = 'c';
	
	
	//--------- Allocation de la dataspace ---------------------------//
	
	t_ambipan_tilde *x = (t_ambipan_tilde*)object_alloc(ambipan_tilde_class);
	
	/*********************************************************/
	/*Récupération du nombre de sorties et de haut-parleurs. */
	if( argc >= 1 ){
		if( argv[0].a_type == A_LONG )
			x->N = argv[0].a_w.w_long;
		else x->N = 0;               
	}
	else 
		x->N = Ndefaut;
	
	if( Nmax < x->N || 2 > x->N ){
		x->N = Ndefaut;
		object_error((t_object *)x, "Il y a un probleme dans la declaration du nombre de haut-parleur, il est de %d par defaut.", Ndefaut);
	}
	x->Nout = x->N;  //MÍme nombre de sorties et d'haut-parleur.
	
	/*Récupération du type repère ************************/
	if( argc >=2 ){
		if( argv[1].a_type == A_SYM )
			car = (char)(atom_getsym(argv+1)->s_name[0]);
		
		if( car == 'c' )
			x->base=1;
		else if( car == 'p' )
			x->base=0;
		else {
			x->base=1;
			object_error((t_object *)x, "erreur dans le type des coordonnees, elles sont cartesiennes par defaut.");
		}
	}
	else 
		x->base = 1; //CartÈsienne par dÈfaut.
    
	/*Récupération du type des entrées (pour des questions de compatibilité avec l'ancienne version) *******************/
	if( argc >=3 && argv[2].a_type == A_SYM){
		object_error((t_object *)x, "l'argument signal/controle est sans effet dans cette version.");
		newVersion = 0;
	} else newVersion = 1;
	
	if (!newVersion) { // si ancienne version : arg 4 = offset, arg 5 = interp time :
		/*Récupération de l'offset ***************************/
		if( argc >= 4 && (atom_gettype(argv+3) == A_FLOAT || atom_gettype(argv+3) == A_LONG) )
			x->offset = fabs(atom_getfloat(argv+3));
		else
			x->offset = (float)OFFSET;
		if( x->offset <= EPSILON ){
			object_error((t_object *)x, "Pas d'offset negatif ou nul s'il vous plait.");
			x->offset = (float)OFFSET;;
		}
		
		
		/*Récupération du temps d'interpolation **************/
		if( argc >= 5 && argv[4].a_type == A_LONG )
			x->dtime = (int)(argv[4].a_w.w_long*sys_getsr()/1000.);
		else
			x->dtime = DTIME*sys_getsr()/1000.;
		if( x->dtime <= 0 ) //Pas de temps d'interpolation nul ou nÈgatif.
			x->dtime = 1;
	}
	else { // si ancienne version : arg 3 = offset, arg 4 = interp time :
		/*Récupération de l'offset ***************************/
        if( argc >= 3 && (atom_gettype(argv+2) == A_FLOAT || atom_gettype(argv+2) == A_LONG) )
            x->offset = fabs(atom_getfloat(argv+2));
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
			x->dtime = DTIME*sys_getsr()/1000.;
		if( x->dtime <= 0 ) //Pas de temps d'interpolation nul ou negatif.
			x->dtime = 1;
	}

	
	/*********************************************************/
	/*Initialisation des données de la classe. ***************/
	for( hp = x->N-1; hp >= 0; hp--) //Initialisation des P
		x->P[hp] = 0;
	
	x->mute= 0;             //entrées signal non mutées
	x->y   = Ydefaut;       //initialisation de y
	x->phi = Phidefaut;     //initialisation de phi
	
	//Initialisation des angles des haut-parleurs et des rayons,
	for( hp=0; hp<x->N; hp++)
	 {
		x->teta[hp] = (float)( Pi*( .5 + (1 - 2*hp )/(float)x->N) );
		x->dist[hp] = 1;
	 }
	
	/*********************************************************/
	/* Création des tableaux cosinus, cos_teta et sin_teta.  */
	/* pour l'audio.                                         */
	x->cos_teta = (float*)getbytes( (short)((T_COS+2*x->N)*sizeof(float)) );
	x->sin_teta = x->cos_teta+x->N;
	
	//Précalculs des cos et sin des haut-parleurs.
	for( hp=0; hp<x->N; hp++){
		x->cos_teta[hp] = (float)cos( x->teta[hp] );
		x->sin_teta[hp] = (float)sin( x->teta[hp] );
	 }
	
	//Remplissage du tableau cosinus,
	x->cosin = x->sin_teta+x->N;
	/*Pour avoir besoin de cos( phi ), on cherche:
	 cos(phi) = 
	 cosin[ (int)( phi*T_COS/360 ))&(((int)T_COS-1) ] */
	for( i=0; i<T_COS; i++) x->cosin[i] = (float)cos( i*2*Pi/T_COS );
	
	/*********************************************************/
	//initialisation de x, X, Y, W et Pn par la méthode recoit x.
	if(x->base) ambipan_tilde_recoit_x( x, Xdefaut);
	else        ambipan_tilde_recoit_x( x, Rdefaut);     
	
	/*********************************************************/
	/*Création des nouvelles entrées. ************************/
	dsp_setup((t_pxobject *)x, 3);
	
	/*Création des sorties************************************/
	for(i=0; i < x->N; i++) outlet_new((t_object *)x, "signal");
	
	return (void *)x;
}

/**********************************************************************/
/*                    DEFINITION DE LA CLASSE	                      */
/**********************************************************************/

void ext_main(void *r)
{
	
	t_class *c = class_new("ambipan~", 
						   (method)ambipan_tilde_new, 
						   (method)ambipan_tilde_dest, 
						   (short)sizeof(t_ambipan_tilde), NULL, A_GIMME, 0);

	//-------------Définition des méthodes-------------------//
	class_addmethod(c, (method)ambipan_tilde_dsp64, "dsp64", A_CANT, 0);
	class_addmethod(c, (method)ambipan_tilde_recoit_liste, "list", A_GIMME, 0);
	class_addmethod(c, (method)ambipan_tilde_recoit_float, "float", A_FLOAT, 0);
	class_addmethod(c, (method)ambipan_tilde_initialiser_nb_hp, "set_nb_hp", A_LONG, 0);
	class_addmethod(c, (method)ambipan_tilde_teta_positionner_hp, "a_setpos", A_GIMME, 0);
	class_addmethod(c, (method)ambipan_tilde_dist_teta_positionner_hp, "ra_setpos", A_GIMME, 0);
	class_addmethod(c, (method)ambipan_tilde_xy_positionner_hp, "xy_setpos", A_GIMME, 0);
	class_addmethod(c, (method)ambipan_tilde_changer_offset, "set_offset", A_FLOAT, 0);
	class_addmethod(c, (method)ambipan_tilde_muter_entrees_signal, "mute_sig", A_LONG, 0);
	class_addmethod(c, (method)ambipan_tilde_changer_type_repere, "change_type", A_SYM, 0);
	class_addmethod(c, (method)ambipan_tilde_informations, "get_info", 0);
	class_addmethod(c, (method)ambipan_tilde_assist,"assist",A_CANT,0);	
	
    cicmtools_post_credits();
	
	class_dspinit(c);
	class_register(CLASS_BOX, c);
    ambipan_tilde_class = c;
}

