/*    CICM-Tools Objects :        version 2.0        for MaxMSP/jitter 5/6, MacOsx and Windows                comments, info, bugs, suggestion, requests appreciated.         mail _at_ rmignot@mshparisnord.org
        mail _at_ eliott.paris@yahoo.fr                */    Copyright (C) 2003-2004 RŽmi Mignot, MSH Paris Nordspatialization externals set for MaxMSP/jitter by cycling74. 


install notes:-    place this folder somewhere within the MaxMSP "search path" (+ erase the older version of the objects).

-    make sure you don't have any older version of CICM-tools in your "search path".-    copy 'CICM-Tools-overview.maxpat' in max/patches/extras folder.    

object listing and short description:ambipan~      2D spatialization by ambisonic B format.
poly.ambipan~ 2D spatialization of multiples sources by ambisonic B format. (Mac Only).ambicube~     3D spatialization by ambisonic B format.vbapan~       2D spatialization by Vector Base Amplitude Panning

history:__history / changes vs.1.5

 -  reference files added. -  method assist.
 -  method Main updated.
 -  method DSP updated. works both with 32 an 64 bit signals.
 -  accept control and signal data (without having to specify it by an argument).
 -  optimization when the object is in a muted patcher context.
 -  objects don't freeze Max anymore when we delete an object with audio turned on.
 -  errors updated.
 -  64 loudspeakers Maximum for ambipan~/vbapan~.