/**********************************************************************/
//                                                                    //
// /****************************************************************/ //
// /*                                                              */ //
// /*                         CICM-TOOLS                           */ //
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

#ifndef CicmTools_h
#define CicmTools_h

#include "ext.h"
#include "ext_obex.h"
#include "z_dsp.h"
#include <math.h>

void cicmtools_post_credits()
{
    /*Publicité pour le CICM *********************************/
    post("\"CICM-Tools\"  Auteurs: R.MIGNOT  "
         "et B.COURRIBET, CICM Université Paris8,");
    post("           MSH Paris Nord, ACI Jeunes "
         "Chercheurs \"Espaces Sonores\".");
    post("           cicm\100univ-paris8.fr.");
    post("v2.1, Max 7 version by E.PARIS.");
}

#endif /* CicmTools_h */
