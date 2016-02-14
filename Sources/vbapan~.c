/**********************************************************************/
//                                                                    //
// /****************************************************************/ //
// /*                                                              */ //
// /*                         VBAPAN                               */ //
// /*                                                              */ //
// /* Auteur: Rémi MIGNOT                                          */ //
// /*         Elève ingénieur Télécom2 à l'ISPG                    */ //
// /*         (Institut Supérieur Polytechnique de Galilée),       */ //
// /*         Université Paris13.                                  */ //
// /*                                                              */ //
// /* Date de creation:   21/07/03                                 */ //
// /* Version: 1.5        25/07/04                                 */ //
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
                à l'aide de N haut-parleurs situés autour
                de l'auditeur. La spatialisation se fait grâce à
                au système VBAP (Vector Base Amplitude Panning).      */


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
#define   Ndefaut   4      //Number of loudspeakers par défaut,
#define   Xdefaut   0      //Valeur par défaut de l'abscisse,
#define   Ydefaut   1      //Valeur par défaut de l'ordonnée,
#define   Phidefaut 1.5708F//Valeur par défaut de l'angle,
#define   Rdefaut   1      //Distance par défaut de la source,
#define   Rdisq     .5     //Rayon du disque central par défaut,
#define   T_COS     4096   //Taille du tableau cosinus (puissance de 2),
#define   MASQUE    4095   //Masque pour le modulo = T_COS - 1,
#define   DTIME     10     //Temps d'interpolation par défaut en ms,
#define   Pi        3.1415926535897932384F  //   = Pi
#define   I360      0.0027777777777777778F  //   = 1/360
#define   EPSILON   0.000001 //CritËre pour tester si les flottants sont nuls




/**********************************************************/
/*         La classe                                      */
/**********************************************************/

t_class* vbapan_tilde_class;

typedef struct _vbapan_tilde
{  
  t_pxobject x_obj;       //Membre indispensable à Max,
	
  int	    base;         //Type de repère, 1->cartésienne, 0->polaire, 		
  int       mute;         //1->entrées signal mutées, 0->non mutées,
  int       Nout;         //Nombre de sorties,
  int       N;            //Number of loudspeakers.
  
  float 	x,   y;       //Coordonnées de la source en cartésien,
  float     phi, r;       //Coordonnées polaires de la source,
	float   c1, c2;       // coordonnées entrées dans l'inlet 1 et 2 (en contrôle ou signal);

  int       dtime;        //Temps d'interpolation en échantillons,
  float     r_c;          //Rayon du disque central,
 
  float		G[Nmax];      //Tableau contenant les N gains des hp,
  float     teta[Nmax];   //Angles au centre des haut-parleurs,
  float     x_hp[Nmax];   //Abscisse de chaque haut-parleur,
  float     y_hp[Nmax];   //Ordonnée des haut-parleurs,
  float     dst_hp[Nmax]; //distance des haut-parleurs,

  float     dG[Nmax];     //Pas pour l'interpolation,
  float     Gstop[Nmax];  //Cible pour l'interpolation,

  float     G0[Nmax];     //gains des hp pour la position centrale,
  float     L[Nmax][2][2];//Tableaux contenant toute les matrices
                          //inversées des configurations de 2 haut-parleurs,
  int       rev[Nmax];    //Tableau qui permet d'ordonner les hp 
                          //par ordre croissant des angles, 

  float     *cosin;       //Adresse des tableaux pour les cosinus,
	int     connected[3]; //inlets connectés?
} t_vbapan_tilde;


/**********************************************************************/
/*      Prototypage des fonctions et méthodes.     	                  */
/**********************************************************************/

void  vbapan_tilde_dsp64(t_vbapan_tilde *x, t_object *dsp64, short *count, double samplerate, long maxvectorsize, long flags);
void  vbapan_tilde_perform_signal64(t_vbapan_tilde *x, t_object *dsp64, double **ins, long numins, double **outs, long numouts, long sampleframes, long flags, void *userparam);
float vbapan_tilde_modulo( float anglep );
void  vbapan_tilde_recoit_x(t_vbapan_tilde *x, double xp);
void  vbapan_tilde_recoit_y(t_vbapan_tilde *x, double yp);
void  vbapan_tilde_recoit_liste( t_vbapan_tilde *x, t_symbol *s, int argc, t_atom *argv);
void  vbapan_tilde_initialiser_nb_hp( t_vbapan_tilde *x, long N);
void  vbapan_tilde_dist_teta_positionner_hp( t_vbapan_tilde *x, t_symbol *s, int argc, t_atom *argv );
void  vbapan_tilde_teta_positionner_hp( t_vbapan_tilde *x, t_symbol *s, int argc, t_atom *argv );
void  vbapan_tilde_xy_positionner_hp( t_vbapan_tilde *x, t_symbol *s, int argc, t_atom *argv );
void  vbapan_tilde_changer_type_repere(t_vbapan_tilde *x, t_symbol *sym);
void  vbapan_tilde_init_hp_mat(t_vbapan_tilde *x );
void  vbapan_tilde_changer_rayon_disq_centrale( t_vbapan_tilde *x, double val);
void  vbapan_tilde_muter_entrees_signal(t_vbapan_tilde *x, int mute);
void  vbapan_tilde_informations( t_vbapan_tilde *x );
void  vbapan_tilde_dest(t_vbapan_tilde *x);
void  *vbapan_tilde_new(t_symbol *s, int argc, t_atom *argv );
void  ambipan_tilde_assist(t_vbapan_tilde *x, void *b, long m, long a, char *s);
void  ambipan_tilde_recoit_float(t_vbapan_tilde *x, double data);


/**********************************************************************/
/*                       METHODE DSP                                  */
/**********************************************************************/

void vbapan_tilde_dsp64(t_vbapan_tilde *x, t_object *dsp64, short *count, double samplerate, long maxvectorsize, long flags)
{
    int i;
    for(i=0; i<3; i++) x->connected[i] = count[i];
    object_method(dsp64, gensym("dsp_add64"), x, vbapan_tilde_perform_signal64, 0, NULL);
}

//----------------------------------------------------------------------------------------------

void  vbapan_tilde_perform_signal64(t_vbapan_tilde *x, t_object *dsp64, double **ins, long numins, double **outs, long numouts, long sampleframes, long flags, void *userparam)
{
	/*Préparation des pointeurs****************************************/
	t_double     *son_in   = ins[0];       //son en entrée
	t_double     *xp       = ins[1];       //réception de x
	t_double     *yp       = ins[2];       //réception de y
	int          n         = sampleframes; //taille des blocs
	
	/*Déclarations des variables locales*/
	int   phii;              //angle en entier (de 0 à T_COS),
	t_double son;            //variable temporaire,
	int   hp;                //indice relatif au haut-parleur,
	int   i;                 //indice du n° de l'échantillon,
	int   N = x->N;          //Number of loudspeakers,
	t_double xl, yl, xtemp;  //coordonnéees cartésienne de la source,
	t_double g[Nmax];        //gains des haut-parleurs (en signal),
	t_double phi;            //angle de la source,
	t_double r;              //rayon de la source.
	t_double Puis, iAmp;     //puissance et amplitude des gains.
	
	
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
	
	if (x->connected[1]) x->c1 = xp[n-1];
	if (x->connected[2]) x->c2 = yp[n-1];
	
	/******************************************************************/
	/*Si entrees signal mutées, on utilise le tableau P de contrôle.***/
	if(x->mute || (!x->connected[1] && !x->connected[2])) 
		for( i=0; i<n; i++)
		 {
			//on stocke l'échantillon d'entrée dans une variable temporaire
			//car son_in = out[0].
			son = son_in[i];
			
			
			//Modulation des sorties avec les gains Pn.
			for( hp=N-1; hp >= 0; hp--)
			 {
				//Incrémentation des gains pour l'interpolation,
				if( x->G[hp] == x->Gstop[hp] ) /*rien*/  ;
				else if ( fabs(x->Gstop[hp] - x->G[hp]) > fabs(x->dG[hp]) )
					x->G[hp] += x->dG[hp];
				else
					x->G[hp] = x->Gstop[hp];
				
				/******************************/
				out[hp][i] = son * x->G[hp];/**/
				/******************************/
			 }
		 }
	
	/******************************************************************/
	/*Si entrées signal non mutées, on utilise les vecteurs de signal**/
	else 
		for( i=n-1; i>=0; i--)
		 {
			son = son_in[i];    //On stocke les échantillons d'entrée, dans des
			//xl = xp[i];         //variables, car elles vont être écrasées.
			//yl = yp[i];
			
			xl = x->connected[1] ? x->c1 = xp[i] : x->c1;
			yl = x->connected[2] ? x->c2 = yp[i] : x->c2;
			
			/*Conversion polaires -> cartésiennes,*******/
			if( !x->base )
			 {
				phi = vbapan_tilde_modulo(yl)*Pi/180;
				r = xl;
				
				//ici xl = rayon et yl = angle,
				phii = (int)( yl*T_COS*I360 )&(int)MASQUE; 
				xtemp    = xl*x->cosin[ phii ];
				phii = (int)(.25*T_COS - phii)&(int)MASQUE;
				yl       = xl*x->cosin[ phii ];
				xl = xtemp;
				//maintenant xl = abscisse et yl = ordonnée.
				
				//Si r est négatif on modifie les coordonnées,
				if( r<0 ){
					r = -r;
					phi = (phi<Pi)?(phi+Pi):(phi-Pi);
				}
				
			 }
			/*Conversion cartésienne -> polaire,**********/
			else
			 {
				r = sqrt( xl*xl+yl*yl );
				phi = acos( xl/r );
				if( yl < 0 )
					phi = 2*Pi - phi;
			 }
			
			
			//remise à zéro des gains:
			for( hp=0; hp<x->N; hp++)
				g[hp] = 0;
			
			//Recherche des hp encadrant le point:
			for( hp=0; hp<x->N-1; hp++){
				if( phi >= x->teta[x->rev[hp]] && phi < x->teta[x->rev[hp+1]] )
					break;
			}
			
			//Calcul du gain:
			g[x->rev[hp]]          = x->L[hp][0][0]*xl + x->L[hp][0][1]*yl;
			g[x->rev[(hp+1)%x->N]] = x->L[hp][1][0]*xl + x->L[hp][1][1]*yl; 
			
			//Puissance du gain (source et hp sur le cercle) :
			Puis =    ( g[x->rev[hp         ]]*g[x->rev[hp         ]] ) 
			+ ( g[x->rev[(hp+1)%x->N]]*g[x->rev[(hp+1)%x->N]] );
			iAmp = (Puis <= EPSILON) ? 0 : (1/(t_double)sqrt(2*Puis));
			
			
			if( r > x->r_c )
			 {
				//Normalisation des g, et prise en compte des distances :
				g[x->rev[hp]]            *=  iAmp * x->dst_hp[x->rev[ hp        ]] / r;
				g[x->rev[(hp+1)%x->N]]   *=  iAmp * x->dst_hp[x->rev[(hp+1)%x->N]] / r;
			 }
			else
			 {
				//Calcul du gain (sur le disque central):
				g[x->rev[hp]]            *=  iAmp * x->dst_hp[x->rev[ hp        ]] * r/(x->r_c*x->r_c);
				g[x->rev[(hp+1)%x->N]]   *=  iAmp * x->dst_hp[x->rev[(hp+1)%x->N]] * r/(x->r_c*x->r_c);
				//ici le gain est proportionnelle ‡ r/r_c dans le disque.
				
				//On mixe linÈairement l'effet VBAP avec les gains pour la position centrale:
				for( hp=0; hp<x->N; hp++)
					g[hp] += (1 - r/x->r_c) * x->G0[hp];
			 }
			
			for( hp=N-1; hp >= 0 ; hp--)
			 {
				/***************************/
				out[hp][i] = son * g[hp];/**/
				/***************************/
			 }
		 }
	
	
	/*Initialisation à zéro des sorties inutilisées*/
	for( hp = x->Nout-1 ; hp >= N ; hp--){
		for( i=n-1 ; i>=0; i--)
			out[hp][i] = 0;
	}
	
noProcess:
	return;
}

/**********************************************************************/
/*        METHODE RECOIT_float                                        */
/**********************************************************************/

void  vbapan_tilde_recoit_float(t_vbapan_tilde *x, double data)
{
	int inlet = proxy_getinlet((t_object *)x);
	switch (inlet) {
		case 1:
			if (!x->connected[1]) x->c1 = data;
			vbapan_tilde_recoit_x(x, data);
			break;
		case 2:
			if (!x->connected[2]) x->c2 = data;
			vbapan_tilde_recoit_y(x, data);
			break;
		default:
			break;
	}
}

/**********************************************************************/
/*        METHODE RECOIT_x                                            */
/**********************************************************************/
//Cette méthode reçoit x et change tous les gains : 

void vbapan_tilde_recoit_x(t_vbapan_tilde *x, double xp)
{
  float Puis, iAmp;
  int   hp;
  float yp  = x->y;
  float phi = x->phi;
  float r;

  //Conversion polaires -> cartésiennes,
  if( !x->base )
  {
    //ici xp = rayon,
    r  = xp;
    xp = (float)( r * cos( phi ) );
    yp = (float)( r * sin( phi ) );
    //maintenant xp = abscisse.  
  }
  //Conversion cartésienne -> polaire,
  else
  {
    r = sqrt( xp*xp+yp*yp );
    phi = acos( xp/r );
    if( yp < 0 )
      phi = 2*Pi - phi;   
  }  
  x->r   = r; 
  x->phi = phi;
  x->y   = yp;
  x->x   = xp;
  
  //Si r est négatif on modifie les coordonnées mais pas la dataspace,
  if( r<0 )
  {
    r = -r;
    phi = (phi<Pi)?(phi+Pi):(phi-Pi);
  }
  
  //Remise à zéro de tout les gains cibles:
  for(hp=0; hp<x->Nout; hp++)
    x->Gstop[hp] = 0;
    
 
  //Recherche des hp encadrant le point:
  for( hp=0; hp<x->N-1; hp++){
    if( phi >= x->teta[x->rev[hp]] && phi < x->teta[x->rev[hp+1]] ) 
      break;
  }


  //Calcul des gains :
  x->Gstop[x->rev[hp]]          = x->L[hp][0][0]*xp + x->L[hp][0][1]*yp;
  x->Gstop[x->rev[(hp+1)%x->N]] = x->L[hp][1][0]*xp + x->L[hp][1][1]*yp;

  //Puissance du gain (source et hp sur le cercle) :
  Puis =    ( x->Gstop[x->rev[hp         ]]*x->Gstop[x->rev[hp         ]] ) 
          + ( x->Gstop[x->rev[(hp+1)%x->N]]*x->Gstop[x->rev[(hp+1)%x->N]] );
  iAmp = (Puis <= EPSILON) ? 0 : (1/(float)sqrt(2*Puis));


  if( r > x->r_c )
  {
    //Normalisation des g, et prise en compte des distances :
    x->Gstop[x->rev[hp]]           *=  iAmp * x->dst_hp[x->rev[ hp        ]] / r;
    x->Gstop[x->rev[(hp+1)%x->N]]  *=  iAmp * x->dst_hp[x->rev[(hp+1)%x->N]] / r;
  }
  else
  {
    //Calcul du gain (sur le disque central):
    x->Gstop[x->rev[hp]]           *=  iAmp * x->dst_hp[x->rev[ hp        ]] * r/(x->r_c*x->r_c);
    x->Gstop[x->rev[(hp+1)%x->N]]  *=  iAmp * x->dst_hp[x->rev[(hp+1)%x->N]] * r/(x->r_c*x->r_c);
    //ici le gain est proportionnelle ‡ r/r_c dans le disque.
    
    //On mixe linÈairement l'effet VBAP avec les gains pour la position centrale:
    for( hp=0; hp<x->N; hp++)
      x->Gstop[hp] += (1 - r/x->r_c) * x->G0[hp];
  }
  
  // Interpolation :
  for( hp=0; hp<x->N; hp++)
  {
    x->dG[hp]          = ( x->Gstop[hp]          - x->G[hp]          ) /(float)x->dtime;
    x->dG[(hp+1)%x->N] = ( x->Gstop[(hp+1)%x->N] - x->G[(hp+1)%x->N] ) /(float)x->dtime;
  }

  return;
}



/**********************************************************************/
/*        METHODE RECOIT_y                                            */
/**********************************************************************/
//Cette méthode reçoit y et change tous les paramètres :

void vbapan_tilde_recoit_y(t_vbapan_tilde *x, double yp)
{
  float Puis, iAmp;
  int hp;
  float xp = x->x;
  float r  = x->r;
  float phi;
  
  //Conversion polaire -> cartésienne,
  if( !x->base )
  {
    //ici yp = angle,
    phi = (float)(vbapan_tilde_modulo(yp)*Pi/180.);
    xp  = (float)( r * cos( phi ) );
    yp  = (float)( r * sin( phi ) );
    //maintenant yp = ordonnée,
  }
  //Conversion cartésienne -> polaire,
  else
  {
    r = sqrt( xp*xp+yp*yp );
    phi = acos( xp/r );
    if( yp < 0 )
      phi = 2*Pi - phi;   
  }    
  x->y = yp;
  x->x = xp;
  x->r = r;
  x->phi = phi;

  //Si r est négatif on modifie les coordonnées mais pas la dataspace,
  if( r<0 )
  {
    r = -r;
    phi = (phi<Pi)?(phi+Pi):(phi-Pi);
  }
  
  //Remise à zéro de tout les gains cibles:
  for(hp=0; hp<x->Nout; hp++)
    x->Gstop[hp] = 0;
 

  //Recherche des hp encadrant le point:
  for( hp=0; hp<x->N-1; hp++){
    if( phi >= x->teta[x->rev[hp]] && phi < x->teta[x->rev[hp+1]] )
      break;
  }


  //Calcul des gains :
  x->Gstop[x->rev[hp]]          = x->L[hp][0][0]*xp + x->L[hp][0][1]*yp;
  x->Gstop[x->rev[(hp+1)%x->N]] = x->L[hp][1][0]*xp + x->L[hp][1][1]*yp;

  //Puissance du gain (source et hp sur le cercle) :
  Puis =    ( x->Gstop[x->rev[hp         ]]*x->Gstop[x->rev[hp         ]] ) 
          + ( x->Gstop[x->rev[(hp+1)%x->N]]*x->Gstop[x->rev[(hp+1)%x->N]] );
  iAmp = (Puis <= EPSILON) ? 0 : (1/(float)sqrt(2*Puis));


  if( r > x->r_c )
  {
    //Normalisation des g, et prise en compte des distances :
    x->Gstop[x->rev[hp]]           *=  iAmp * x->dst_hp[x->rev[ hp        ]] / r;
    x->Gstop[x->rev[(hp+1)%x->N]]  *=  iAmp * x->dst_hp[x->rev[(hp+1)%x->N]] / r;
  } 
  else
  {
    //Calcul du gain (sur le disque central):
    x->Gstop[x->rev[hp]]           *=  iAmp * x->dst_hp[x->rev[ hp        ]] * r/(x->r_c*x->r_c);
    x->Gstop[x->rev[(hp+1)%x->N]]  *=  iAmp * x->dst_hp[x->rev[(hp+1)%x->N]] * r/(x->r_c*x->r_c);
    //ici le gain est proportionnelle ‡ r/r_c dans le disque.
    
    //On mixe linÈairement l'effet VBAP avec les gains pour la position centrale:
    for( hp=0; hp<x->N; hp++)
      x->Gstop[hp] += (1 - r/x->r_c) * x->G0[hp];
  }
  
  // Interpolation :
  for( hp=0; hp<x->N; hp++)
  {
    x->dG[hp]          = ( x->Gstop[hp]          - x->G[hp]          ) /(float)x->dtime;
    x->dG[(hp+1)%x->N] = ( x->Gstop[(hp+1)%x->N] - x->G[(hp+1)%x->N] ) /(float)x->dtime;
  }

  return;
}



/**********************************************************************/
/*        METHODE RECOIT_LISTE                                        */
/**********************************************************************/

void vbapan_tilde_recoit_liste( t_vbapan_tilde *x, t_symbol *s, int argc, t_atom *argv)
{
  float xp = 0, yp = 0;
  
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
  else if( x->base )
    yp = x->y;
  else 
    yp = x->phi*180/Pi;
  
  //Initialisation des valeurs,
  if( x->base )    x->y = yp;
  else             x->phi = vbapan_tilde_modulo(yp)*Pi/180;

  vbapan_tilde_recoit_x( x, xp);

  return;
}



/**********************************************************************/
/*      METHODE POSITIONNER HAUT-PARLEUR sur le cercle avec phi       */
/**********************************************************************/

void vbapan_tilde_teta_positionner_hp( t_vbapan_tilde *x, t_symbol *s, int argc, t_atom *argv )
{
  int hp;

  for( hp=0; hp<x->N && hp<argc; hp++)
  {
    //Récupération des angles.
    if( argv[hp].a_type == A_FLOAT )
      x->teta[hp] = (vbapan_tilde_modulo(argv[hp].a_w.w_float)*Pi/180.);
    else if( argv[hp].a_type == A_LONG )
      x->teta[hp] = (vbapan_tilde_modulo((float)argv[hp].a_w.w_long)*Pi/180.);
    else if( argv[hp].a_type == A_SYM ){
      object_error((t_object *)x, "Members of the list must be float or int");
      return;
    }

    //Affectations des nouvelles coordonnées:
    x->x_hp[hp] = cos( x->teta[hp] );
    x->y_hp[hp] = sin( x->teta[hp] );

    //La distance sera initialisÈe dans "init_hp_mat()".
  }
  
  //Modification des matrices relatives aux hp:
  vbapan_tilde_init_hp_mat( x );
  
  /*Affectation des gains des hp*/
	if(x->base)     vbapan_tilde_recoit_x( x, x->x);
    else            vbapan_tilde_recoit_x( x, x->r);

  return;
}



/**********************************************************************/
/*      METHODE POSITIONNER HAUT-PARLEUR  avec phi et le rayon        */
/**********************************************************************/

void vbapan_tilde_dist_teta_positionner_hp( t_vbapan_tilde *x, t_symbol *s, int argc, t_atom *argv )
{
  int hp;
  float rayon;

  for( hp=0; hp<x->N && 2*hp<argc; hp++)
  {
    //Récupération des distances des haut-praleurs au centre,
    if( argv[2*hp].a_type == A_FLOAT )
      rayon = (argv[2*hp].a_w.w_float);
    else if( argv[2*hp].a_type == A_LONG )
      rayon = (float)(argv[2*hp].a_w.w_long);
    else if( argv[2*hp].a_type == A_SYM ){
      object_error((t_object *)x, "Members of the list must be float or int");
      return;
    }

    //Récupération des angles des hp,
    if( 2*hp+1<argc )
    {
      if( argv[2*hp+1].a_type == A_FLOAT )
        x->teta[hp] = (vbapan_tilde_modulo(argv[2*hp+1].a_w.w_float)*Pi/180);
      else if( argv[2*hp+1].a_type == A_LONG )
        x->teta[hp] = (vbapan_tilde_modulo((float)argv[2*hp+1].a_w.w_long)*Pi/180);
      else if( argv[2*hp+1].a_type == A_SYM ){
        object_error((t_object *)x, "Members of the list must be float or int");
        return;
      }
    }
   
    //Affectations des nouvelles coordonnées:
    x->x_hp[hp] = rayon*cos( x->teta[hp] );
    x->y_hp[hp] = rayon*sin( x->teta[hp] );
  }

  //Modification des matrices relatives aux hp:
  vbapan_tilde_init_hp_mat( x );


  /*Affectation des gains des hp*/
  if(x->base)     vbapan_tilde_recoit_x( x, x->x);
  else            vbapan_tilde_recoit_x( x, x->r);

  return;
}


/**********************************************************************/
/*      METHODE POSITIONNER HAUT-PARLEUR  avec coordonnées x et y     */
/**********************************************************************/

void vbapan_tilde_xy_positionner_hp( t_vbapan_tilde *x, t_symbol *s, int argc, t_atom *argv )
{
  int hp;
  float xp, yp = 0,
        rayon, angle;

  /*Récupération des coordonnées cartésiennes**********/
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

    if( 2*hp+1<argc )
    {
      if( argv[2*hp+1].a_type == A_FLOAT )
        yp = (argv[2*hp+1].a_w.w_float);
      else if( argv[2*hp+1].a_type == A_LONG )
        yp = (float)(argv[2*hp+1].a_w.w_long);
      else if( argv[2*hp+1].a_type == A_SYM ){
        object_error((t_object *)x, "Members of the list must be float or int");
        return;
      }
    }
    else
      yp = x->y_hp[hp];


    /*Conversion en polaires****************************/  
    rayon = (float)sqrt( pow( xp, 2) + pow( yp, 2) );
    if( rayon!=0 )
    {
      angle = (float)acos( (float)xp/rayon );
      if(yp<0)
        angle = 2*Pi - angle;
    }
    else
      angle = 0;

    //Affectation des nouvelles coordonnées:
    x->teta[hp] = angle;
    x->x_hp[hp] = xp;
    x->y_hp[hp] = yp;
  }

  //Modification des matrices relatives aux hp:
  vbapan_tilde_init_hp_mat( x );
    
  /*Affectation des gains des hp*/
  if(x->base)     vbapan_tilde_recoit_x( x, x->x);
  else            vbapan_tilde_recoit_x( x, x->r);
    
  return;
}





/**********************************************************************/
/*              METHODE MODIFIER LE NOMBRE DE HP                      */
/**********************************************************************/

void vbapan_tilde_initialiser_nb_hp( t_vbapan_tilde *x, long N)
{
  int hp;


  if( x->Nout >= N && 2 < N )
  {
    x->N = (int)N;
    
    //Modifications des angles des haut-parleurs et des rayons,
    for( hp=0; hp<x->N; hp++){
      x->teta[hp] = (float)( Pi*( .5 + (1 - 2*hp )/(float)x->N) );
      if( x->teta[hp] < 0)
        x->teta[hp] += 2*Pi;
      x->x_hp[hp] = cos( x->teta[hp] );
      x->y_hp[hp] = sin( x->teta[hp] );
    }

    //Modification des matrices relatives aux hp:
    vbapan_tilde_init_hp_mat( x );
  }  
  else
      object_error((t_object *)x, "The number of loudspeakers must be between 3 and %d", x->Nout);
    
  /*Affectation des gains */
  if(x->base)     vbapan_tilde_recoit_x( x, x->x);
  else            vbapan_tilde_recoit_x( x, x->r);
   
}



/**********************************************************************/
/*              METHODE POUR MUTER LES ENTREES SIGNAL                 */
/**********************************************************************/

void  vbapan_tilde_muter_entrees_signal(t_vbapan_tilde *x, int mute)
{
  if( mute == 0)   x->mute = 0;
  else             x->mute = 1;
}



/**********************************************************************/
/*                   MODIFICATION DU TYPE DE REPERE                   */
/**********************************************************************/

void vbapan_tilde_changer_type_repere(t_vbapan_tilde *x, t_symbol *sym)
{
  //Changement de x->base
  if( sym->s_name[0] == 'c')
    x->base = 1;
  else if( sym->s_name[0] == 'p')
    x->base = 0; 
  else 
    object_error((t_object *)x, "Unknown type of coordinates");
    
  //Si coordonnées polaires il faut initialiser x->r et x->phi:
  if( x->base == 0 )    //(Inutile en fait).
  {
    //Conversion de x et y en polaires:  
    x->r = (float)sqrt( pow( x->x, 2) + pow( x->y, 2) );
    if( x->r!=0 )
    {
      x->phi = (float)acos( (float)x->x/x->r );
      if(x->y<0)
        x->phi = 2*Pi - x->phi;
    }
    else
      x->phi = 0;
  }  
}




/**********************************************************************/
/*            METHODE CHANGER RAYON DU DISQUE CENTRAL                 */
/**********************************************************************/

void vbapan_tilde_changer_rayon_disq_centrale(t_vbapan_tilde *x, double val)
{    
  if(val <= EPSILON ){
    object_error((t_object *)x, "pas de rayon nul ou negatif.");
    return;
  }
  x->r_c = val;

  //Modification des matrices relatives aux hp:
  vbapan_tilde_init_hp_mat( x );
    
  /*Affectation des gains des hp*/
  if(x->base)     vbapan_tilde_recoit_x( x, x->x);
  else            vbapan_tilde_recoit_x( x, x->r);

  return;
}



/**********************************************************************/
/*         AFFICHAGE DES INFORMATIONS DANS LA FENETRE DE MAX          */
/**********************************************************************/

void vbapan_tilde_informations( t_vbapan_tilde *x)
{
  int hp;
  
  object_post((t_object *)x, "Info Vbapan~: ");
  
	if(x->base) post("   Cartesian coordinates,");
	else post("   Polar coordinates,");
    
	if(!x->mute)	post("   With active signal inlets,");
	else post("   With inactive signal inlets,");

  post("   Radius of the central disc = %f,", x->r_c);
  post("   Interpolation time (for control)  = %d ms,", (int)(x->dtime*1000/sys_getsr()));
  post("   Number of loudspeakers = %d," , x->N);
  
  post("   Position of the loudspeakers:");
  for( hp=0; hp<x->N-1; hp++)
    post("      hp %d: %f.x + %f.y,", hp+1, x->x_hp[hp], x->y_hp[hp] );
    
  post("      hp %d: %f.x + %f.y.", hp+1, x->x_hp[hp], x->y_hp[hp] );
	post("      ***");
}
    
//--------------- Assistance Methode ---------------------//

void vbapan_tilde_assist(t_vbapan_tilde *x, void *b, long m, long a, char *s)
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

void vbapan_tilde_dest(t_vbapan_tilde *x)
{
	dsp_free((t_pxobject *)x);
	freebytes( x->cosin, (short)(T_COS*sizeof(float)) ); //Libération de la mémoire allouée pour les cosinus//
	return;
}


/**********************************************************************/
/*                       FONCTION CREATION                            */
/**********************************************************************/

void *vbapan_tilde_new( t_symbol *s, int argc, t_atom *argv )
{
	int    i, hp, newVersion;
	char   car = 'c';
	
	/*Allocation de la dataspace ****************************/
	t_vbapan_tilde *x = (t_vbapan_tilde *)object_alloc(vbapan_tilde_class);
	
	/********************************************************/
	/*Récupération du nombre de sorties et de haut-parleurs */
	if( argc >= 1 ){
		if( argv[0].a_type == A_LONG )
			x->N = argv[0].a_w.w_long;
		else x->N = 0;               }
	else 
		x->N = Ndefaut;
	
	if( x->N <= 2 || x->N > Nmax ){
		x->N = Ndefaut;
		object_error((t_object *)x, "Bad number of loudspeaker setup, %d by default", Ndefaut);
	}
	x->Nout = x->N;  //Même nombre de sorties et d'haut-parleur.
	
	
	/*Récupération du type repère ****************************/
	if( argc >=2 ){
		if( argv[1].a_type == A_SYM )
			car = (char)(argv[1].a_w.w_sym->s_name[0]);
		
		if( car == 'c' )
			x->base=1;
		else if( car == 'p' )
			x->base=0;
		else {
			x->base=1;
			object_error((t_object *)x, "Unknown type of coordinates, cartesian by default");
		}
	}
	else 
		x->base = 1;
	
    
	/*Récupération du type des entrées (pour des questions de compatibilité avec l'ancienne version) *******************/
	if( argc >=3 && argv[2].a_type == A_SYM){
		object_error((t_object *)x, "The signal/control argument has no effect in this version");
		newVersion = 0;
	} else newVersion = 1;
	
	
	if (!newVersion) { // si ancienne version : arg 4 = offset, arg 5 = interp time :
		/*Récupération du rayon du disque central **************/
        if( argc >= 4 && (atom_gettype(argv+3) == A_FLOAT || atom_gettype(argv+3) == A_LONG) )
            x->r_c = fabs(atom_getfloat(argv+3));
		else
			x->r_c = (float)Rdisq;
		if( x->r_c <= EPSILON ){
			object_error((t_object *)x, "Pas de rayon de disque centrale nul s'il vous plait.");
			x->r_c = (float)Rdisq;
		}
		
		/*Récupération du temps d'interpolation ******************/
		if( argc >= 5 && argv[4].a_type == A_LONG )
			x->dtime = (int)(argv[4].a_w.w_long*sys_getsr()/1000.);
		else
			x->dtime = (int)(DTIME*sys_getsr()/1000.);
		if( x->dtime <= 0 )
			x->dtime = 1;
	}
	else {
		/*Récupération du rayon du disque central **************/
        if( argc >= 3 && (atom_gettype(argv+2) == A_FLOAT || atom_gettype(argv+2) == A_LONG) )
            x->r_c = fabs(atom_getfloat(argv+2));
		else
			x->r_c = (float)Rdisq;
		if( x->r_c <= EPSILON ){
			object_error((t_object *)x, "Pas de rayon de disque centrale nul s'il vous plait.");
			x->r_c = (float)Rdisq;
		}
		
		/*Récupération du temps d'interpolation ******************/
		if( argc >= 4 && argv[3].a_type == A_LONG )
			x->dtime = (int)(argv[3].a_w.w_long*sys_getsr()/1000.);
		else
			x->dtime = (int)(DTIME*sys_getsr()/1000.);
		if( x->dtime <= 0 )
			x->dtime = 1;
	}

	/*********************************************************/
	/*Initialisation des données de la classe*****************/
	for( hp = x->N-1; hp >= 0; hp--) //Initialisation des P
		x->G[hp] = 0;
	
	x->mute= 0;             //entrées signal non mutées
	x->y   = Ydefaut;       //initialisation de y
	x->phi = Phidefaut;     //initialisation de phi
	
	//Initialisation des angles des haut-parleurs et des rayons,
	for( hp=0; hp<x->N; hp++)
	 {
		x->teta[hp] = (float)( Pi*( .5 + (1 - 2*hp )/(float)x->N) );
		if( x->teta[hp] < 0)
			x->teta[hp] += 2*Pi;
		x->x_hp[hp] = cos( x->teta[hp]);
		x->y_hp[hp] = sin( x->teta[hp]);
	 }
	
	//Initialisation de tout les tableaux relatifs aux hp:
	vbapan_tilde_init_hp_mat( x );
	
	//initialisation de x, X, Y, W et Pn par la méthode recoit x.
	if( x->base )    vbapan_tilde_recoit_x( x, Xdefaut);
	else             vbapan_tilde_recoit_x( x, Rdefaut);     
	
	
	/*********************************************************/
	/*Création du tableaux cosinus                           */
	
	x->cosin = (float*)getbytes( (short)(T_COS*sizeof(float)) );
	
	//Remplissage du tableau cosinus,
	for( i=0; i<T_COS; i++)
		x->cosin[i] = (float)cos( i*2*Pi/T_COS );
	
	/*Pour avoir besoin de cos( phi ), on cherche:
	 cos(phi) = 
	 cosin[ (int)( phi*T_COS/360 ))&(((int)T_COS-1) ] */
	
	/*********************************************************/
	/*Création des nouvelles entrées. ************************/
	dsp_setup((t_pxobject *)x, 3);
    
	/*Création des sorties************************************/
	for( i=0; i < x->N; i++) outlet_new((t_object *)x, "signal");
	
	return (void *)x;
}



/**********************************************************************/
/*                    DEFINITION DE LA CLASSE	                        */
/**********************************************************************/

void ext_main(void *r)
{
	
	t_class *c = class_new("vbapan~", 
						   (method)vbapan_tilde_new, 
						   (method)vbapan_tilde_dest, 
						   (short)sizeof(t_vbapan_tilde), NULL, A_GIMME, 0);
	
	//-------------Définition des méthodes-------------------//
	class_addmethod(c, (method)vbapan_tilde_dsp64, "dsp64", A_CANT, 0);
	class_addmethod(c, (method)vbapan_tilde_recoit_liste, "list", A_GIMME, 0);
	class_addmethod(c, (method)vbapan_tilde_recoit_float, "float", A_FLOAT, 0);
	class_addmethod(c, (method)vbapan_tilde_initialiser_nb_hp, "set_nb_hp", A_LONG, 0);
	class_addmethod(c, (method)vbapan_tilde_muter_entrees_signal, "mute_sig", A_LONG, 0);
	class_addmethod(c, (method)vbapan_tilde_changer_type_repere, "change_type", A_SYM, 0);
	class_addmethod(c, (method)vbapan_tilde_teta_positionner_hp, "a_setpos", A_GIMME, 0);
	class_addmethod(c, (method)vbapan_tilde_xy_positionner_hp, "xy_setpos", A_GIMME, 0);
	class_addmethod(c, (method)vbapan_tilde_dist_teta_positionner_hp, "ra_setpos", A_GIMME, 0);
	class_addmethod(c, (method)vbapan_tilde_changer_rayon_disq_centrale, "set_disc", A_FLOAT, 0);
	class_addmethod(c, (method)vbapan_tilde_informations, "get_info", 0);
	class_addmethod(c, (method)vbapan_tilde_assist,"assist",A_CANT,0);
	
	cicmtools_post_credits();
	
	class_dspinit(c);
	class_register(CLASS_BOX, c);
	vbapan_tilde_class = c;
}




/**********************************************************************/
/*      FONCTION QUI DONNE LE MODULO 360 D'UN ANGLE EN FLOTTANT       */
/**********************************************************************/

float vbapan_tilde_modulo(float anglep)
{
  float angle;

  if( anglep >= 0 && anglep < 360 )    return anglep;

  angle = anglep;
  while( angle < 0 )     angle += 360;
  while( angle >= 360 )  angle -= 360;
    
  return angle;
}



/**********************************************************************/
/*               FONCTION CHANGER LES MATRICES DES HP                 */
/**********************************************************************/

void vbapan_tilde_init_hp_mat(t_vbapan_tilde *x)
{
  float xhp1, yhp1, xhp2, yhp2; //CoordonnÈes provisoires des hp,
  int   hp, i, j;               //indices de boucles,
  int   itemp; float ftemp;     //entier et flottant temporaire,
  float g[Nmax];                //gains de la position centrale,
  float Puis, iAmp;             //puissance et amplitude des gains,
  float pteta, pdist;           //angle et distance porovisoires,
  float g1, g2;                 //gains provisoires.
  float px[4]={1,0,-1, 0};      //positions des 4 points pour la position centrale,
  float py[4]={0,1, 0,-1};


  //Initialisation des tableaux:
  for( i=0; i<Nmax; i++)
  {
    x->rev[i] = i;
    g[i] = 0;
  }


  //Arrangement du tableau rev qui donne l'ordre croissant des angles des hp:
  // ordre croissant -> ordre de crÈation.
  for( j=0; j<x->N; j++ ) {
    for( i=0; i<x->N-1; i++ ) {
      if( x->teta[x->rev[i]] > x->teta[x->rev[i+1]] )
      {
        itemp = x->rev[i];
        x->rev[i] = x->rev[i+1];
        x->rev[i+1] = itemp;
      }
    }
  }
  

  //Calcul des distances des haut-parleurs:
  for( hp=0; hp<x->N; hp++)
    x->dst_hp[hp] = (float)sqrt( x->x_hp[hp]*x->x_hp[hp] + x->y_hp[hp]*x->y_hp[hp] );


  //Calcul des matrices inversées, dans l'ordre des tÈta croissants :
  for( hp=0; hp<x->N; hp++)
  {

    // CoodonnÈes des hp ramenÈs sur le cercle
    xhp1 = x->x_hp[ x->rev[hp]          ] / x->dst_hp[x->rev[hp]];
    yhp1 = x->y_hp[ x->rev[hp]          ] / x->dst_hp[x->rev[hp]];
    xhp2 = x->x_hp[ x->rev[(hp+1)%x->N] ] / x->dst_hp[x->rev[(hp+1)%x->N]];
    yhp2 = x->y_hp[ x->rev[(hp+1)%x->N] ] / x->dst_hp[x->rev[(hp+1)%x->N]];


    // Calcul du dÈnominateur
    ftemp = xhp1 * yhp2 - yhp1 * xhp2;

    // Si le dÈnominateur est nul, c'est que 2 hp consÈcutifs 
    //sont alignÈs, c'est pas possible.
    if( fabs( ftemp ) <= EPSILON )    {
      object_error((t_object *)x, "deux haut-parleurs adjacents sont alignes, \n"
            "  la configuration actuelle n'est pas possible.");
      ftemp = 0;
    }
    else
      ftemp = 1/ftemp;


    // Calcul de la matrice inverse associÈe aux 2 hp:
    x->L[hp][0][0] =  ftemp*yhp2;
    x->L[hp][0][1] = -ftemp*xhp2;
    x->L[hp][1][0] = -ftemp*yhp1;
    x->L[hp][1][1] =  ftemp*xhp1;
  }

  //Boucle sur les quatres points pour calculer les gains de la position centrale.
  for( i = 0; i < 4; i++ )
  {
      pdist = (float)sqrt( px[i]*px[i] + py[i]*py[i] );
  
      pteta = (float)acos( px[i]/pdist );
      if( py[i] < 0 )
        pteta = 2*Pi - pteta;
 
      //Recherche des hp encadrant le point:
      for( hp=0; hp<x->N-1; hp++){
        if( pteta >= x->teta[x->rev[hp]] && pteta < x->teta[x->rev[(hp+1)%x->N]] )
          break;
      }

      //Calcul des gains:
      g1 = x->L[hp][0][0]*px[i] + x->L[hp][0][1]*py[i];
      g2 = x->L[hp][1][0]*px[i] + x->L[hp][1][1]*py[i];

      //Calcul de l'amplitude efficace:
      Puis =  g1*g1 + g2*g2;
      iAmp = (Puis <= EPSILON) ? 0 : (1/(float)sqrt(2*Puis));

      //Normalisation des g, et prise en compte des distances:
      g[x->rev[hp]]           +=  g1 * iAmp * x->dst_hp[x->rev[ hp        ]] /pdist;
      g[x->rev[(hp+1)%x->N]]  +=  g2 * iAmp * x->dst_hp[x->rev[(hp+1)%x->N]] /pdist;
  }

  //Calcul de la puissance global:
  Puis = 0;
  for( hp=0; hp<x->N; hp++)
    Puis += (float)pow( g[hp]/x->dst_hp[x->rev[hp]], 2 );

  //Affectation dans la dataspace avec normalisation :
  for( hp=0; hp<x->N; hp++) 
    x->G0[hp] = (float)(g[hp]/(sqrt(2*Puis)*x->r_c));


  return;
}
